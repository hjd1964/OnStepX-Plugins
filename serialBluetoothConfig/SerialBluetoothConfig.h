#pragma once

#include "Config.h"

class SerialBluetoothConfigPlugin {
  public:
    // Called by OnStepX during initialization
    void init() {
      // This plugin only runs if a serial port for Bluetooth is defined in OnStepX's main Config.h
      #if SERIAL_BLUETOOTH != OFF
        // And if a name is set in this plugin's Config.h
        #if defined(BT_SET_NAME)
          VLF("MSG: BT_CONFIG, checking module. Ensure module is in AT mode (slow blink).");
          SERIAL_BLUETOOTH.begin(9600); // HC-06 AT command mode is usually 9600 baud
          delay(2000); // Wait for module to initialize

          // Send AT and check for OK response before proceeding
          SERIAL_BLUETOOTH.print("AT");
          delay(1000);
          String response = "";
          while (SERIAL_BLUETOOTH.available()) { response += (char)SERIAL_BLUETOOTH.read(); }

          if (response.indexOf("OK") != -1) {
            VLF("MSG: BT_CONFIG, module responded. Checking configuration...");
            
            // Check current name
            SERIAL_BLUETOOTH.print("AT+NAME?");
            delay(1000);
            response = "";
            while (SERIAL_BLUETOOTH.available()) { response += (char)SERIAL_BLUETOOTH.read(); }

            String desiredName = BT_SET_NAME;
            if (response.indexOf(desiredName) == -1) {
              VLF("MSG: BT_CONFIG, name mismatch. Sending new configuration...");
              SERIAL_BLUETOOTH.print("AT+NAME"); SERIAL_BLUETOOTH.print(BT_SET_NAME); delay(500);
              
              // Set baud rate based on BT_SET_BAUD
              if (BT_SET_BAUD == 1200) { SERIAL_BLUETOOTH.print("AT+BAUD1"); }
              else if (BT_SET_BAUD == 2400) { SERIAL_BLUETOOTH.print("AT+BAUD2"); }
              else if (BT_SET_BAUD == 4800) { SERIAL_BLUETOOTH.print("AT+BAUD3"); }
              else if (BT_SET_BAUD == 9600) { SERIAL_BLUETOOTH.print("AT+BAUD4"); }
              else if (BT_SET_BAUD == 19200) { SERIAL_BLUETOOTH.print("AT+BAUD5"); }
              else if (BT_SET_BAUD == 38400) { SERIAL_BLUETOOTH.print("AT+BAUD6"); }
              else if (BT_SET_BAUD == 57600) { SERIAL_BLUETOOTH.print("AT+BAUD7"); }
              else if (BT_SET_BAUD == 115200) { SERIAL_BLUETOOTH.print("AT+BAUD8"); }
              delay(500);

              SERIAL_BLUETOOTH.print("AT+PIN"); SERIAL_BLUETOOTH.print(BT_SET_PIN); delay(1000);

              VLF("MSG: BT_CONFIG, setup complete.");
            } else {
              VLF("MSG: BT_CONFIG, name matches. Skipping setup.");
            }
          } else {
            VLF("MSG: BT_CONFIG, module not responding. Skipping setup. Will try again on next boot.");
          }
          // It's important to set the baud rate back to what OnStepX expects for normal operation
          #if defined(SERIAL_BAUD_BLUETOOTH)
            SERIAL_BLUETOOTH.begin(SERIAL_BAUD_BLUETOOTH);
          #endif
        #endif
      #endif
    }
};

// Instantiate the plugin object
SerialBluetoothConfigPlugin serialBluetoothConfigPlugin;
