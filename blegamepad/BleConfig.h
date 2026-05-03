// bleGamepad configuration file
#pragma once

  //***********************************************************************************************************
  // Configuration settings
  //***********************************************************************************************************

  // BLE GAMEPAD SETTINGS (ESP32 ONLY) ------------------------------------------------ see https://onstep.groups.io/g/main/wiki/26762
  #define BLE_GAMEPAD                   ON //    OFF, ON to allow BLE gamepad connection for ESP32 only.                       Option
  #define BLE_GP_ADDR   "ff:24:08:05:21:2d" // f5:cf", GamePad MAC address #1                                                   Adjust
                                            //         GamePad device address must match exactly else it will not connect!
                                            //         Replace address with yours, in lower case, with colons as shown.
  #define BLE_GP_ADDR1  "ff:ff:ff:ff:ff:ff" // ff:ff", GamePad MAC address #2                                                   Adjust
                                            //         Two GamePads are allowed, operating in a "handoff" mode, one at time.

  // ON/OFF unless noted otherwise

  // Direction button swaps
  #define NS_SWAP              OFF      // ON reverses direction buttons from OnStep standard
  #define EW_SWAP              OFF      // East/West swap

  #define LED_ENABLE            ON      // LED on/off behavior, OFF disables LED BLE connected/disconnected
                                        // indication. ON = LED off when connected, on when disconnected
                                        // REV = LED on when connected, off when disconnected

  #define FOCUS_INIT            ON      // Focuser moves to half-position on M button "Start tracking" press,
                                        // moves to "0" position on "M" button "Park". OFF disables feature

  #define SOUND                 ON      // ON, Enables beeps and alerts

  #define M_BUTTON              ON      // ON, Add addtional functions to M button.  
                                        // For supported GamePads only.

  #define INIT_ALIGN            ON      // ON, Enables M button align from "At Home" (startup) with 3 
                                        // star alignment (see ALIGNSTARS to change number of stars) 

  #define MODE_D               OFF      // OFF, uses Mode B, ON use Mode D for GamePad if it supports it.   
                                        // V2.0 code for backward compatabilty.
  
  #define M_SINGLE_CLICK       OFF      // OFF, ON, M button will activate on a single click as in  
                                        // V2.0 code, for backward compatabilty.                                                                                    

  //***********************************************************************************************************