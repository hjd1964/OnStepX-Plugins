# OnStepX/OCS MQTT Plugin

## Overview

The MQTT Plugin creates a bi-directional bridge between MQTT and OnStepX/OCS. It receives MQTT messages in command format, processes them, and publishes both the received command and the response. It also broadcasts commands received from other command channels (USB, Serial, etc.) to MQTT for monitoring.

This plugin works with both:
- **OnStepX**: Telescope mount control system
- **OCS**: Observatory Control System (roof, dome, power, weather, lighting, thermostat)

## Features

- **Dual Platform Support**: Works with both OnStepX and OCS without modification
- **Platform Independent**: Works on all supported platforms (ESP32, ESP8266, Teensy, Arduino Due, etc.)
- **Network Agnostic**: Supports both WiFi (ESP32/ESP8266) and Ethernet (Teensy, Due, etc.)
- **Bi-directional Communication**: Receive commands via MQTT and send responses back
- **Command Broadcasting**: Echo commands from any channel (MQTT, USB, Serial, etc.)
- **Non-Interfering**: Does not capture or block responses to other command channels
- **Slightly Secure Authentication**: Username and password authentication support
- **Auto-Reconnection**: Automatic reconnection to MQTT broker on connection loss
- **Connection Status Monitoring**: Online/offline status via Last Will and Testament
- **Reconnection Notifications**: Messages published when connection is restored after loss
- **Message Loss Warning**: Automatic notification when connection restored indicating potential message gaps
- **Quality of Service**: QoS 1 (at-least-once delivery) for commands, QoS 0 for echo responses
- **Configurable Topics**: Separate topics for commands, responses, echoes, and status

## Requirements

- OnStepX or OCS controller on any supported platform
- Network connection configured (WiFi or Ethernet depending on platform)
- MQTT broker accessible on the network
- PubSubClient library (installed via Arduino Library Manager)

## Installation

Copy the `/mqtt` directory into the appropriate plugins directory:
 - **OnStepX**: `OnStepX/src/plugins`
 - **OCS**: `OCS/src/plugins`

Add an entry in `Plugins.config.h` (identical for both OnStepX and OCS):

```cpp
#define PLUGIN1                       mqtt
#define PLUGIN1_COMMAND_PROCESSING    ON
#include "mqtt/Mqtt.h"
```

Install the PubSubClient library:
  Open Arduino IDE
- Go to Sketch → Include Library → Manage Libraries
- Search for "PubSubClient"
- Install "PubSubClient" by Nick O'Leary
- or download directly from https://github.com/knolleary/pubsubclient

Configure your MQTT broker settings in `mqtt/Mqtt.h`:

```cpp
#define MQTT_BROKER_HOST       "192.168.1.100"   // Your MQTT broker IP
#define MQTT_BROKER_PORT       1883              // MQTT broker port
#define MQTT_USERNAME          "onstep"          // MQTT username
#define MQTT_PASSWORD          "password"        // MQTT password
```

**For multiple controllers on the same network:**

By default, the plugin uses `HOST_NAME` (from your main Config.h) to create unique MQTT topics. This allows multiple controllers to connect to the same broker without conflicts.

If you need to override the device identifier (e.g., to use different names for MQTT than your network hostname), add this line in `mqtt/Mqtt.h` before the other configuration:

```cpp
#define MQTT_DEVICE_ID         "Telescope"       // Override default HOST_NAME
```

Example multi-controller setup:
- **Main telescope mount**: Uses `HOST_NAME` → topics like `Telescope/cmd`
- **Auxiliary devices**: Override with `#define MQTT_DEVICE_ID "Auxiliary"` → topics like `Auxiliary/cmd`
- **Observatory control**: Override with `#define MQTT_DEVICE_ID "Observatory"` → topics like `Observatory/cmd`

**Important**: MQTT_DEVICE_ID must contain only letters (A-Z, a-z), numbers (0-9), underscore (_), and hyphen (-). Spaces and special characters are not allowed and will cause a compile error with a descriptive message.

## Configuration

### MQTT Broker Settings

Edit these values in `Mqtt.h` to match your MQTT broker configuration:

- `MQTT_BROKER_HOST`: IP address or hostname of your MQTT broker
- `MQTT_BROKER_PORT`: Port number (default: 1883 for non-SSL)
- `MQTT_USERNAME`: Username for MQTT authentication (leave as empty string `""` if no auth)
- `MQTT_PASSWORD`: Password for MQTT authentication (leave as empty string `""` if no auth)

### MQTT Topics

The plugin creates topics using the device identifier (MQTT_DEVICE_ID or HOST_NAME):

- `<device-id>/cmd`: Subscribe to this topic to send commands
- `<device-id>/response`: Responses to commands are published here
- `<device-id>/echo`: All commands (from MQTT and other channels) are echoed here
- `<device-id>/status`: Online/offline status messages


This allows you to:
- Send commands to specific controllers
- Monitor all controllers from a single MQTT client
- Coordinate operations between controllers
- Use wildcards to subscribe to groups (e.g., `+/status` for all status messages)

**Connection Status and Reliability:**

The plugin monitors connection health and provides status information:

- **Last Will and Testament**: The broker automatically publishes "offline" to the status topic if the controller disconnects unexpectedly
- **Reconnection Messages**: When connection is restored after loss, a message is published to the echo topic indicating downtime duration and potential message loss
- **QoS 1 Delivery**: All command messages use QoS 1 to ensure delivery even with temporary network issues

### Connection Settings

- `MQTT_RECONNECT_DELAY`: Time in milliseconds between reconnection attempts (default: 5000)
- `MQTT_KEEPALIVE`: MQTT keepalive interval in seconds (default: 60)
- `MQTT_QOS`: Quality of Service level, 0, 1, or 2 (default: 1)
- `MQTT_RETAIN`: Whether to retain messages on the broker (default: false)

**Quality of Service (QoS):**

Due to PubSubClient library limitations:
- **Incoming commands** (subscriptions): QoS 1 - guaranteed delivery of commands to the controller
- **Outgoing messages** (publications): QoS 0 - responses, echoes, and status messages are sent without delivery guarantee

This means:
- Commands sent to the controller via MQTT are guaranteed to arrive (QoS 1 subscription)
- Responses and status updates could be lost during brief network interruptions (QoS 0 publishing)
- For mission-critical monitoring, consider using a different MQTT library that supports QoS 1 publishing

## Usage

### Sending Commands

Publish commands to the `<device-id>/cmd` topic in standard command format with colon prefix and hash suffix.

**Single Controller Examples** using mosquitto_pub:

```bash
# Send to telescope controller (assuming HOST_NAME or MQTT_DEVICE_ID is "Telescope")
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Telescope/cmd" -m ":GVP#"
```
```bash
# Get safety status
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Observatory/cmd" -m ":Gs#"

```

### Receiving Responses

Subscribe to `<device-id>/response` to receive command responses:

```bash
# Single controller
mosquitto_sub -h 192.168.1.100 -u onstep -P password -t "Telescope/response"

# Multiple controllers - use wildcards
mosquitto_sub -h 192.168.1.100 -u onstep -P password -t "+/response"
```

The `Source` field indicates where the command originated:
- `MQTT`: Command received via MQTT (includes response)
- `OTHER`: Command received from another channel such as USB or Serial

**Note**: Commands from MQTT show both the command and response. Commands from OTHER sources only show the command; their responses are sent directly to the originating channel (USB, Serial, etc.) and are not captured by the plugin due to the command processing architecture.

**Connection Events:**

The echo topic also publishes connection state events:
```
Connection restored after 45 seconds - commands and responses during this period were not published to MQTT
```

This message appears when MQTT connection is restored after being lost, indicating:
- How long the connection was unavailable
- That there may be gaps in the MQTT message stream during that period
- Commands from other channels (USB, Serial) continued to work during the outage

### Status Monitoring

Subscribe to `<device-id>/status` to monitor connection status:

```bash
# Single controller
mosquitto_sub -h 192.168.1.100 -u onstep -P password -t "Telescope/status"

# All controllers
mosquitto_sub -h 192.168.1.100 -u onstep -P password -t "+/status"
```

Status values:
- `online`: MQTT plugin is connected to the broker
- `offline`: MQTT plugin has disconnected (Last Will and Testament message)

**Last Will and Testament:**

The plugin uses MQTT's Last Will feature to automatically publish "offline" status if:
- Controller loses power
- Network connection lost
- Controller crashes
- Any ungraceful disconnection

When connection is restored, "online" status is published immediately. For graceful disconnections with reconnection, check the echo topic for detailed timing information.

## Security Considerations

**IMPORTANT**: This plugin stores MQTT credentials in **plain text** in the `Mqtt.h` file.

## License

This plugin follows the same GPL-3.0 license as OnStepX.

## Support

Originally written by claude.ai wrestled into submission by Ed Lee
https://github.com/LuckyEddie47/OnStepX-Plugins

Tested with:
- 
- OCS          3.12j
- OnStepX      10.25n
- PubSubClient 2.80
- Mosquitto    2.0.18
- MQTTX        1.12.1

## Version History

- V1.0 Initial release