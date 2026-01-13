// ======================================================================================
// MQTT Plugin for OnStepX and OCS - Implementation
// ======================================================================================

#include "Mqtt.h"

// Compile-time warning if using default HOST_NAME
// This message appears here (in .cpp) so it only shows once during compilation
#if defined(HOST_NAME) && !defined(MQTT_DEVICE_ID_OVERRIDE)
  #pragma message "MQTT Plugin: Using HOST_NAME for device ID. Ensure HOST_NAME contains only letters, numbers, underscore (_), and hyphen (-). Spaces and special characters will cause runtime error."
#endif

Mqtt mqtt;
Mqtt* Mqtt::instance = nullptr;

// Include platform-specific controller headers
#if defined(WATCHDOG)
  #include "../../observatory/Observatory.h"
#else
  #include "../../telescope/Telescope.h"
#endif

bool Mqtt::validateDeviceId(const char* deviceId) {
  if (!deviceId || deviceId[0] == '\0') {
    DLF("ERR: MQTT Plugin, MQTT_DEVICE_ID cannot be empty");
    return false;
  }
  
  // Validate characters: A-Z, a-z, 0-9, underscore, hyphen
  for (int i = 0; deviceId[i] != '\0' && i < 64; i++) {
    char c = deviceId[i];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
          (c >= '0' && c <= '9') || c == '_' || c == '-')) {
      DF("ERR: MQTT Plugin, MQTT_DEVICE_ID contains invalid character '");
      D(c);
      DLF("' at position ");
      DL(i);
      DLF("ERR: MQTT Plugin, Only letters (A-Z, a-z), numbers (0-9), underscore (_), and hyphen (-) are allowed");
      return false;
    }
  }
  
  return true;
}

void Mqtt::init() {
  if (initialized) return;
  
  VLF("MSG: MQTT Plugin, initializing");
  
  if (!validateDeviceId(MQTT_TOSTRING(MQTT_DEVICE_ID))) {
    DLF("ERR: MQTT Plugin, initialization failed due to invalid MQTT_DEVICE_ID");
    DLF("ERR: MQTT Plugin, check HOST_NAME or override MQTT_DEVICE_ID in Mqtt.h");
    return;
  }
  
  instance = this;
  lastReconnectAttempt = 0;
  lastPollTime = 0;
  networkAvailable = false;
  processingMqttCommand = false;
  wasConnected = false;
  disconnectedSince = 0;
  firstConnectAttempt = true;
  
  // Use strncpy for safe string copying with bounds checking
  strncpy(clientId, MQTT_CLIENT_ID, sizeof(clientId) - 1);
  clientId[sizeof(clientId) - 1] = '\0';
  
  strncpy(topicCommand, MQTT_TOPIC_COMMAND, sizeof(topicCommand) - 1);
  topicCommand[sizeof(topicCommand) - 1] = '\0';
  
  strncpy(topicResponse, MQTT_TOPIC_RESPONSE, sizeof(topicResponse) - 1);
  topicResponse[sizeof(topicResponse) - 1] = '\0';
  
  strncpy(topicEcho, MQTT_TOPIC_ECHO, sizeof(topicEcho) - 1);
  topicEcho[sizeof(topicEcho) - 1] = '\0';
  
  strncpy(topicStatus, MQTT_TOPIC_STATUS, sizeof(topicStatus) - 1);
  topicStatus[sizeof(topicStatus) - 1] = '\0';
  
  mqttClient.setClient(netClient);
  mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
  mqttClient.setCallback(Mqtt::callback);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE);
  mqttClient.setBufferSize(MQTT_MSG_BUFFER_SIZE);
  
  VLF("MSG: MQTT Plugin, configured");
  VF("MSG: MQTT Plugin, platform: "); VL(MQTT_PLATFORM);
  VF("MSG: MQTT Plugin, device ID: "); VL(MQTT_TOSTRING(MQTT_DEVICE_ID));
  VF("MSG: MQTT Plugin, broker: "); V(MQTT_BROKER_HOST); V(":"); VL(MQTT_BROKER_PORT);
  VF("MSG: MQTT Plugin, client ID: "); VL(clientId);
  VF("MSG: MQTT Plugin, command topic: "); VL(topicCommand);
  VF("MSG: MQTT Plugin, response topic: "); VL(topicResponse);
  
  checkNetwork();
  
  if (networkAvailable) {
    connect();
  }
  
  initialized = true;
  VLF("MSG: MQTT Plugin, initialization complete");
}

void Mqtt::poll() {
  if (!initialized) return;
  
  unsigned long now = millis();
  if (now - lastPollTime < 100) return;
  lastPollTime = now;
  
  static unsigned long lastNetworkCheck = 0;
  if (now - lastNetworkCheck > 10000) {
    checkNetwork();
    lastNetworkCheck = now;
  }
  
  if (!networkAvailable) return;
  
  if (!mqttClient.connected()) {
    if (wasConnected) {
      wasConnected = false;
      disconnectedSince = now;
      VLF("WRN: MQTT Plugin, connection lost");
    }
    
    if (now - lastReconnectAttempt > MQTT_RECONNECT_DELAY) {
      lastReconnectAttempt = now;
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    if (!wasConnected) {
      wasConnected = true;
    }
    mqttClient.loop();
  }
}

// Check network availability using manager state and link status
void Mqtt::checkNetwork() {
  #if OPERATIONAL_MODE == WIFI
    // Use wifiManager state and WiFi connection status
    networkAvailable = wifiManager.active && (WiFi.status() == WL_CONNECTED);
    if (!networkAvailable && wasConnected) {
      VLF("WRN: MQTT Plugin, WiFi not connected");
    }
  #elif OPERATIONAL_MODE >= ETHERNET_FIRST && OPERATIONAL_MODE <= ETHERNET_LAST
    // Use ethernetManager state and Ethernet link status
    networkAvailable = ethernetManager.active && (Ethernet.linkStatus() == LinkON);
    if (!networkAvailable && wasConnected) {
      VLF("WRN: MQTT Plugin, Ethernet not connected");
    }
  #endif
}

void Mqtt::connect() {
  VLF("MSG: MQTT Plugin, attempting connection");
  
  if (!networkAvailable) {
    VLF("WRN: MQTT Plugin, network not available");
    return;
  }
  
  reconnect();
}

bool Mqtt::reconnect() {
  if (mqttClient.connected()) return true;
  
  if (firstConnectAttempt) {
    VLF("MSG: MQTT Plugin, connecting to broker");
    firstConnectAttempt = false;
  }
  
  bool connected = false;
  
  if (strlen(MQTT_USERNAME) > 0 && strlen(MQTT_PASSWORD) > 0) {
    connected = mqttClient.connect(clientId, MQTT_USERNAME, MQTT_PASSWORD,
                                   topicStatus, MQTT_QOS, true, "offline");
  } else {
    connected = mqttClient.connect(clientId, topicStatus, MQTT_QOS, true, "offline");
  }
  
  if (connected) {
    VLF("MSG: MQTT Plugin, connected to broker");
    
    if (disconnectedSince > 0) {
      unsigned long downtime = millis() - disconnectedSince;
      char msg[MQTT_MSG_BUFFER_SIZE];
      snprintf(msg, sizeof(msg), "Connection restored after %lu seconds - commands and responses during this period were not published to MQTT", downtime / 1000);
      publishMessage(topicEcho, msg);
      disconnectedSince = 0;
    }
    
    publishMessage(topicStatus, "online", true);
    
    if (mqttClient.subscribe(topicCommand, MQTT_QOS)) {
      VF("MSG: MQTT Plugin, subscribed to "); VL(topicCommand);
    } else {
      VLF("ERR: MQTT Plugin, subscription failed");
    }
    
    firstConnectAttempt = true;
    return true;
  } else {
    VF("ERR: MQTT Plugin, connection failed, rc="); VL(mqttClient.state());
    return false;
  }
}

void Mqtt::callback(char* topic, byte* payload, unsigned int length) {
  if (instance == nullptr) return;
  
  if (length >= MQTT_CMD_BUFFER_SIZE) {
    length = MQTT_CMD_BUFFER_SIZE - 1;
  }
  
  memcpy(instance->cmdBuffer, payload, length);
  instance->cmdBuffer[length] = '\0';
  
  VF("MSG: MQTT Plugin, received: "); VL(instance->cmdBuffer);
  
  instance->processCommand(instance->cmdBuffer);
}

void Mqtt::buildCommandString(char* dest, size_t destSize, const char* command, const char* parameter) {
  snprintf(dest, destSize, "%s%s", command, parameter ? parameter : "");
}

void Mqtt::processCommand(const char* command) {
  processingMqttCommand = true;
  
  responseBuffer[0] = '\0';
  
  char cmdCopy[MQTT_CMD_BUFFER_SIZE];
  strncpy(cmdCopy, command, MQTT_CMD_BUFFER_SIZE - 1);
  cmdCopy[MQTT_CMD_BUFFER_SIZE - 1] = '\0';
  
  // Parse command into command and parameter parts
  // OnStep commands are format :CCCppp# where CCC is command, ppp is parameter
  char cmd[MQTT_CMD_BUFFER_SIZE] = "";
  char param[MQTT_CMD_BUFFER_SIZE] = "";
  
  // Skip leading colon if present
  char* start = cmdCopy;
  if (start[0] == ':') start++;
  
  // Find the # terminator and null it
  char* end = strchr(start, '#');
  if (end) *end = '\0';
  
  // Split at first non-letter character for parameter
  int i = 0;
  while (i < MQTT_CMD_BUFFER_SIZE - 1 && start[i] && 
         ((start[i] >= 'A' && start[i] <= 'Z') || (start[i] >= 'a' && start[i] <= 'z'))) {
    cmd[i] = start[i];
    i++;
  }
  cmd[i] = '\0';
  
  // Rest is parameter
  if (start[i]) {
    strcpy(param, &start[i]);
  }
  
  CommandError cmdError = CE_NONE;
  bool supressFrame = false;
  bool numericReply = true;
  bool success = false;
  
  #if defined(WATCHDOG)
    success = observatory.command(responseBuffer, cmd, param, &supressFrame, &numericReply, &cmdError);
  #else
    success = telescope.command(responseBuffer, cmd, param, &supressFrame, &numericReply, &cmdError);
  #endif
  
  if (success || cmdError == CE_NONE) {
    publishCommandEcho(command, responseBuffer, "MQTT");
  } else {
    snprintf(responseBuffer, sizeof(responseBuffer), "ERROR: %d", (int)cmdError);
    publishCommandEcho(command, responseBuffer, "MQTT");
  }
  
  processingMqttCommand = false;
}

void Mqtt::publishMessage(const char* topic, const char* message, bool retain) {
  if (!mqttClient.connected()) return;
  
  mqttClient.publish(topic, (uint8_t*)message, strlen(message), retain);
}

void Mqtt::publishCommandEcho(const char* command, const char* response, const char* source) {
  if (!mqttClient.connected()) return;
  
  char message[MQTT_MSG_BUFFER_SIZE];
  
  // Use snprintf with precision specifiers to safely truncate long strings
  // Format string adds ~37 chars: "Received: ", ", Response: ", ", Source: "
  // Limit command to 150 chars and response to 250 chars for safety
  snprintf(message, sizeof(message), "Received: %.150s, Response: %.250s, Source: %s", 
           command, response, source);
  
  publishMessage(topicEcho, message);
  publishMessage(topicResponse, response);
  
  VF("MSG: MQTT Plugin, published echo: "); VL(message);
}

#if defined(WATCHDOG)
bool Mqtt::command(char reply[], char command[], char parameter[], bool *supressFrame, bool *numericReply, CommandError *commandError) {
  return processCommandChannel(reply, command, parameter, supressFrame, numericReply, commandError);
}
#else
bool Mqtt::commandProcessing(char *reply, char *command, char *parameter, bool *supressFrame, bool *numericReply, CommandError *commandError) {
  return processCommandChannel(reply, command, parameter, supressFrame, numericReply, commandError);
}
#endif

bool Mqtt::processCommandChannel(char *reply, char *command, char *parameter, bool *supressFrame, bool *numericReply, CommandError *commandError) {
  // Broadcast commands from other channels to MQTT while allowing normal processing
  
  if (!initialized || !mqttClient.connected()) return false;
  
  if (processingMqttCommand) return false;
  
  char fullCommand[MQTT_CMD_BUFFER_SIZE];
  buildCommandString(fullCommand, sizeof(fullCommand), command, parameter);
  
  if (strlen(fullCommand) > 0) {
    const char* response = (reply != nullptr && strlen(reply) > 0) ? reply : "<no response>";
    
    publishCommandEcho(fullCommand, response, "OTHER");
    
    VF("MSG: MQTT Plugin, broadcast command from other channel: "); V(fullCommand);
    VF(", Response: "); VL(response);
  }
  
  return false;
}
