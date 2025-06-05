// ElegantOTAPlugin plugin

#include <ElegantOTA.h>

#include "ElegantOTAPlugin.h"
#include "../../Common.h"
#include "../../lib/tasks/OnTask.h"

#include "../../lib/ethernet/webServer/WebServer.h"
#include "../../lib/wifi/webServer/WebServer.h"

WebServer *eOTAWebServer = nullptr;

void ElegantOTAPlugin::init() {
#ifdef __ONSTEP_HAS_WEBSERVER
  eOTAWebServer = &www;
  initialiseElegantOTA();
  started = true;
#else
  eOTAWebServer = new WebServer(80);
  eOTAWebServer->on("/", [this](){
    eOTAWebServer->sendHeader("Location", "/update");
    eOTAWebServer->send(302);
  });
  eOTAWebServer->onNotFound([this](){
    eOTAWebServer->send(404, "text/plain", "Not found");
  });
#endif
}

bool ElegantOTAPlugin::command(char reply[], char command[], char parameter[], bool *supressFrame, bool *numericReply, CommandError *commandError) {
#ifdef __ONSTEP_HAS_WEBSERVER
  return false;
#else
  if(command[0] == 'E' && command[1] == 'O' && parameter[0] == 'T' && parameter[1] == 'A') {
    *numericReply = false;
    if(started) {
      sprintf(reply, "ERR_ALREADY_STARTED");
    } else {
      if(!initialiseWiFi()) {
        sprintf(reply, "ERR_NO_WIFI");
      } else {
        eOTAWebServer->begin();
        initialiseElegantOTA();
        started = true;
        sprintf(reply, "ON");
      }
    }
    
    return true;
  }
  return false;
#endif
}

void ElegantOTAPlugin::initialiseElegantOTA() {
  VLF("MSG: Plugins, starting: elegantOTA");
  ElegantOTA.begin(eOTAWebServer, ELEGANTOTA_PLUGIN_USERNAME, ELEGANTOTA_PLUGIN_PASSWORD);

  tasks.add(100, 0, true, 7, [](){ ElegantOTA.loop();});
  #ifndef __ONSTEP_HAS_WEBSERVER
  tasks.add(10, 0, true, 7, [](){ eOTAWebServer->handleClient();});
  #endif
}

#ifndef __ONSTEP_HAS_WEBSERVER
bool ElegantOTAPlugin::initialiseWiFi() {
  WiFi.setHostname("Onstep-ElegantOTA");
  WiFi.mode(WIFI_AP);
  return WiFi.softAP(ELEGANTOTA_PLUGIN_SSID, ELEGANTOTA_PLUGIN_PSK);
}
#endif

ElegantOTAPlugin elegantOTAPlugin;
