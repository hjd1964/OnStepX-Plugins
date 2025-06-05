# OnStepX-Plugins

## Website

The SWS Website for ESP32's and WiFi only, ported to OnStepX.

**You must copy the /website directory into the OnStepX/src/plugins directory and add an entery for it in Plugins.config.h similar to the following:**
- #define PLUGIN1 website 
- #include "website/Website.h"

**This plugin requires that WiFi be activated in OnStepX's Config.h file, for example:**
- #define SERIAL_RADIO WIFI_ACCESS_POINT

Which enables WiFi and the Webserver components of OnStepX.

OnStepX will then initialize and use it.

Additional settings are in /website/Config.h

## Guide Rate Rheostat

You must copy the /guideRateRheostat directory into the OnStepX/src/plugins directory and add an entery for it in Plugins.config.h similar to the following:

- #define PLUGIN1 guideRateRheostat
- #include "guideRateRheostat/GuideRateRheostat.h"

The guide rate rheostat allows using an knob, on a basic hand controller for example, to adjust the guide rate.

Its settings are in /guideRateRheostat/Config.h and you need to set the pin to be used for analog input, the values to describe the rheostat voltage divider, etc.


## Metrics

Prometheus compatible metrics page for OnStepX.

This can be useful both for collecting statistics using Prometheus, or for debugging purposes.

This plugin depends on having the Website plugin enabled.

You must copy the /metrics directory into the OnStepX/src/plugins directory and add an entery for it in Plugins.config.h similar to the following:

```
#define PLUGIN2                       metricsPlugin // Assuming PLUGIN1 will be Website
#include "metrics/MetricsPlugin.h"
#endif
```

Once enabled, you can head to the path defined by `METRICS_PLUGIN_PATH` (by default `/metrics`) to see your metrics. This is also the path you'll want to use if you want to scrape these metrics using Prometheus.

### Adding metrics

An example implementation on how to add custom metrics is available with GPS metrics:
 - in `GPS.cpp`, add the following includes:
 ```
#include "../../../Common.h"
#include "../../../plugins/Plugins.config.h"
```
 - Also in `GPS.cpp`, inside the `init` method (towards the end), add the following snippet of code to initialise the GPS metrics:
 ```
  #ifdef HAS_METRICS_PLUGIN
  metricsPlugin.initGpsMetrics(gps);
  #endif
```
 - Now you can look at `initGpsMetrics` to see how metrics are populated.