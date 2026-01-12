// ======================================================================================
// MQTT Plugin for OnStepX and OCS
// Copyright (C) 2026
//
// This plugin creates a bi-directional bridge between MQTT and OnStep/Observatory
// commands. It receives MQTT messages in command format and publishes both the
// received command and the response. It also broadcasts commands received from
// other command channels.
// ======================================================================================

#pragma once

#include "../../Common.h"

// PubSubClient library is required for MQTT functionality
// Install via Arduino Library Manager: "PubSubClient" by Nick O'Leary
#include <PubSubClient.h>

// -------------------------------------------------------------------------------------------------
// PLATFORM DETECTION

#if defined(FirmwareName)
  #if FirmwareName == "OCS"
    #define MAIN_CONTROLLER observatory
  #elif FirmwareName == "OnStepX"
    #define MAIN_CONTROLLER onStep
  #endif
#endif

#ifndef MAIN_CONTROLLER
  #error "MQTT Plugin: Unable to detect platform. FirmwareName must be defined as 'OnStepX' or 'OCS'"
#endif

// -------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------
// PLUGIN CONFIGURATION

// Device identifier for MQTT topics - defaults to HOST_NAME if not overridden
// This allows multiple controllers to connect to the same MQTT broker with unique topics
// Override this if you need different MQTT topic names than your network hostname
#ifndef MQTT_DEVICE_ID
  #define MQTT_DEVICE_ID       HOST_NAME         //    HOST_NAME, Unique device identifier for MQTT topics. Option
#endif

// Validate MQTT_DEVICE_ID contains only allowed characters
// MQTT topics cannot contain: + # / spaces or other special characters
#define MQTT_VALID_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-"
#define MQTT_CHECK_CHAR_0  (MQTT_DEVICE_ID[0] != '\0')
#define MQTT_CHECK_CHAR_1  (!MQTT_DEVICE_ID[0] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[0]))
#define MQTT_CHECK_CHAR_2  (!MQTT_DEVICE_ID[1] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[1]))
#define MQTT_CHECK_CHAR_3  (!MQTT_DEVICE_ID[2] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[2]))
#define MQTT_CHECK_CHAR_4  (!MQTT_DEVICE_ID[3] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[3]))
#define MQTT_CHECK_CHAR_5  (!MQTT_DEVICE_ID[4] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[4]))
#define MQTT_CHECK_CHAR_6  (!MQTT_DEVICE_ID[5] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[5]))
#define MQTT_CHECK_CHAR_7  (!MQTT_DEVICE_ID[6] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[6]))
#define MQTT_CHECK_CHAR_8  (!MQTT_DEVICE_ID[7] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[7]))
#define MQTT_CHECK_CHAR_9  (!MQTT_DEVICE_ID[8] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[8]))
#define MQTT_CHECK_CHAR_10 (!MQTT_DEVICE_ID[9] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[9]))
#define MQTT_CHECK_CHAR_11 (!MQTT_DEVICE_ID[10] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[10]))
#define MQTT_CHECK_CHAR_12 (!MQTT_DEVICE_ID[11] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[11]))
#define MQTT_CHECK_CHAR_13 (!MQTT_DEVICE_ID[12] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[12]))
#define MQTT_CHECK_CHAR_14 (!MQTT_DEVICE_ID[13] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[13]))
#define MQTT_CHECK_CHAR_15 (!MQTT_DEVICE_ID[14] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[14]))
#define MQTT_CHECK_CHAR_16 (!MQTT_DEVICE_ID[15] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[15]))
#define MQTT_CHECK_CHAR_17 (!MQTT_DEVICE_ID[16] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[16]))
#define MQTT_CHECK_CHAR_18 (!MQTT_DEVICE_ID[17] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[17]))
#define MQTT_CHECK_CHAR_19 (!MQTT_DEVICE_ID[18] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[18]))
#define MQTT_CHECK_CHAR_20 (!MQTT_DEVICE_ID[19] || strchr(MQTT_VALID_CHARS, MQTT_DEVICE_ID[19]))

#if !MQTT_CHECK_CHAR_0
  #error "MQTT Plugin: MQTT_DEVICE_ID cannot be empty. Set MQTT_DEVICE_ID to a unique name using only letters, numbers, underscore, and hyphen (e.g., 'Telescope' or 'Observatory_Main')"
#elif !(MQTT_CHECK_CHAR_1 && MQTT_CHECK_CHAR_2 && MQTT_CHECK_CHAR_3 && MQTT_CHECK_CHAR_4 && MQTT_CHECK_CHAR_5 && \
        MQTT_CHECK_CHAR_6 && MQTT_CHECK_CHAR_7 && MQTT_CHECK_CHAR_8 && MQTT_CHECK_CHAR_9 && MQTT_CHECK_CHAR_10 && \
        MQTT_CHECK_CHAR_11 && MQTT_CHECK_CHAR_12 && MQTT_CHECK_CHAR_13 && MQTT_CHECK_CHAR_14 && MQTT_CHECK_CHAR_15 && \
        MQTT_CHECK_CHAR_16 && MQTT_CHECK_CHAR_17 && MQTT_CHECK_CHAR_18 && MQTT_CHECK_CHAR_19 && MQTT_CHECK_CHAR_20)
  #error "MQTT Plugin: MQTT_DEVICE_ID contains invalid characters. Only letters (A-Z, a-z), numbers (0-9), underscore (_), and hyphen (-) are allowed. Spaces, slashes, and special characters like + # / are not permitted. Current value is problematic - check HOST_NAME or override MQTT_DEVICE_ID in Mqtt.h"
#endif

// Helper macros for string concatenation in preprocessor
#define MQTT_STRINGIFY(x) #x
#define MQTT_TOSTRING(x) MQTT_STRINGIFY(x)
#define MQTT_CONCAT(a, b) MQTT_TOSTRING(a) "/" MQTT_TOSTRING(b)

#define MQTT_BROKER_HOST       "192.168.1.100"  //    "...", MQTT broker IP address or hostname.                     Option
#define MQTT_BROKER_PORT       1883              //    1883, MQTT broker port.                                        Option
#define MQTT_CLIENT_ID         MQTT_TOSTRING(MQTT_DEVICE_ID) //    MQTT_DEVICE_ID, MQTT client identifier.         Option

#define MQTT_USERNAME          "onstep"          //    "onstep", MQTT username for authentication (empty "" = none).  Option
#define MQTT_PASSWORD          "password"        //    "password", MQTT password for authentication (empty "" = none). Option

#define MQTT_TOPIC_COMMAND     MQTT_CONCAT(MQTT_DEVICE_ID, cmd)      //    MQTT_DEVICE_ID "/cmd", Topic to receive commands.       Option
#define MQTT_TOPIC_RESPONSE    MQTT_CONCAT(MQTT_DEVICE_ID, response) //    MQTT_DEVICE_ID "/response", Topic to publish responses. Option
#define MQTT_TOPIC_ECHO        MQTT_CONCAT(MQTT_DEVICE_ID, echo)     //    MQTT_DEVICE_ID "/echo", Topic to echo all commands.     Option
#define MQTT_TOPIC_STATUS      MQTT_CONCAT(MQTT_DEVICE_ID, status)   //    MQTT_DEVICE_ID "/status", Topic for online/offline.     Option

#define MQTT_RECONNECT_DELAY   5000              //    5000, Delay between reconnection attempts (milliseconds).      Option
#define MQTT_KEEPALIVE         60                //    60, MQTT keepalive interval (seconds).                         Option
#define MQTT_QOS               1                 //    0, 1, 2. MQTT Quality of Service level.                        Option
#define MQTT_RETAIN            false             //    true, false. Retain messages on broker.                        Option

#define MQTT_CMD_BUFFER_SIZE   256               //    256, Command buffer size (bytes).                              Option
#define MQTT_MSG_BUFFER_SIZE   512               //    512, Message buffer size (bytes).                              Option

// -------------------------------------------------------------------------------------------------

class Mqtt {
  public:
    void init();
    void poll();
    
    #if FirmwareName == "OCS"
      bool command(char reply[], char command[], char parameter[], bool *supressFrame, bool *numericReply, CommandError *commandError);
    #else
      bool commandProcessing(char *reply, char *command, char *parameter, bool *supressFrame, bool *numericReply, CommandError *commandError);
    #endif
    
  private:
    #ifdef ARDUINO_ARCH_ESP32
      #include <WiFi.h>
      WiFiClient netClient;
    #elif defined(ARDUINO_ARCH_ESP8266)
      #include <ESP8266WiFi.h>
      WiFiClient netClient;
    #elif defined(__arm__) && defined(TEENSYDUINO)
      #include <NativeEthernet.h>
      EthernetClient netClient;
    #elif defined(ARDUINO_SAM_DUE)
      #include <Ethernet.h>
      EthernetClient netClient;
    #else
      #include <Ethernet.h>
      EthernetClient netClient;
    #endif
    
    PubSubClient mqttClient;
    
    unsigned long lastReconnectAttempt;
    unsigned long lastPollTime;
    bool initialized;
    bool networkAvailable;
    char cmdBuffer[MQTT_CMD_BUFFER_SIZE];
    char msgBuffer[MQTT_MSG_BUFFER_SIZE];
    char responseBuffer[MQTT_MSG_BUFFER_SIZE];
    bool processingMqttCommand;
    
    char topicCommand[64];
    char topicResponse[64];
    char topicEcho[64];
    char topicStatus[64];
    char clientId[32];
    
    bool wasConnected;
    unsigned long disconnectedSince;
    bool firstConnectAttempt;
    
    void checkNetwork();
    void connect();
    bool reconnect();
    
    static void callback(char* topic, byte* payload, unsigned int length);
    static Mqtt* instance;
    
    void processCommand(const char* command);
    void publishMessage(const char* topic, const char* message, bool retain = MQTT_RETAIN);
    void publishCommandEcho(const char* command, const char* response, const char* source);
    void buildCommandString(char* dest, size_t destSize, const char* command, const char* parameter);
    bool processCommandChannel(char *reply, char *command, char *parameter, bool *supressFrame, bool *numericReply, CommandError *commandError);
};

extern Mqtt mqtt;
