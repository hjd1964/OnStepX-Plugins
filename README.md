# OnStepX-Plugins


## Bluetooth BLE

Exposes the OnStepX LX200 command interface over BLE using the **Nordic UART Service (NUS)**, so iOS and Android apps that support NUS can connect wirelessly without a WiFi module.

> **Note:** This plugin uses BLE (Bluetooth Low Energy / Bluetooth 4.0+) and is **not compatible with the existing OnStep Android app** (OnStep Controller2). That app uses classic Bluetooth SPP, which is a completely different protocol. To use OnStepX with the existing Android app do not use thie plugin.

### Requirements

- **Platform:** ESP32 only
- **Library:** [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) — use version **1.4.2** for ESP32 Arduino core 2.x. NimBLE-Arduino 2.x requires core 3.x and will not compile against 2.0.17.
  - Arduino IDE: *Sketch → Include Library → Manage Libraries* → search **NimBLE-Arduino** → select version **1.4.2** from the version dropdown → Install
  - PlatformIO: add `h2zero/NimBLE-Arduino@^1.4.2` to `lib_deps`
- **Do not** set `SERIAL_BLUETOOTH` in OnStepX's `Config.h` while using this plugin. Classic Bluetooth and BLE share the same radio hardware on the ESP32 and the two stacks will conflict, causing crashes or failed initialization at startup. Ensure `SERIAL_BLUETOOTH` is set to `OFF` in `Config.h`.

### Installation

Copy the `/BluetoothBle` directory into the `OnStepX/src/plugins` directory and add an entry to `Plugins.config.h`:

```cpp
#define PLUGIN1                    bluetoothBle
#include "BluetoothBle/BluetoothBle.h"
#define PLUGIN1_COMMAND_PROCESSING OFF
```

Configuration is in `/BluetoothBle/Config.h`:

```cpp
#define BLE_DEVICE_NAME         "OnStepX"  // name shown in iOS device picker
#define BLE_RESPONSE_TIMEOUT_MS  50        // ms to wait for a response from OnStepX
```

### How it works

The plugin advertises the NUS service (`6E400001-…`) and bridges BLE UART traffic to `SERIAL_LOCAL`, which is OnStepX's internal LX200 command processor. Any NUS-compatible app (including the companion iOS app) can connect, send LX200 commands, and receive responses exactly as it would over WiFi.

NUS characteristic layout (names are from the central/iOS perspective):

| UUID suffix | Direction | Property |
|-------------|-----------|----------|
| `…0002` | iOS → ESP32 (commands) | WRITE / WRITE_NR |
| `…0003` | ESP32 → iOS (responses) | NOTIFY |


## Website

The SWS Website for ESP32's and WiFi only, ported to OnStepX.

**You must copy the /website directory into the OnStepX/src/plugins directory and add an entery for it in Plugins.config.h similar to the following:**
- #define PLUGIN1 website 
- #include "website/Website.h"

**This plugin requires that WiFi be activated in OnStepX's Config.h file, for example:**
- #define SERIAL_RADIO WIFI_ACCESS_POINT

Which enables WiFi and the Webserver components of OnStepX.

OnStepX will then initialize and use it.

Additional settings are in /website/Config.h

## Guide Rate Rheostat

You must copy the /guideRateRheostat directory into the OnStepX/src/plugins directory and add an entery for it in Plugins.config.h similar to the following:

- #define PLUGIN1 guideRateRheostat
- #include "guideRateRheostat/GuideRateRheostat.h"

The guide rate rheostat allows using an knob, on a basic hand controller for example, to adjust the guide rate.

Its settings are in /guideRateRheostat/Config.h and you need to set the pin to be used for analog input, the values to describe the rheostat voltage divider, etc.

## BLE Gamepad

BLE gamepad controller support for ESP32 based OnStepX builds. This plugin connects to supported BLE HID gamepads and uses the directional joystick as telescope slew controls.

This plugin is based on the SmartWebServer [bleGamepad module](https://github.com/hjd1964/SmartWebServer/tree/main/src/libApp/bleGamepad), adapted for OnStepX plugins and NimBLE.

You must copy the /blegamepad directory into the OnStepX/src/plugins directory and add an entry for it in Plugins.config.h similar to the following:

```
#define PLUGIN1                       blegamepad
#include "blegamepad/BleGamepad.h"
```

This plugin requires the NimBLE-Arduino library and ESP32 BLE support.

Configuration settings are in /blegamepad/BleConfig.h. Set `BLE_GP_ADDR` to your controller MAC address before enabling the plugin.

## Serial Bluetooth Config

For external HC-05/06 modules. This plugin sends AT commands to configure the module one time at startup. You may need to hold the button on the BT module (if present) while powering on OnStep to enter AT command mode.

This plugin requires that a serial port for Bluetooth is activated in OnStepX's `Config.h` file, along with its corresponding baud rate.

Example for MaxPCB4 (which uses `Serial1`):
- `#define SERIAL_BLUETOOTH Serial1`
- `#define SERIAL_C_BAUD 9600`

You must copy the /serialBluetoothConfig directory into the OnStepX/src/plugins directory and add an entry for it in Plugins.config.h similar to the following:

- `#define PLUGIN1 serialBluetoothConfigPlugin`
- `#include "serialBluetoothConfig/SerialBluetoothConfig.h"`

Configuration settings are in /serialBluetoothConfig/Config.h

## ElegantOTA

First, you must add [ElegantOTA](https://docs.elegantota.pro/) to your libraries. Follow the instructions [here](https://docs.elegantota.pro/getting-started/installation).
You must copy the /elegantota directory into the OnStepX/src/plugins directory and add an entery for it in Plugins.config.h similar to the following:

```
#define PLUGIN1 elegantOTAPlugin
#define PLUGIN2_COMMAND_PROCESSING ON // Only required if WiFi is disabled
#include "elegantota/ElegantOTAPlugin.h"
#define ELEGANTOTA_PLUGIN_SSID "OnStepX-OTA" // Optional, access point SSID
#define ELEGANTOTA_PLUGIN_PSK "onstepx-ota-wifipassword" // Optional
#define ELEGANTOTA_PLUGIN_USERNAME "otauser" // Optional
#define ELEGANTOTA_PLUGIN_PASSWORD "otapassword" // Optional
#endif
```

To add ElegantOTA to your OnStepX website plugin menu, you can define the following in `Config.h`:
```
#define WEBSITE_PLUGIN_PAGE1_TITLE "OTA Update"
#define WEBSITE_PLUGIN_PAGE1_URL "/update"
#define WEBSITE_PLUGIN_PAGE1_TARGET "_blank" // Optional, opens in a new tab
```

### Firmware upload

The plugin works in two different ways, depending on whether WiFi is enabled

 - If WiFi is enabled, ElegantOTA is always on, and reachable through the `/update` path
 - If WiFi is *not* enabled (for instance, if you're using a separate ESP8266 for the webserver), you need to manually send the `:EOTA#` command to the OnStepX serial port when you want to turn on ElegantOTA. The ElegantOTA plugin will start a WiFi access point (defined by `ELEGANTOTA_PLUGIN_SSID/ELEGANTOTA_PLUGIN_PSK`, default `OnStepX-OTA`, and no password). After connecting to the access point, simply go to the OnStepX IP address (typically `192.168.1.4`) to start the update.

Next, make sure that `Firmware` is selected in the upload page, and click "Choose file".
 - If you're using Arduino IDE, you need to select the compiled `firmware.bin`. This is usually in a temporary directory, but you can follow [this page](https://randomnerdtutorials.com/bin-binary-files-sketch-arduino-ide/) to build the firmware inside your project directory.
 - If you're using PlatformIO, the firmware should be in the `.pio/build/<environment-name>` directory of your project.

Once the file is selected, the upload is started automatically, and the board should reboot when it's successful.

## Metrics

Prometheus compatible metrics page for OnStepX.

This can be useful both for collecting statistics using Prometheus, or for debugging purposes.

This plugin depends on having the Website plugin enabled.

You must copy the /metrics directory into the OnStepX/src/plugins directory and add an entery for it in Plugins.config.h similar to the following:

```
#define PLUGIN2                       metricsPlugin // Assuming PLUGIN1 will be Website
#include "metrics/MetricsPlugin.h"
#endif
```

Once enabled, you can head to the path defined by `METRICS_PLUGIN_PATH` (by default `/metrics`) to see your metrics. This is also the path you'll want to use if you want to scrape these metrics using Prometheus.

You can also add the metrics to your OnStepX website menu by defining the following in `Config.h`:
```
#define WEBSITE_PLUGIN_PAGE2_TITLE "Metrics"
#define WEBSITE_PLUGIN_PAGE2_URL "/metrics"
#define WEBSITE_PLUGIN_PAGE2_TARGET "_blank"
```

### Adding metrics

An example implementation on how to add custom metrics is available with GPS metrics:
 - in `GPS.cpp`, add the following includes:
 ```
#include "../../../Common.h"
#include "../../../plugins/Plugins.config.h"
```
 - Also in `GPS.cpp`, inside the `init` method (towards the end), add the following snippet of code to initialise the GPS metrics:
 ```
  #ifdef HAS_METRICS_PLUGIN
  metricsPlugin.initGpsMetrics(gps);
  #endif
```
 - Now you can look at `initGpsMetrics` to see how metrics are populated.

## USB Switcher

An extension to the switch facility provided by the Auxiliary facilities.

This plugin defines an extra set of switchable outputs designated for USB ports. This allows combination auxiliary devices, such as a on-scope mounted power-box / USB hub, to have more options available as none of the 8 Features are used for switching USB ports.

You must copy the /usb directory into the OnStepX/src/plugins directory and add an entry for it in Plugins.config.h similar to the following:
```
#define PLUGIN1                       usb //    Can be any of the PLUGIN definition positions
#include "usb/Usb.h"                      //    Specify the header file to include the class.
#define PLUGIN1_COMMAND_PROCESSING     ON //    MUST be set to ON for this plugin
#endif
```
All configuration is contained in the Usb.h file, in this format:
```
 // Plugin config
  #define USB1_PIN                    29 //    OFF, n. I/O pin controlling the port state.    Option - 29 is just an example here
  #define USB1_NAME           "Main Cam" //    "USB..", Name of the device being controlled.  Option
  #define USB1_DEFAULT_STATE         OFF //    OFF, ON. State to startup in.                  Option
  #define USB1_ON_STATE             HIGH //    HIGH, LOW Port control pin ON (active) state.  Option
  #define USB2_PIN                   OFF //    OFF, n. I/O pin controlling the port state.    Option
  #define USB2_NAME ... etc
```
A maximum of 8 USB ports can be defined.

Additional serial commands are provided to control the USB ports:
```
:GUX[n]# - Get USB port n status
:GUY[n]# - Get USB port n name
:GUY0# - Get defined USB ports in format 11110000 where 1 indicates a defined port
:SUX[n],V[1|0]# - Set USB port n to state ON (V1) or OFF (V0)
:SUX0,V[1|0]# - Set all defined USB ports to state ON (V1) or OFF (V0)
```
