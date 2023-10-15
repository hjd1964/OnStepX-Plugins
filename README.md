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

Its settings are in /guideRateRheostat/Config.h and are as follows:

#define RHEOSTAT_PIN OFF                       // default disabled, change to pin# for rheostat analog input
#define RHEOSTAT_CHANGE_THRESHOLD 5            // default >= 5% change before setting a new rate
#define RHEOSTAT_RATE_MINIMUM 1.0              // default slowest guide rate is 1.0x the sidereal rate
#define RHEOSTAT_RATE_MAXIMUM 0.5              // default fastest guide rate is 0.5x the goto rate
#define RHEOSTAT_EXPONENTIAL 5                 // higher exponentials provide finer adjustments of low guide rates
#define RHEOSTAT_R1 10000                      // resistance in Ohms, fixed value part of voltage divider
#define RHEOSTAT_R2 10000                      // resistance in Ohms, maximum value of potentiometer
