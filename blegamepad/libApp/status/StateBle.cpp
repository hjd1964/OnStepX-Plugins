// scan OnStep state in the background
#include "StateBle.h"

#include "../../../../lib/tasks/OnTask.h"
#include "../cmd/CmdBle.h"

void StateBle::init()
{
  statusBle.update();
}

void StateBle::poll()
{
  if ((long)(millis() - lastPoll) < STATE_POLLING_RATE_MS) return;
  lastPoll = millis();

  statusBle.update();

  if (statusBle.focuserFound == SD_TRUE && millis() - lastFocuserPageLoadTime < 2000) {
    char temp[80];
    if (!onStepBle.command(":FG#", temp)) strcpy(temp, "?"); else strcat(temp, " microns"); delay(0);
    strncpy(focuserPositionStr, temp, 20); focuserPositionStr[19] = 0; delay(0);
  }
}

StateBle stateBle;
