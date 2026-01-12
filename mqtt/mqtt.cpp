// ======================================================================================
// MQTT Plugin for OnStepX - Implementation
// ======================================================================================

#include "Mqtt.h"

Mqtt mqtt;
Mqtt* Mqtt::instance = nullptr;

void Mqtt::init() {
  if (initialized) return;
  
  VLF("MSG: MQTT Plugin, initializing");
  
  instance = this;
  lastReconnectAttempt = 0;
  lastPollTime = 0;
  initialized = false;
  networkAvailable = false;
  processingMqttCommand = false;
  wasConnected = false;
  disconnectedSince = 0;
  firstConnectAttempt = true;
  
  strcpy(clientId, MQTT_CLIENT_ID);
  strcpy(topicCommand, MQTT_TOPIC_COMMAND);
  strcpy(topicResponse, MQTT_TOPIC_RESPONSE);
  strcpy(topicEcho, MQTT_TOPIC_ECHO);
  strcpy(topicStatus, MQTT_TOPIC_STATUS);
  
  mqttClient.setClient(netClient);
  mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
  mqttClient.setCallback(Mqtt::callback);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE);
  mqttClient.setBufferSize(MQTT_MSG_BUFFER_SIZE);
  
  VLF("MSG: MQTT Plugin, configured");
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

// Check network availability across WiFi and Ethernet platforms
void Mqtt::checkNetwork() {
  #if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    networkAvailable = (WiFi.status() == WL_CONNECTED);
    if (!networkAvailable) {
      VLF("WRN: MQTT Plugin, WiFi not connected");
    }
  #else
    networkAvailable = (Ethernet.hardwareStatus() != EthernetNoHardware) &&
                       (Ethernet.linkStatus() == LinkON);
    if (!networkAvailable) {
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
  
  CommandError cmdError = CE_NONE;
  bool success = MAIN_CONTROLLER.command(cmdCopy, responseBuffer, sizeof(responseBuffer), &cmdError);
  
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
  
  snprintf(message, sizeof(message), "Received: %s, Response: %s, Source: %s", 
           command, response, source);
  
  publishMessage(topicEcho, message);
  publishMessage(topicResponse, response);
  
  VF("MSG: MQTT Plugin, published echo: "); VL(message);
}

#if FirmwareName == "OCS"
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
