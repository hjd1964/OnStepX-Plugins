// BluetoothBle plugin for OnStepX
#include "BluetoothBle.h"

#ifdef ESP32

#include "Config.h"
#include "../../Common.h"
#include "../../lib/serial/Serial_Local.h"
#include "../../lib/tasks/OnTask.h"

#include <NimBLEDevice.h>

// Nordic UART Service UUIDs — names are from the central's (iOS) perspective:
//   NUS_TX (0002): central TXmits → peripheral receives   → we expose as WRITE
//   NUS_RX (0003): central RXeives ← peripheral notifies  → we expose as NOTIFY
static const char *NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *NUS_TX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *NUS_RX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

// ── BLE callbacks ─────────────────────────────────────────────────────────────

class BleServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *) override    { bluetoothBle.onConnect();    }
  void onDisconnect(NimBLEServer *) override {
    bluetoothBle.onDisconnect();
    NimBLEDevice::startAdvertising();  // re-advertise so the app can reconnect
  }
};

class BleWriteCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *ch) override {
    const std::string &v = ch->getValue();
    if (!v.empty())
      bluetoothBle.onBleWrite(reinterpret_cast<const uint8_t *>(v.data()), v.length());
  }
};

// ── OnTask wrapper ────────────────────────────────────────────────────────────

static void bleWrapper() { bluetoothBle.loop(); }

// ── BluetoothBle::init ────────────────────────────────────────────────────────

void BluetoothBle::init() {
  VLF("MSG: Plugins, starting: bluetoothBle");

  mutex = xSemaphoreCreateMutex();
  if (!mutex) { DLF("ERR: BluetoothBle, mutex create failed"); return; }

  NimBLEDevice::init(BLE_DEVICE_NAME);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // maximum TX power

  NimBLEServer *server = NimBLEDevice::createServer();
  server->setCallbacks(new BleServerCallbacks());

  NimBLEService *svc = server->createService(NUS_SERVICE_UUID);

  // iOS writes LX200 commands to this characteristic.
  NimBLECharacteristic *txCh = svc->createCharacteristic(
    NUS_TX_CHAR_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  txCh->setCallbacks(new BleWriteCallbacks());

  // ESP32 notifies LX200 responses on this characteristic.
  rxChar = svc->createCharacteristic(NUS_RX_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);

  svc->start();

  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SERVICE_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);  // recommended for iOS connection stability
  adv->setMaxPreferred(0x12);
  NimBLEDevice::startAdvertising();

  VLF("MSG: BluetoothBle, advertising as '" BLE_DEVICE_NAME "'");

  // Poll loop every 20 ms; priority 7 matches the sample plugin.
  tasks.add(20, 0, true, 7, bleWrapper);
}

// ── Connection state ──────────────────────────────────────────────────────────

void BluetoothBle::onConnect() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  clientConnected = true;
  xSemaphoreGive(mutex);
  VLF("MSG: BluetoothBle, client connected");
}

void BluetoothBle::onDisconnect() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  clientConnected = false;
  xSemaphoreGive(mutex);
  VLF("MSG: BluetoothBle, client disconnected");
}

// ── BLE data arrival (runs on BLE stack task) ─────────────────────────────────

void BluetoothBle::onBleWrite(const uint8_t *data, size_t len) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  size_t space = sizeof(rxBuf) - rxLen - 1;
  size_t n     = (len < space) ? len : space;
  memcpy(rxBuf + rxLen, data, n);
  rxLen       += (int)n;
  rxBuf[rxLen] = '\0';
  xSemaphoreGive(mutex);
}

// ── loop (runs on OnTask every 20 ms) ────────────────────────────────────────

void BluetoothBle::loop() {
  // Snapshot the shared buffer so we hold the mutex for as short a time as possible.
  char   local[512] = "";
  bool   connected  = false;

  xSemaphoreTake(mutex, portMAX_DELAY);
  connected = clientConnected;
  if (rxLen > 0 && connected) {
    memcpy(local, rxBuf, rxLen + 1);
    rxLen    = 0;
    rxBuf[0] = '\0';
  }
  xSemaphoreGive(mutex);

  if (!connected || local[0] == '\0') return;

  // Walk the buffer, processing one '#'-terminated command at a time.
  char *p = local;
  while (*p) {
    char *end = strchr(p, '#');
    if (!end) {
      // Partial command — put the leftover bytes back at the front of rxBuf
      // so the next loop() call can complete it with fresh incoming data.
      xSemaphoreTake(mutex, portMAX_DELAY);
      int partLen = (int)strlen(p);
      int total   = partLen + rxLen;
      if (total < (int)sizeof(rxBuf) - 1) {
        memmove(rxBuf + partLen, rxBuf, rxLen);
        memcpy(rxBuf, p, partLen);
        rxLen        = total;
        rxBuf[rxLen] = '\0';
      }
      xSemaphoreGive(mutex);
      break;
    }

    int cmdLen = (int)(end - p) + 1;  // includes '#'
    if (cmdLen > 0 && cmdLen < (int)sizeof(local)) {
      char cmd[256] = "";
      strncpy(cmd, p, cmdLen);
      cmd[cmdLen] = '\0';
      processCommand(cmd);
    }
    p = end + 1;
  }
}

// ── processCommand ────────────────────────────────────────────────────────────

void BluetoothBle::processCommand(const char *cmd) {
  VF("MSG: BluetoothBle, cmd: "); VLF(cmd);

  SERIAL_LOCAL.transmit(cmd);

  // Poll in 5 ms increments up to BLE_RESPONSE_TIMEOUT_MS for OnStepX to
  // produce a response.  Commands with no response (motion, stop, tracking)
  // will exhaust the timeout and result in no BLE notification — which is
  // correct: the iOS app doesn't wait for a reply from those commands.
  const int polls = BLE_RESPONSE_TIMEOUT_MS / 5;
  for (int i = 0; i < polls; i++) {
    tasks.yield(5);
    if (SERIAL_LOCAL.receiveAvailable() > 0) break;
  }

  char *resp = SERIAL_LOCAL.receive();
  if (!resp || resp[0] == '\0') {
    VLF("MSG: BluetoothBle, no response");
    return;
  }

  // The iOS parser requires a '#' terminator.  Append one if it's missing.
  size_t rlen = strlen(resp);
  char   buf[130];
  if (resp[rlen - 1] == '#') {
    strncpy(buf, resp, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
  } else {
    snprintf(buf, sizeof(buf), "%s#", resp);
  }

  VF("MSG: BluetoothBle, resp: "); VLF(buf);

  xSemaphoreTake(mutex, portMAX_DELAY);
  bool conn = clientConnected;
  xSemaphoreGive(mutex);

  if (conn && rxChar) {
    rxChar->setValue(reinterpret_cast<uint8_t *>(buf), strlen(buf));
    rxChar->notify();
  }
}

// ── command (no custom LX200 commands) ───────────────────────────────────────

bool BluetoothBle::command(char *reply, char *command, char *parameter,
                            bool *suppressFrame, bool *numericReply,
                            CommandError *commandError) {
  UNUSED(*reply);
  UNUSED(*command);
  UNUSED(*parameter);
  UNUSED(*suppressFrame);
  UNUSED(*numericReply);
  UNUSED(*commandError);
  return false;
}

BluetoothBle bluetoothBle;

#endif  // ESP32
