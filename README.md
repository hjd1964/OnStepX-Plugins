# OnStepX-Plugins

## Website

You must copy the /website directory into the OnStepX/src/plugins directory and add an entery for it in Plugins.config.h similar to the following:

- #define PLUGIN1 website 
- #include "website/Website.h"

The SWS Website for ESP32's and WiFi only, ported to OnStepX.

This requires changing two lines in OnStepX's Extended.config.h file:
- #define SERIAL_IP_MODE WIFI_ACCESS_POINT
- #define WEB_SERVER ON

Which enables WiFi and the Webserver components of OnStepX.

OnStepX will then initialize and use it.

Additional settings are in /website/Config.h

## Guide Rate Rheostat

You must copy the /guideRateRheostat directory into the OnStepX/src/plugins directory and add an entery for it in Plugins.config.h similar to the following:

- #define PLUGIN1 guideRateRheostat
- #include "guideRateRheostat/GuideRateRheostat.h"

The guide rate rheostat allows using an knob, on a basic hand controller for example, to adjust the guide rate.

Its settings are in /guideRateRheostat/Config.h and you need to set the pin to be used for analog input, the values to describe the rheostat voltage divider, etc.
