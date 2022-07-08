# OnStepX-Plugins

One plugin is currently available... the SWS Website ported to OnStepX.

This requires adding one line to Config.h (in addition to enabling the ESP32 ACCESS_POINT or STATION mode.)

#define WEB_SERVER ON

Which overrides the default disabled state.

You also must copy the /website directory into the src/OnStepX/plugins directory and add an entery for it in Plugins.config.h similar to the following:

#define PLUGIN1 website

#include "website/Website.h"

OnStepX will then initialize and use it.

Additional settings are in /website/Website.config.h
