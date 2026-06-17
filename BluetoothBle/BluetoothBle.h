// BluetoothBle plugin for OnStepX
//
// Bridges the OnStepX LX200 command interface over BLE using the Nordic UART
// Service (NUS), so any BLE client (iOS, Android, desktop) can connect without a WiFi module.
//
// Requires: NimBLE-Arduino library
//   Arduino IDE  → Sketch ▸ Include Library ▸ Manage Libraries → "NimBLE-Arduino"
//   PlatformIO   → lib_deps = h2zero/NimBLE-Arduino
//
// Platform: ESP32 only
// Note: do not enable SERIAL_BLUETOOTH (classic BT) at the same time — the
//       two Bluetooth stacks share the radio and will conflict.
#pragma once

#include "../../Common.h"
#include "../../lib/commands/CommandErrors.h"

#ifdef ESP32

#include <NimBLEDevice.h>

class BluetoothBle {
public:
  void init();
  void loop();

  // Required by Plugins.config.h — this plugin has no custom LX200 commands.
  bool command(char *reply, char *command, char *parameter,
               bool *suppressFrame, bool *numericReply, CommandError *commandError);

  // Called from the NimBLE stack task — must be public for the callbacks.
  void onBleWrite(const uint8_t *data, size_t len);
  void onConnect();
  void onDisconnect();

private:
  void processCommand(const char *cmd);

  SemaphoreHandle_t mutex     = nullptr;
  NimBLECharacteristic *rxChar = nullptr;  // notify characteristic (ESP32 → client)

  // Shared receive buffer — written by the BLE task, drained by loop().
  char   rxBuf[512] = "";
  int    rxLen      = 0;

  bool    clientConnected = false;
  uint8_t pendingHandle   = 0;  // CommandBroker handle; 0 = none in-flight
};

extern BluetoothBle bluetoothBle;

#endif  // ESP32
