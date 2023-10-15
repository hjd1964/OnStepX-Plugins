// GuideRateRheostat plugin (experimental!) configuration file
#pragma once

#define RHEOSTAT_PIN OFF                       // default disabled, change to pin# for rheostat analog input
#define RHEOSTAT_CHANGE_THRESHOLD 5            // default >= 5% change before setting a new rate
#define RHEOSTAT_RATE_MINIMUM 1.0              // default slowest guide rate is 1.0x the sidereal rate
#define RHEOSTAT_RATE_MAXIMUM 0.5              // default fastest guide rate is 0.5x the goto rate
#define RHEOSTAT_EXPONENTIAL 5                 // higher exponentials provide finer adjustments of low guide rates
#define RHEOSTAT_R1 10000                      // resistance in Ohms, fixed value part of voltage divider
#define RHEOSTAT_R2 10000                      // resistance in Ohms, maximum value of potentiometer
