# OnStepX/OCS MQTT Plugin

## Overview

The MQTT Plugin creates a bi-directional bridge between MQTT and OnStepX/OCS. It receives MQTT messages in command format, processes them, and publishes both the received command and the response. It also broadcasts commands received from other command channels (USB, Serial, etc.) to MQTT for monitoring.

This plugin works with both:
- **OnStepX**: Telescope mount control system
- **OCS**: Observatory Control System (roof, dome, power, weather, lighting, thermostat)

**Important**: This plugin properly integrates with the command processing system and does NOT interfere with normal command/response flow. Commands from other channels continue to work normally and receive their responses through their original channels.

## Features

- **Dual Platform Support**: Works with both OnStepX and OCS without modification
- **Platform Independent**: Works on all supported platforms (ESP32, ESP8266, Teensy, Arduino Due, etc.)
- **Network Agnostic**: Supports both WiFi (ESP32/ESP8266) and Ethernet (Teensy, Due, etc.)
- **Bi-directional Communication**: Receive commands via MQTT and send responses back
- **Command Broadcasting**: Echo all commands from any channel (MQTT, USB, Serial, etc.)
- **Non-Interfering**: Does not capture or block responses to other command channels
- **Secure Authentication**: Username and password authentication support
- **Auto-Reconnection**: Automatic reconnection to MQTT broker on connection loss
- **Connection Status Monitoring**: Online/offline status via Last Will and Testament
- **Reconnection Notifications**: Messages published when connection is restored after loss
- **Message Loss Warning**: Automatic notification when connection restored indicating potential message gaps
- **Quality of Service**: QoS 1 (at-least-once delivery) for all messages
- **Configurable Topics**: Separate topics for commands, responses, echoes, and status

## Requirements

- OnStepX or OCS controller on any supported platform
- Network connection configured (WiFi or Ethernet depending on platform)
- MQTT broker accessible on the network
- PubSubClient library (installed via Arduino Library Manager)

### Platform-Specific Requirements

**ESP32/ESP8266:**
- WiFi configured in OnStepX Config.h
- No additional hardware required

**Teensy (3.x, 4.x):**
- Ethernet shield (W5500, W5100, etc.) or NativeEthernet
- Ethernet configured in OnStepX

**Arduino Due:**
- Ethernet shield
- Ethernet configured in OnStepX

## Installation

1. Copy the `/mqtt` directory into the appropriate plugins directory:
   - **OnStepX**: `OnStepX/src/plugins`
   - **OCS**: `OCS/src/plugins`

2. Add an entry in `Plugins.config.h` (identical for both OnStepX and OCS):

```cpp
#define PLUGIN1                       mqtt
#define PLUGIN1_COMMAND_PROCESSING    ON  // Required for broadcasting commands from other channels
#include "mqtt/Mqtt.h"
```

3. Install the PubSubClient library:
   - Open Arduino IDE
   - Go to Sketch → Include Library → Manage Libraries
   - Search for "PubSubClient"
   - Install "PubSubClient" by Nick O'Leary

4. Configure your MQTT broker settings in `mqtt/Mqtt.h`:

```cpp
#define MQTT_BROKER_HOST       "192.168.1.100"  // Your MQTT broker IP
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

5. Ensure your network is configured:
   - **For WiFi (ESP32/ESP8266)**: Configure in Config.h
   - **For Ethernet**: Configure Ethernet settings per your platform

6. Compile and upload to your controller

## Configuration

### MQTT Broker Settings

Edit these values in `Mqtt.h` to match your MQTT broker configuration:

- `MQTT_BROKER_HOST`: IP address or hostname of your MQTT broker
- `MQTT_BROKER_PORT`: Port number (default: 1883 for non-SSL)
- `MQTT_USERNAME`: Username for MQTT authentication (leave as empty string `""` if no auth)
- `MQTT_PASSWORD`: Password for MQTT authentication (leave as empty string `""` if no auth)

### Device Identification for Multiple Controllers

The plugin automatically uses `HOST_NAME` (from your main Config.h) to create unique MQTT topics. This allows multiple controllers to share the same MQTT broker.

**To override the device identifier**, add this line at the top of the configuration section in `Mqtt.h`:

```cpp
#define MQTT_DEVICE_ID         "YourDeviceName"
```

**Allowed characters**: Letters (A-Z, a-z), numbers (0-9), underscore (_), and hyphen (-) only. The compiler will show a clear error message if invalid characters are used.

**Example for a three-controller observatory:**

*Controller 1 - Main Telescope (OnStepX):*
```cpp
// In Mqtt.h - uses default HOST_NAME "Telescope"
// Results in topics: Telescope/cmd, Telescope/response, etc.
```

*Controller 2 - Auxiliary Devices (OnStepX):*
```cpp
// In Mqtt.h
#define MQTT_DEVICE_ID         "Auxiliary"
// Results in topics: Auxiliary/cmd, Auxiliary/response, etc.
```

*Controller 3 - Observatory Infrastructure (OCS):*
```cpp
// In Mqtt.h
#define MQTT_DEVICE_ID         "Observatory"
// Results in topics: Observatory/cmd, Observatory/response, etc.
```

### MQTT Topics

The plugin creates topics using the device identifier (MQTT_DEVICE_ID or HOST_NAME):

- `<device-id>/cmd`: Subscribe to this topic to send commands
- `<device-id>/response`: Responses to commands are published here
- `<device-id>/echo`: All commands (from MQTT and other channels) are echoed here
- `<device-id>/status`: Online/offline status messages

**Examples:**
- If `HOST_NAME` is "Telescope": Topics are `Telescope/cmd`, `Telescope/response`, etc.
- If `MQTT_DEVICE_ID` is "Observatory": Topics are `Observatory/cmd`, `Observatory/response`, etc.

**Multi-Controller Setup:**

When running multiple controllers with the same broker, each uses its own topic namespace:

```
Telescope/cmd         → Main mount commands
Telescope/response    → Main mount responses
Auxiliary/cmd         → Auxiliary device commands
Auxiliary/response    → Auxiliary device responses
Observatory/cmd       → OCS commands
Observatory/response  → OCS responses
```

This allows you to:
- Send commands to specific controllers
- Monitor all controllers from a single MQTT client
- Coordinate operations between controllers
- Use wildcards to subscribe to groups (e.g., `+/status` for all status messages)

**Connection Status and Reliability:**

The plugin monitors connection health and provides status information:

- **Last Will and Testament**: The broker automatically publishes "offline" to the status topic if the controller disconnects unexpectedly
- **Reconnection Messages**: When connection is restored after loss, a message is published to the echo topic indicating downtime duration and potential message loss
- **QoS 1 Delivery**: All messages use QoS 1 to ensure delivery even with temporary network issues

### Connection Settings

- `MQTT_RECONNECT_DELAY`: Time in milliseconds between reconnection attempts (default: 5000)
- `MQTT_KEEPALIVE`: MQTT keepalive interval in seconds (default: 60)
- `MQTT_QOS`: Quality of Service level, 0, 1, or 2 (default: 1)
- `MQTT_RETAIN`: Whether to retain messages on the broker (default: false)

**Quality of Service (QoS):**

The plugin uses QoS 1 by default, which provides:
- **Guaranteed delivery**: Messages will arrive at least once
- **Possible duplicates**: Messages may be delivered more than once if network interrupted
- **Acknowledgment**: Both sending and receiving are confirmed

QoS levels:
- **QoS 0** (at most once): Fastest, no guarantees - not recommended for commands
- **QoS 1** (at least once): Balanced, guaranteed delivery, possible duplicates - **recommended default**
- **QoS 2** (exactly once): Slowest, guaranteed exactly once - overkill for most telescope operations

## Usage

### Sending Commands

Publish commands to the `<device-id>/cmd` topic in standard command format with colon prefix and hash suffix.

**Single Controller Examples** using mosquitto_pub:

```bash
# Send to telescope controller (assuming HOST_NAME or MQTT_DEVICE_ID is "Telescope")
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Telescope/cmd" -m ":GVP#"
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Telescope/cmd" -m ":GR#"
```

**Multi-Controller Examples:**

```bash
# Main telescope mount
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Telescope/cmd" -m ":Te#"

# Auxiliary devices  
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Auxiliary/cmd" -m ":U2#"

# Observatory infrastructure
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Observatory/cmd" -m ":Gs#"
```

**OnStepX Command Examples:**

```bash
# Get firmware version
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Telescope/cmd" -m ":GVP#"

# Get Right Ascension
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Telescope/cmd" -m ":GR#"

# Start tracking
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Telescope/cmd" -m ":Te#"
```

**OCS Command Examples:**

```bash
# Get version
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Observatory/cmd" -m ":IN#"

# Get safety status
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Observatory/cmd" -m ":Gs#"

# Open roof (example - actual command depends on roof configuration)
mosquitto_pub -h 192.168.1.100 -u onstep -P password -t "Observatory/cmd" -m ":RO#"
```

### Receiving Responses

Subscribe to `<device-id>/response` to receive command responses:

```bash
# Single controller
mosquitto_sub -h 192.168.1.100 -u onstep -P password -t "Telescope/response"

# Multiple controllers - use wildcards
mosquitto_sub -h 192.168.1.100 -u onstep -P password -t "+/response"
```

Response example:
```
OnStep
12:34:56#
1#
```

### Monitoring All Commands

Subscribe to `<device-id>/echo` to see all commands processed, regardless of source:

```bash
# Single controller
mosquitto_sub -h 192.168.1.100 -u onstep -P password -t "Telescope/echo"

# All controllers
mosquitto_sub -h 192.168.1.100 -u onstep -P password -t "+/echo"
```

Echo message format:
```
Received: :GVP#, Response: OnStep, Source: MQTT
Received: :GR#, Response: 12:34:56#, Source: OTHER
Received: :Td#, Response: <no response>, Source: OTHER
```

The `Source` field indicates where the command originated:
- `MQTT`: Command received via MQTT
- `OTHER`: Command received from another channel (USB, Serial, etc.)

**Note**: Commands from OTHER sources have their responses captured and published to MQTT while also being returned to the originating channel. Commands that don't generate responses will show `<no response>`.

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

## Architecture

### Non-Interfering Design

This plugin is carefully designed to NOT interfere with normal operation:

1. **MQTT Commands**: Commands received via MQTT are processed through the standard command processor and responses are published to MQTT topics.

2. **Other Channel Commands**: Commands from USB, Serial, or other channels:
   - Are echoed to MQTT for monitoring
   - **Responses are captured and published to MQTT topics**
   - Responses ALSO go back to their originating channel (non-interfering)
   - Plugin returns `false` to allow normal flow

3. **No Response Interference**: The plugin captures responses from all channels and publishes them to MQTT, but NEVER prevents responses from being sent back to their originating channels. Commands from USB/Serial still receive their responses normally.

### Connection Management

The plugin implements robust connection management:

1. **Startup**: Attempts to connect to broker at initialization
2. **Periodic Reconnection**: If disconnected, attempts reconnection every 5 seconds
3. **Connection Loss Detection**: Detects when connection is lost and tracks downtime
4. **Reconnection Notification**: Publishes message to echo topic when connection restored, including downtime duration
5. **Last Will**: Uses MQTT Last Will and Testament for automatic offline notification on ungraceful disconnect
6. **Reduced Logging**: Only logs first connection attempt and successful connections to avoid log spam

### Quality of Service

All messages use QoS 1 (at-least-once delivery):
- **Commands subscribed** with QoS 1: Guaranteed to receive commands even with brief network interruptions
- **Responses published** with QoS 1: Responses guaranteed to reach broker
- **Status messages** with QoS 1: Status updates reliably delivered
- **Echo messages** with QoS 1: All command/response logs reliably captured

This provides a good balance between reliability and performance for telescope control applications.

### Platform Abstraction

The plugin uses compile-time detection to support both OnStepX and OCS automatically:

- **Platform Detection**: Automatically detects platform based on distinctive configuration defines:
  - OCS detected via `WATCHDOG` define
  - OnStepX detected via `STATUS_LED` define
  - Compile error if neither found

- **Method Naming**: Adapts to platform requirements:
  - **OCS**: Uses `command()` method
  - **OnStepX**: Uses `commandProcessing()` method

- **Controller Selection**: Uses correct controller object:
  - **OCS**: `observatory` controller
  - **OnStepX**: `onStep` controller

Platform detection is automatic - no user configuration needed. The plugin will fail compilation with a clear error if the platform cannot be detected.

## Security Considerations

**IMPORTANT**: This plugin stores MQTT credentials in **plain text** in the `Mqtt.h` file.

### Observatory Network Security

Astronomical equipment requires careful security management, separate from general domestic infrastructure:

1. **Dedicated Network Segment**: Use a separate VLAN or physical network for observatory equipment
2. **Access Control**: Implement strict firewall rules limiting MQTT broker access
3. **Authentication**: Use strong, unique passwords specific to observatory systems
4. **Monitoring**: Log all MQTT traffic for security auditing
5. **Physical Security**: Ensure MQTT broker is on observatory-controlled hardware
6. **Separation**: Keep observatory automation separate from home automation systems to reduce risk exposure

### Additional Security Measures

For enhanced security:
- Enable MQTT SSL/TLS (requires modifying the plugin to use WiFiClientSecure or similar)
- Use MQTT ACLs to restrict topic access by client
- Regular password rotation
- Network intrusion detection
- Keep firmware source code secure
- Consider VPN access for remote operations

## Integration Examples

### Observatory Automation

MQTT enables integration of telescope mount (OnStepX) and observatory infrastructure (OCS):

**Coordinated Operations:**
- Monitor mount tracking status before opening roof
- Coordinate dome/roof position with telescope pointing
- Trigger imaging sequences based on equipment readiness
- Automated shutdown sequences based on weather conditions

**Remote Monitoring:**
- Monitor telescope position and tracking status
- Check observatory environmental conditions (weather, power, safety)
- Receive equipment status updates
- Log all operations for analysis

**Multi-System Coordination:**
- Coordinate OnStepX mount with OCS observatory control
- Synchronize camera systems via MQTT commands
- Weather-based automation workflows
- Equipment health monitoring and alerting

### MQTT-Based Observatory Control Example

```python
import paho.mqtt.client as mqtt

class ObservatoryController:
    def __init__(self, broker="192.168.1.100"):
        self.client = mqtt.Client()
        self.client.username_pw_set("onstep", "password")
        self.client.on_message = self.on_message
        self.client.connect(broker, 1883, 60)
        
        # Subscribe to all responses and status
        self.client.subscribe("+/response")
        self.client.subscribe("+/status")
        self.client.subscribe("+/echo")
        
    def on_message(self, client, userdata, message):
        device = message.topic.split('/')[0]
        topic_type = message.topic.split('/')[1]
        print(f"[{device}] {topic_type}: {message.payload.decode()}")
    
    def send_command(self, device, command):
        """Send command to specific device"""
        self.client.publish(f"{device}/cmd", command)
    
    def check_safety(self):
        """OCS: Check safety status"""
        self.send_command("Observatory", ":Gs#")
    
    def get_mount_position(self):
        """OnStepX: Get RA/Dec"""
        self.send_command("Telescope", ":GR#")
        self.send_command("Telescope", ":GD#")
    
    def start_tracking(self):
        """OnStepX: Enable tracking"""
        self.send_command("Telescope", ":Te#")
    
    def coordinated_startup(self):
        """Example: Coordinated startup sequence"""
        print("Starting coordinated observatory startup...")
        self.check_safety()
        # Wait for safety check response before proceeding
        # In production, parse responses in on_message
        self.send_command("Observatory", ":RO#")  # Open roof
        self.send_command("Telescope", ":Te#")     # Start tracking
        self.send_command("Auxiliary", ":U1#")     # Power on auxiliary

# Usage
controller = ObservatoryController()
controller.coordinated_startup()
controller.client.loop_forever()
```

## Troubleshooting

### Plugin not connecting to MQTT broker

1. Check that network (WiFi or Ethernet) is configured and connected
2. Verify MQTT broker IP address and port are correct
3. Check MQTT username and password
4. Ensure MQTT broker is running and accessible
5. Check Arduino Serial Monitor for error messages
6. For Ethernet: Verify cable connection and link status
7. **For multiple controllers**: Ensure each has a unique MQTT_DEVICE_ID or HOST_NAME

### Commands not being processed

1. Verify MQTT topic names match configuration
2. Check QoS settings
3. Ensure commands are in correct OnStepX format (`:CMD#`)
4. Monitor the serial output for error messages
5. Check that PubSubClient library is installed correctly

### Connection keeps dropping

1. Adjust `MQTT_KEEPALIVE` value (increase for unstable networks)
2. Check network stability
3. Verify MQTT broker capacity and load
4. Check for firewall issues
5. Reduce `MQTT_QOS` to 0 if experiencing issues (not recommended - see QoS documentation)
6. Monitor echo topic for reconnection messages indicating frequency of disconnects

### Commands from USB not working after installing plugin

This should NOT happen - the plugin is designed to be non-interfering. If you experience this:
1. Verify `PLUGIN1_COMMAND_PROCESSING` is set to ON
2. Check that the plugin returns `false` from its command processing method
3. Check for compile errors or warnings
4. Verify platform detection is working (check serial output during initialization)
5. Report as a bug

### Multiple controllers won't connect simultaneously

If multiple controllers can't connect:
1. Verify each has a unique MQTT_DEVICE_ID (or unique HOST_NAME)
2. Check MQTT broker logs for connection conflicts or client ID collisions
3. Ensure broker allows multiple simultaneous connections
4. Verify network isn't blocking multiple connections from same subnet
5. Check broker's maximum client limit hasn't been reached

### Not receiving reconnection messages

If you don't see reconnection messages in the echo topic:
1. Verify you're subscribed to the echo topic
2. Check that QoS settings are appropriate (should be QoS 1)
3. Connection may be very stable (reconnection messages only appear after disconnect/reconnect)
4. Check serial monitor logs for "Connection lost" and "Connection restored" messages

### Compile error about MQTT_DEVICE_ID

If you see an error during compilation about MQTT_DEVICE_ID:

```
MQTT Plugin: MQTT_DEVICE_ID must be defined (should default to HOST_NAME)
```

**Cause**: HOST_NAME is not defined in your main Config.h file.

**Fix**: Ensure HOST_NAME is defined in your Config.h:
```cpp
#define HOST_NAME "Telescope"
```

Or override MQTT_DEVICE_ID in Mqtt.h:
```cpp
#define MQTT_DEVICE_ID_OVERRIDE
#define MQTT_DEVICE_ID "MyDevice"
```

### Compile error about platform detection

If you see an error like:

```
MQTT Plugin: Cannot detect platform. Neither WATCHDOG (OCS) nor STATUS_LED (OnStepX) found.
```

**Cause**: The plugin cannot determine if you're using OnStepX or OCS.

**Fix**: Ensure you're including the plugin in Plugins.config.h AFTER the main configuration is loaded. The plugin should be included from the main project, not standalone.

### Runtime error about invalid MQTT_DEVICE_ID characters

The plugin validates MQTT_DEVICE_ID at startup. If you see an error like:

```
ERR: MQTT Plugin, MQTT_DEVICE_ID contains invalid character
```

**Fix**: Edit the MQTT_DEVICE_ID (or HOST_NAME) to use only:
- Letters: A-Z, a-z
- Numbers: 0-9  
- Underscore: _
- Hyphen: -

**Invalid examples:**
- `"My Telescope"` (contains space)
- `"Obs/Dome"` (contains slash)
- `"Device+1"` (contains plus)

**Valid examples:**
- `"Telescope"`
- `"Observatory_Main"`
- `"Device-1"`

## Common Commands

### OnStepX Commands

Here are some useful OnStepX commands for telescope control:

- `:GVP#` - Get firmware version
- `:GR#` - Get Right Ascension
- `:GD#` - Get Declination
- `:Te#` - Enable tracking
- `:Td#` - Disable tracking
- `:Gg#` - Get current site longitude
- `:Gt#` - Get current site latitude
- `:GZ#` - Get azimuth
- `:GA#` - Get altitude

For a complete command reference, see the OnStep documentation at the [OnStep Groups.io Wiki](https://onstep.groups.io/g/main/wiki).

### OCS Commands

Here are some useful OCS commands for observatory control:

- `:IN#` - Get version number
- `:IP#` - Get product name
- `:Gs#` - Get safety status (SAFE/UNSAFE)
- `:GP#` - Get power status (OK/OUT/N/A)
- `:GAn#` - Get analog sensor n (0-F)
- `:GSn#` - Get digital sense n (1-6, returns ON/OFF)
- `:SU[date,time]#` - Set UTC date and time

For complete OCS command documentation, refer to the OCS manual and source code.

## Dependencies

This plugin requires the following library:

- **PubSubClient** (by Nick O'Leary) - Install via Arduino Library Manager

Platform-specific libraries (already included with Arduino):
- **WiFi** (ESP32) or **ESP8266WiFi** (ESP8266)
- **NativeEthernet** (Teensy) or **Ethernet** (other platforms)

## License

This plugin follows the same GPL-3.0 license as OnStepX.

## Support

For questions and support:
- OnStep Groups.io: https://groups.io/g/onstep
- OnStepX GitHub: https://github.com/hjd1964/OnStepX
- OCS GitHub: https://github.com/hjd1964/OCS
- OnStepX-Plugins GitHub: https://github.com/hjd1964/OnStepX-Plugins

## Version History

- v2.3 - Improved compile-time validation
  - Automatic platform detection via WATCHDOG (OCS) and STATUS_LED (OnStepX)
  - Compile-time validation of platform detection
  - Hybrid device ID validation (compile-time basic checks + runtime character validation)
  - Clear compile-time error messages for configuration issues
  - Added platform detection logging during initialization
  
- v2.2 - Connection management and QoS improvements
  - QoS 1 (at-least-once delivery) now used for all published messages
  - Automatic reconnection with downtime tracking
  - Reconnection notifications published to echo topic with timing information
  - Message loss warning on connection restoration
  - Reduced logging verbosity (first attempt and success only)
  - Last Will and Testament for automatic offline status on ungraceful disconnect
  
- v2.1 - Multi-device support
  - Added MQTT_DEVICE_ID configuration using HOST_NAME as default
  - Compile-time validation of device ID characters
  - Support for multiple controllers on same MQTT broker
  - Dynamic topic construction based on device ID
  - Clear error messages for invalid device identifiers
  
- v2.0 - Dual platform support
  - Added OCS compatibility
  - Automatic platform detection
  - Conditional method naming (command() for OCS, commandProcessing() for OnStepX)
  - Updated documentation for both platforms
  
- v1.1 - Platform abstraction update
  - Platform-independent network client support
  - Support for WiFi (ESP32/ESP8266) and Ethernet (Teensy, Due, etc.)
  - Fixed response handling to not interfere with other channels
  - Proper command processing integration
  
- v1.0 - Initial release
  - Bi-directional MQTT bridge
  - Command broadcasting from all channels
  - Auto-reconnection
  - Status monitoring

## Known Limitations

1. **SSL/TLS**: Not currently supported. Plain MQTT only.
2. **Authentication**: Username/password stored in plain text
3. **Response Timing**: Commands from other channels only show command echo, not response (by design)
4. **Network Change**: Requires restart if network configuration changes
5. **Single Broker**: Only one MQTT broker connection supported

## Future Enhancements

Potential improvements for future versions:
- SSL/TLS support for encrypted connections
- Secure credential storage
- Multiple broker support
- Discovery protocol support (Home Assistant MQTT Discovery)
- Retained state publishing
