// USB port switching plugin
// Acts similar to a Feature SWITCH type
// leaving all 8 Features available for other uses
// PLUGIN[n]_COMMAND_PROCESSING must be set ON in Plugins.config.h

#pragma once

#include "../../Common.h"
#include "../../lib/commands/CommandErrors.h"

  // Plugin config
  #define USB1_PIN                   OFF //    OFF, n. I/O pin controlling the port state.    Option
  #define USB1_NAME               "USB1" //    "USB..", Name of the device being controlled.  Option
  #define USB1_ON_STATE             HIGH //    HIGH, LOW Port control pin ON (active) state.  Option
  #define USB1_DEFAULT_STATE         OFF //    OFF, ON. State to startup in.                  Option
  #define USB2_PIN                   OFF //    OFF, n. I/O pin controlling the port state.    Option
  #define USB2_NAME               "USB2" //    "USB..", Name of the device being controlled.  Option
  #define USB2_ON_STATE             HIGH //    HIGH, LOW Port control pin ON (active) state.  Option
  #define USB2_DEFAULT_STATE         OFF //    OFF, ON. State to startup in.                  Option
  #define USB3_PIN                   OFF //    OFF, n. I/O pin controlling the port state.    Option
  #define USB3_NAME               "USB3" //    "USB..", Name of the device being controlled.  Option
  #define USB3_ON_STATE              LOW //    HIGH, LOW Port control pin ON (active) state.  Option
  #define USB3_DEFAULT_STATE         OFF //    OFF, ON. State to startup in.                  Option
  #define USB4_PIN                   OFF //    OFF, n. I/O pin controlling the port state.    Option
  #define USB4_NAME               "USB4" //    "USB..", Name of the device being controlled.  Option
  #define USB4_ON_STATE             HIGH //    HIGH, LOW Port control pin ON (active) state.  Option
  #define USB4_DEFAULT_STATE         OFF //    OFF, ON. State to startup in.                  Option
  #define USB5_PIN                   OFF //    OFF, n. I/O pin controlling the port state.    Option
  #define USB5_NAME               "USB5" //    "USB..", Name of the device being controlled.  Option
  #define USB5_ON_STATE             HIGH //    HIGH, LOW Port control pin ON (active) state.  Option
  #define USB5_DEFAULT_STATE         OFF //    OFF, ON. State to startup in.                  Option
  #define USB6_PIN                   OFF //    OFF, n. I/O pin controlling the port state.    Option
  #define USB6_NAME               "USB6" //    "USB..", Name of the device being controlled.  Option
  #define USB6_ON_STATE             HIGH //    HIGH, LOW Port control pin ON (active) state.  Option
  #define USB6_DEFAULT_STATE         OFF //    OFF, ON. State to startup in.                  Option
  #define USB7_PIN                   OFF //    OFF, n. I/O pin controlling the port state.    Option
  #define USB7_NAME               "USB7" //    "USB..", Name of the device being controlled.  Option
  #define USB7_ON_STATE             HIGH //    HIGH, LOW Port control pin ON (active) state.  Option
  #define USB7_DEFAULT_STATE         OFF //    OFF, ON. State to startup in.                  Option
  #define USB8_PIN                   OFF //    OFF, n. I/O pin controlling the port state.    Option
  #define USB8_NAME               "USB8" //    "USB..", Name of the device being controlled.  Option
  #define USB8_ON_STATE             HIGH //    HIGH, LOW Port control pin ON (active) state.  Option
  #define USB8_DEFAULT_STATE         OFF //    OFF, ON. State to startup in.                  Option

class Usb {
public:
  void init();
  bool command(char *reply, char *command, char *parameter, bool *suppressFrame, bool *numericReply, CommandError *commandError);
};

typedef struct Port {
   const char* name;
   int16_t pin;
   int16_t value;
   const int16_t active;
} Port;

extern Port myPort[8];
extern Usb usb;
