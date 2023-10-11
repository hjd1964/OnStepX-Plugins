// GuideRateRheostat plugin (experimental!) configuration file
#pragma once

#define RHEOSTAT_PIN OFF                       // default disabled, change to pin# for rheostat analog input
#define RHEOSTAT_OFF_THRESHOLD_VOLTS 3.2       // default >= 3.2V is considered disabled
#define RHEOSTAT_CHANGE_THRESHOLD_VOLTS 0.2    // default 0.2V change before setting a new rate
#define RHEOSTAT_RATE_RANGE 0.5                // default fastest guide rate is 0.5x the goto rate
