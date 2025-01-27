// BLE gamepad plugin
#pragma once

class BleGamepad {
public:
  void init();
  void loop();

private:
  void bleInit();
  void bleSetup();
  void bleTimers();
  void bleConnTest();

};

extern BleGamepad blegamepad;
