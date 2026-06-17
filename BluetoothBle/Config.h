// BluetoothBle plugin configuration
#pragma once

// Name the ESP32 advertises over BLE — shown in the device picker
#ifndef BLE_DEVICE_NAME
  #define BLE_DEVICE_NAME               "OnStepX"
#endif

// Maximum milliseconds to wait for OnStepX to produce a response after
// forwarding a command.  Most queries respond in < 5 ms; this cap covers
// slow or no-response commands without blocking the BLE loop indefinitely.
#ifndef BLE_RESPONSE_TIMEOUT_MS
  #define BLE_RESPONSE_TIMEOUT_MS        1000
#endif
