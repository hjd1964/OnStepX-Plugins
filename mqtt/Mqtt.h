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

#include "../../lib/commands/CommandErrors.h"
#include "../../Common.h"

// PubSubClient library is required for MQTT functionality
// Install via Arduino Library Manager: "PubSubClient" by Nick O'Leary
#include <PubSubClient.h>

// -------------------------------------------------------------------------------------------------
// PLATFORM DETECTION

// Auto-detect platform based on distinctive defines
// OCS always has WATCHDOG defined, OnStepX always has STATUS_LED defined
#if defined(WATCHDOG)
  #define MQTT_PLATFORM "OCS"
#elif defined(STATUS_LED)
  #define MQTT_PLATFORM "OnStepX"
#else
  #error "MQTT Plugin: Cannot detect platform. Neither WATCHDOG (OCS) nor STATUS_LED (OnStepX) found. Ensure plugin is included after main configuration."
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

// Device ID will be validated at runtime for valid characters (A-Z, a-z, 0-9, _, -)

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

// Undefine PubSubClient's MQTT_KEEPALIVE and define our own
#ifdef MQTT_KEEPALIVE
  #undef MQTT_KEEPALIVE
#endif
#define MQTT_KEEPALIVE         60                //    60, MQTT keepalive interval (seconds).                         Option

#define MQTT_QOS               1                 //    0, 1, 2. MQTT Quality of Service level.                        Option
#define MQTT_RETAIN            false             //    true, false. Retain messages on broker.                        Option

#define MQTT_CMD_BUFFER_SIZE   256               //    256, Command buffer size (bytes). Used for cmdBuffer.          Option
#define MQTT_MSG_BUFFER_SIZE   512               //    512, Message buffer size (bytes). Used for responseBuffer.     Option

// -------------------------------------------------------------------------------------------------

class Mqtt {
  public:
    void init();
    void poll();
    
    #if defined(WATCHDOG)
      bool command(char reply[], char command[], char parameter[], bool *supressFrame, bool *numericReply, CommandError *commandError);
    #else
      bool commandProcessing(char *reply, char *command, char *parameter, bool *supressFrame, bool *numericReply, CommandError *commandError);
    #endif
    
  private:
    // Network client - type determined by OPERATIONAL_MODE
    // WiFi/Ethernet libraries are already included via Common.h -> Manager headers
    #if OPERATIONAL_MODE == WIFI
      WiFiClient netClient;
    #elif OPERATIONAL_MODE >= ETHERNET_FIRST && OPERATIONAL_MODE <= ETHERNET_LAST
      EthernetClient netClient;
    #else
      #error "MQTT Plugin: OPERATIONAL_MODE must be set to WIFI or an Ethernet mode"
    #endif
    
    PubSubClient mqttClient;
    
    unsigned long lastReconnectAttempt;
    unsigned long lastPollTime;
    bool initialized;
    bool networkAvailable;
    char cmdBuffer[MQTT_CMD_BUFFER_SIZE];
    char responseBuffer[MQTT_MSG_BUFFER_SIZE];
    bool processingMqttCommand;
    
    char topicCommand[80];
    char topicResponse[80];
    char topicEcho[80];
    char topicStatus[80];
    char clientId[64];
    
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
    bool validateDeviceId(const char* deviceId);
};

extern Mqtt mqtt;
