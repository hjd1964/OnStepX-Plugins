// ElegantOTAPlugin plugin
#pragma once
#include "../../lib/commands/CommandErrors.h"
#include "../../lib/wifi/WifiManager.h"
#include <WebServer.h>

#if OPERATIONAL_MODE == WIFI && WEB_SERVER == ON
#define __ONSTEP_HAS_WEBSERVER
#endif

#ifndef ELEGANTOTA_PLUGIN_USERNAME
#define ELEGANTOTA_PLUGIN_USERNAME ""
#endif

#ifndef ELEGANTOTA_PLUGIN_PASSWORD
#define ELEGANTOTA_PLUGIN_PASSWORD ""
#endif

#ifndef ELEGANTOTA_PLUGIN_SSID
#define ELEGANTOTA_PLUGIN_SSID "OnStepX-OTA"
#endif

#ifndef ELEGANTOTA_PLUGIN_PSK
#define ELEGANTOTA_PLUGIN_PSK ""
#endif

class ElegantOTAPlugin {
public:
  void init();

  bool command(char reply[], char command[], char parameter[], bool *suppressFrame, bool *numericReply, CommandError *commandError);

private:
  void initialiseElegantOTA();
  #ifndef __ONSTEP_HAS_WEBSERVER
  bool initialiseWiFi();
  #endif
  bool started = false;
};

extern ElegantOTAPlugin elegantOTAPlugin;
