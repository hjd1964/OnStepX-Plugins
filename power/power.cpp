// Power plugin

#include "Power.h"
#include "../../Common.h"
#include "../../lib/serial/Serial_Local.h"
#include "../../lib/tasks/OnTask.h"
#include "../../telescope/auxiliary/features.h"
#include "../../telescope/Telescope.h"

#ifndef ANALOG_READ_RANGE
  #define ANALOG_READ_RANGE 1023
#endif

float featureVoltage[8] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
float featureCurrent[8] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};

void powerWrapper() { power.loop(); }

void Power::init() {
  VLF("MSG: Plugins, starting: power monitor");

  // start a task that runs twice a second, run at priority level 7 as precise timing isn't very important
  tasks.add(200, 0, true, 7, powerWrapper);

  // get ready for fan control
  #if defined(FAN_PIN) && defined(FAN_THRESHOLD_LOW) && defined(FAN_POWER_LOW)
  //  pinMode(FAN_PIN, OUTPUT);
    fanMode = FM_OFF;
  #endif
}

void Power::loop() {
  float v = 0.0F;
  bool disableFeatures = false;

  #if defined(I_SENSE_CHANNEL_MAX) && (I_SENSE_COMBINED_MAX)
    float currentSenseTotal = 0;
  #endif

  #if defined(V_SENSE_PINS) && defined(V_SENSE_FORMULA)
    int voltageSensePin[8] = V_SENSE_PINS;
    for (int j = 0; j < 8; j++) {
      if (voltageSensePin[j] >= 0) {
        if (isnan(featureVoltage[j])) delay(10);

        #ifdef ESP32
          v = analogReadMilliVolts(voltageSensePin[j])/1000.0F;
        #else
          v = (analogRead(voltageSensePin[j])/(float)ANALOG_READ_RANGE)*HAL_VCC;
        #endif

        if (isnan(featureVoltage[j])) featureVoltage[j] = V_SENSE_FORMULA; else featureVoltage[j] = (featureVoltage[j]*6.0F + V_SENSE_FORMULA)/7.0F;

        #if defined(V_SENSE_LIMIT_LOW) && defined(V_SENSE_LIMIT_HIGH)
          if (j != V_SENSE_LIMIT_EXCLUDE && (featureVoltage[j] < V_SENSE_LIMIT_LOW || featureVoltage[j] > V_SENSE_LIMIT_HIGH)) {
            VLF("MSG: Power plugin, OV");
            disableFeatures = true;
          }
        #endif
      }
    }
  #endif

  Y;

  #if defined(I_SENSE_PINS) && defined(I_SENSE_FORMULA)
    int currentSensePin[8] = I_SENSE_PINS;
    for (int j = 0; j < 8; j++) {
      if (currentSensePin[j] >= 0) {
        if (isnanf(featureCurrent[j])) delay(10);

        #ifdef ESP32
          v = analogReadMilliVolts(currentSensePin[j])/1000.0F;
        #else
          v = (analogRead(currentSensePin[j])/(float)ANALOG_READ_RANGE)*HAL_VCC;
        #endif

        if (isnanf(featureCurrent[j])) featureCurrent[j] = I_SENSE_FORMULA; else featureCurrent[j] = (featureCurrent[j]*6.0F + I_SENSE_FORMULA)/7.0F;

        #if defined(I_SENSE_CHANNEL_MAX)
          float currentSenseMax[8] = I_SENSE_CHANNEL_MAX;
          if (fabs(featureCurrent[j]) > currentSenseMax[j]) {
            VF("MSG: Power plugin, OC channel"); V(j); VF(" current="); V(featureCurrent[j]); VLF("A");
            features.deviceOff(j);
          }

          #if defined(I_SENSE_COMBINED_MAX)
            currentSenseTotal += fabs(featureCurrent[j]);
            if (currentSenseTotal > I_SENSE_COMBINED_MAX) {
              VF("MSG: Power plugin, OC combined"); VF(" current="); V(currentSenseTotal); VLF("A");
              disableFeatures = true;
            }
          #endif
        #endif
      }
    }
  #endif

  Y;

  #if defined(FAN_PIN)
    FanMode lastFanMode = fanMode;

    if (!isnan(telescope.mcuTemperature) && (long)(millis() - nextFanModeChangeTime) > 0) {
      #if defined(FAN_THRESHOLD_OT)
        if (telescope.mcuTemperature > FAN_THRESHOLD_OT) {
          VLF("MSG: Power plugin, MCU OT");
          disableFeatures = true;
        }
      #endif
      #if defined(FAN_THRESHOLD_HIGH) && defined(FAN_POWER_HIGH)
        if (telescope.mcuTemperature > FAN_THRESHOLD_HIGH) {
          if (fanMode != FM_HIGH) {
            VLF("MSG: Power plugin, FAN high");
            analogWrite(FAN_PIN, ANALOG_WRITE_RANGE*(FAN_POWER_HIGH/100.0F));
            fanMode = FM_HIGH;
          }
        } else
      #endif
      #if defined(FAN_THRESHOLD_MID) && defined(FAN_POWER_MID)
        if (telescope.mcuTemperature > FAN_THRESHOLD_MID) {
          if (fanMode != FM_MID) {
            VLF("MSG: Power plugin, FAN mid");
            analogWrite(FAN_PIN, ANALOG_WRITE_RANGE*(FAN_POWER_MID/100.0F));
            fanMode = FM_MID;
          }
        } else
      #endif
      #if defined(FAN_THRESHOLD_LOW) && defined(FAN_POWER_LOW)
        if (telescope.mcuTemperature > FAN_THRESHOLD_LOW) {
          if (fanMode != FM_LOW) {
            VLF("MSG: Power plugin, FAN low");
            analogWrite(FAN_PIN, ANALOG_WRITE_RANGE*(FAN_POWER_LOW/100.0F));
            fanMode = FM_LOW;
          }
        } else
      #endif
      if (fanMode != FM_OFF) {
        VLF("MSG: Power plugin, FAN off");
        analogWrite(FAN_PIN, 0);
        fanMode = FM_OFF;
      }
    }

    if (lastFanMode != fanMode) nextFanModeChangeTime = millis() + 30000UL;
  #endif

  #if (defined(V_SENSE_PINS) && defined(V_SENSE_FORMULA)) || (defined(I_SENSE_PINS) && defined(I_SENSE_FORMULA))
    if (disableFeatures) {
      VLF("MSG: Power plugin, disable all features");
      for (int j = 0; j < 8; j++) features.deviceOff(j);
    }
  #endif

}

Power power;
