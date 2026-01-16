// ======================================================================================
// MQTT Plugin for OnStepX and OCS
// This plugin creates a bi-directional bridge between MQTT and OnStepX/OCS commands.
// It receives MQTT messages in command format and publishes both the received command 
// and the response. It also broadcasts commands received from other command channels.
// ======================================================================================

#pragma once

#include "../../lib/commands/CommandErrors.h"
#include "../../Common.h"

#if OPERATIONAL_MODE == WIFI
  #if defined(ESP32)
    #include <WiFi.h>
    #include <WiFiClient.h>
  #elif defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <WiFiClient.h>
  #endif
#elif OPERATIONAL_MODE == ETHERNET_W5100 || OPERATIONAL_MODE == ETHERNET_W5500
  #ifdef ESP8266
    #include <Ethernet2.h>
  #else
    #include <Ethernet.h>
  #endif
#endif

#include <PubSubClient.h>           // https://github.com/knolleary/pubsubclient


// -------------------------------------------------------------------------------------------------
// PLUGIN CONFIGURATION

// Device identifier for MQTT topics - defaults to HOST_NAME if not overridden
// This allows multiple controllers to connect to the same MQTT broker with unique topics
// Override this if you need different MQTT topic names than your network hostname
#ifndef MQTT_DEVICE_ID
  #define MQTT_DEVICE_ID       HOST_NAME         //    HOST_NAME, Unique device identifier for MQTT topics.             Option
#endif

// Compile-time check that MQTT_DEVICE_ID is not an empty string
// HOST_NAME is always a string literal like "OCS" or ""
#if defined(MQTT_DEVICE_ID)
  static_assert(sizeof(MQTT_DEVICE_ID) > 1, "MQTT_DEVICE_ID cannot be empty - set HOST_NAME in Config.h");
#endif

#define MQTT_BROKER_HOST       "192.168.1.100"   //    "...", MQTT broker IP address or hostname.                       Option
#define MQTT_BROKER_PORT       1883              //    1883, MQTT broker port.                                          Option
#define MQTT_CLIENT_ID         MQTT_DEVICE_ID    //    MQTT_DEVICE_ID, MQTT client identifier.                          Option

#define MQTT_USERNAME          "onstep"          //    "onstep", MQTT username for authentication (empty "" = none).    Option
#define MQTT_PASSWORD          "password"        //    "password", MQTT password for authentication (empty "" = none).  Option

// -------------------------------------------------------------------------------------------------

#define MQTT_TOPIC_COMMAND     MQTT_DEVICE_ID "/cmd"
#define MQTT_TOPIC_RESPONSE    MQTT_DEVICE_ID "/response"
#define MQTT_TOPIC_ECHO        MQTT_DEVICE_ID "/echo"
#define MQTT_TOPIC_STATUS      MQTT_DEVICE_ID "/status"

#define MQTT_RECONNECT_DELAY   5000

// Undefine PubSubClient's MQTT_KEEPALIVE and define our own
#ifdef MQTT_KEEPALIVE
  #undef MQTT_KEEPALIVE
#endif
#define MQTT_KEEPALIVE         60
#define MQTT_QOS               1
#define MQTT_RETAIN            false
#define MQTT_CMD_BUFFER_SIZE   256
#define MQTT_MSG_BUFFER_SIZE   512

// -------------------------------------------------------------------------------------------------

class Mqtt {
  public:
    void init();
    void poll();
    bool command(char reply[], char command[], char parameter[], bool *supressFrame, bool *numericReply, CommandError *commandError);
    
  private:
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
    //bool processCommandChannel(char *reply, char *command, char *parameter, bool *supressFrame, bool *numericReply, CommandError *commandError);
    bool validateDeviceId(const char* deviceId);
};

extern Mqtt mqtt;
