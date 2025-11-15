// USB port switching plugin

#include "Usb.h"
#include "../../lib/gpioEx/GpioEx.h"

Port myPort[8] = {
      { USB1_NAME, USB1_PIN, USB1_DEFAULT_STATE, USB1_ON_STATE },
      { USB2_NAME, USB2_PIN, USB2_DEFAULT_STATE, USB2_ON_STATE },
      { USB3_NAME, USB3_PIN, USB3_DEFAULT_STATE, USB3_ON_STATE },
      { USB4_NAME, USB4_PIN, USB4_DEFAULT_STATE, USB4_ON_STATE },
      { USB5_NAME, USB5_PIN, USB5_DEFAULT_STATE, USB5_ON_STATE },
      { USB6_NAME, USB6_PIN, USB6_DEFAULT_STATE, USB6_ON_STATE },
      { USB7_NAME, USB7_PIN, USB7_DEFAULT_STATE, USB7_ON_STATE },
      { USB8_NAME, USB8_PIN, USB8_DEFAULT_STATE, USB8_ON_STATE }
    };

void Usb::init() {
  VLF("MSG: Plugins, starting: USB port switcher");

  // set up USB port control
  for (int i = 0; i < 8; i++) {
    if (myPort[i].pin != OFF) {
     pinMode(myPort[i].pin, OUTPUT);
     myPort[i].value = (-1 - myPort[i].value);
     digitalWriteEx(myPort[i].pin, myPort[i].value);
    }
  }
}

// Command process is a copy and paste from Features.command.cpp with unwanted stripped out and names changed
bool Usb::command(char *reply, char *command, char *parameter, bool *supressFrame, bool *numericReply, CommandError *commandError) {
  *supressFrame = false;

  // Get USB commands
  if (command[0] == 'G' && command[1] == 'U') {
    // :GUXn#
    // return status of USB port (1 for ON, 0 for OFF)
    if (parameter[0] == 'X') { 
      int i = parameter[1] - '1';
      if (i < 0 || i > 7)  { *commandError = CE_PARAM_FORM; return true; }
      char s[255];
        sprintf(s, "%d", myPort[i].value);
        strcat(reply, s);
        *numericReply = false;
        return true;
    } else // end :GUXn# 
      
    // :GUY0#
    // return active USB ports in the form 00100111 where each digit is enabled/disabled
    if (parameter[0] == 'Y') {
      int i = parameter[1] - '1';
      if (i == -1) {
        for (int j = 0; j < 8; j++) {
          if (myPort[j].pin == OFF) reply[j] = '0'; else reply[j] = '1';
          reply[j + 1] = 0;
        }
        *numericReply = false;
        return true;
      }

      // :GUY[n]#
      // where [n] = 1..8 to get USB port name
      if (i < 0 || i > 7)  { *commandError = CE_PARAM_FORM; return true; }
      if (myPort[i].pin == OFF) { *commandError = CE_0; return true; }
      char s[255];
      strcpy(s, myPort[i].name);
      if (strlen(s) > 10) s[10] = 0;
      strcpy(reply, s);
      *numericReply = false;
      return true;
    } else return false; // end :GUY...#
  } else

  // Set USB commands
  if (command[0] == 'S' && command[1] == 'U' && parameter[2] == ',') {
    // :SUX[n],V[0|1]#
    // for example :SUX1,V1#
    // special case :SUX0,V[0|1] # = switch all enabled ports
    if (parameter[0] == 'X') { 
      int i = parameter[1] - '1';
      if (i < -1 || i > 7)  { *commandError = CE_PARAM_FORM; return true; }

      char* conv_end;
      float f = strtof(&parameter[4], &conv_end);
      if (&parameter[4] == conv_end) { *commandError = CE_PARAM_FORM; return true; }
      long v = lroundf(f);

      if (parameter[3] == 'V') {
        if (v >= 0 && v <= 1) { // value 0..1 for enabled or not
          if (i == -1) {
            // switch all enabled ports
            for (int j = 0; j < 8; j++) {
              if (myPort[j].pin != OFF) {
                digitalWriteEx(myPort[j].pin, v == myPort[j].active);
                myPort[j].value = v;
              }
            }
          } else {
            // switch single port
            digitalWriteEx(myPort[i].pin, v == myPort[i].active);
            myPort[i].value = v;
          }
          return true;
        } else *commandError = CE_PARAM_RANGE;
      } else *commandError = CE_PARAM_FORM;
    } else return false;
  } else return false; // end :SUX...#

  return true;
}

Usb usb;
