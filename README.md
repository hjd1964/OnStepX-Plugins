# OnStepX-Plugins

One plugin is currently available... the SWS Website ported to OnStepX.

This requires changing two lines in OnStepX's Extended.config.h file:
- #define SERIAL_IP_MODE WIFI_ACCESS_POINT
- #define WEB_SERVER ON

Which enables WiFi and the Webserver components of OnStepX.

You also must copy the /website directory into the OnStepX/src/plugins directory and add an entery for it in Plugins.config.h similar to the following:

- #define PLUGIN1 website 
- #include "website/Website.h"

OnStepX will then initialize and use it.

Additional settings are in /website/Config.h
