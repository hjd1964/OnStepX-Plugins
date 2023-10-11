// GuideRateRheostat plugin (experimental!)

#include "GuideRateRheostat.h"
#include "../../Common.h"
#include "../../lib/serial/Serial_Local.h"
#include "../../lib/tasks/OnTask.h"
#include "../../lib/convert/Convert.h"
#include "../../telescope/mount/goto/Goto.h"

void rheostatWrapper() { guideRateRheostat.loop(); }

void GuideRateRheostat::init() {
  VLF("MSG: Plugins, starting: GuideRateRheostat");

  // start a task that runs once a second, run at priority level 7 so
  tasks.add(1000, 0, true, 7, rheostatWrapper);
}

void GuideRateRheostat::loop() {
  #if RHEOSTAT_PIN != OFF
    float v = (analogRead(RHEOSTAT_PIN)/1023.0F)*3.3F;

    if (v < RHEOSTAT_OFF_THRESHOLD_VOLTS) {
      if (abs(v - lastVoltage) > RHEOSTAT_CHANGE_THRESHOLD_VOLTS) {
        char s[40];
        sprintF(s, ":RA%1.3f#", (v/RHEOSTAT_OFF_THRESHOLD_VOLTS)*radToDegF(goTo.rate)*RHEOSTAT_RATE_RANGE);
        SERIAL_LOCAL.transmit(s);
        sprintF(s, ":RE%1.3f#", (v/RHEOSTAT_OFF_THRESHOLD_VOLTS)*radToDegF(goTo.rate)*RHEOSTAT_RATE_RANGE);
        SERIAL_LOCAL.transmit(s);
        lastVoltage = v;
      }
    }
  #endif
}

GuideRateRheostat guideRateRheostat;
