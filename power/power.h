// Sample plugin
#pragma once

enum FanMode {FM_OFF, FM_LOW, FM_MID, FM_HIGH};

class Power {
public:
  // the initialization method must be present and named: void init();
  void init();

  void loop();

private:
  FanMode fanMode;
  unsigned long nextFanModeChangeTime = 0;
};

extern Power power;
