// GuideRateRheostat plugin (experimental!)
#pragma once

#include "Config.h"

class GuideRateRheostat {
public:
  // the initialization method must be present and named: void init();
  void init();

  void loop();

private:
  float lastVoltage = 3.3F;

};

extern GuideRateRheostat guideRateRheostat;
