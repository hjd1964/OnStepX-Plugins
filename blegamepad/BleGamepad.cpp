// BLE gamepad plugin

#include "BleGamepad.h"
#include "Common.h"
#include "../../lib/tasks/OnTask.h"

#include <NimBLEDevice.h>
#include "BleConfig.h"

void blegamepadWrapper() { blegamepad.loop(); }

void BleGamepad::init() {
  VLF("MSG: Plugins, starting: BleGamepad");
  bleInit();
  bleSetup();

    // allow time for the background servers to come up
  delay(2000);

  if (statusBle.onStepFound) {
    statusBle.update();
    delay(100);
  }

  stateBle.init();

  // start a task that runs twice a second, run at priority level 7 so
  // we can block using tasks.yield(); fairly aggressively without significant impact on operation
  tasks.add(500, 0, true, 7, blegamepadWrapper);
  VLF("MSG: BleGamepad Plugin ready");
}

void BleGamepad::loop() {
  bleTimers(); Y;
  bleConnTest(); Y;
}

// Support for specific GamePads
enum 
{
  CONTROLLER_UNKNOWN = 0,
  CONTROLLER_R1,
  CONTROLLER_VRBOX,  
};

// Exact GamePad names (with spaces)
char *szR1A = (char *)"Magicsee R1       ";
char *szR1B = (char *)"Magicsee R1";  
char *szVRBOX = (char *)"VR BOX";
char *szVREXT = (char *)"XTREME VR REMOTE  ";
static int iControllerType;

// ===== GamePad Commands =====
// Only commandBlind() is used to process these since it is ESP32 thread safe 
// (unlike commandBool() etc.) and still reads any response as appropriate.
#define FOCUS_IN          ":F-#"
#define FOCUS_OUT         ":F+#"
#define FOCUS_LOW         ":F3#"     // Set speed 3 (100um) for visual
#define FOCUS_HIGH        ":F4#"     // High speed, 0.5x
#define FOCUS_STOP        ":FQ#"
#define SPIRAL            ":Mp#"     // Spiral search
#define HOMEFOCUS         ":Fh#"     // Move focus to Home (1/2 max position)
#define HOMEZERO          ":FS0#"    // Move focus to 0 position
#define PARKFOCUS         ":FZ#"     // Set focus position to 0
#define PARK              ":hP#"  
#define UNPARK            ":hR#"
#define TRACK_ON          ":Te#"
#define TRACK_OFF         ":Td#"
#define STOP_ALL           ":Q#"     // Emergency stop
#define BEEP          ":SX97,2#"     // Short beep
#define ALERT         ":SX97,3#"     // Alert, full 1 second blast
#define CLICK         ":SX97,4#"     // Click
#define GOTO_CURRENT      ":MS#"     // Returns to last Goto location
#define ALIGN             ":A+#"     // Accept alignment location
#define ALIGNSTARS        ":A3#"     // Begin alignment with 3 stars
#define SYNC              ":CS#"     // Sync with current target 

// ===== Timer values in milliseconds ======
#define MTIMEOUT            1000     // M button "double click" time window
#define SCANTIMER          15000     // Number of ms before rescanning
#define FOCUSTIMER          5000     // Focus speedup timer
#define LONGTIMER            500     // Long push timer 

//====== GamePad Definitions ======
enum
{
  GP_JOY= 0,
  GP_TRIGGERS,
  GP_BTN_M,
  GP_BTN_L,
  GP_BTN_H,  
  GP_BTNAB,
  GP_BTNCD,  
  GP_NUMBYTES
};

// ===== GamePad Button Masks =====

#define GP_BUTTON_UP        0x50
#define GP_LOW_TRIGGER      0x40
#define GP_UPR_TRIGGER      0x80
#define GP_BUTTON_A         0x10
#define GP_BUTTON_B         0x01
#define GP_BUTTON_C         0x08
#define GP_BUTTON_D         0x02
#define GP_BUTTON_M         0x00
#define GP_BUTTON_N         0x10
#define GP_BUTTON_S         0x90
#define GP_BUTTON_E         0x40
#define GP_BUTTON_W         0x60
// VR Box has different direction key codes
#define VR_BUTTON_N         0x40
#define VR_BUTTON_S         0x60
#define VR_BUTTON_E         0x90
#define VR_BUTTON_W         0x10 

// We will connect to server(GamePad) by MAC address
String My_BLE_Address; 
String My_BLE_Address1;
String Scaned_BLE_Address;

// We will use the name to identify GamePad type
String Scaned_BLE_Name;

// All four modes send data. However each mode uses different byte positions and
// values for each of the switches. We are using Mode "B". This allows use of all buttons.

// This is the service UUID of the mouse/joystick device (HID)
static NimBLEUUID serviceUUID("00001812-0000-1000-8000-00805f9b34fb");

// This characteristic UUID works for joystick & triggers (report)
static NimBLEUUID ReportCharUUID("00002A4D-0000-1000-8000-00805f9b34fb"); 

// Control flags
static bool movingNorth = false;
static bool movingSouth = false;
static bool movingWest = false;
static bool movingEast = false;
static bool joyActive = false; // Joystick movement active flag
static bool buttonPress = false; // buttons active flag
static bool buttonAPress = false; // button A pressed 
static bool triggerPress = false; // Focus trigger active
static bool timerReturn = false; // Focus timer for high speed
static bool pressedOnce = false; // Indicates a button was pushed (for M button)
static bool focusSpd = false; // Focus speed toggle flag
static bool spiralActive = false; // Spiral search start/stop flag
static bool doConnect = false; // Run the connection routine
static bool connected = false; // Fully connected to the GamePad
static bool haveDevice = false; // Found one of our GamePads
static bool firstRate = true; // Allow rate change during alignment

// NimBLE device model
static NimBLEAdvertisedDevice* myDevice = nullptr;
static NimBLEClient *pClient = nullptr;

// This is where we store the data from the buttons and joystick
volatile byte GpBoxData[GP_NUMBYTES];

// ===== Timers, millisecond re-triggerable =====

// LTimer is the time window for a long push
volatile uint32_t LTimer = millis();

// MTimer is the amount of time you have to do a
// second click. 
volatile uint32_t MTimer = millis();

// pushTimer sets the amount of time a key must be pressed 
// before a button state change is triggered
volatile uint32_t pushTimer = millis();

// scanTimer sets the amount of time between rescans on 
// device disconnect
volatile uint32_t scanTimer = millis();

// Task handles  
TaskHandle_t HandleJS = NULL;   // handle of the joystick task
TaskHandle_t HandleBtn = NULL;  // handle of the button task
TaskHandle_t HandleM = NULL;  // handle of the M button task

// GuideRate up/down
int activeGuideRate;      // Scope, 2 to 9

// Task Stack memory allocation size
#define TaskStackSize     4096

void logInputPacket(const char* label, const uint8_t* pData, size_t length)
{
  char msg[96];
  if (length >= 2) {
    snprintf(msg, sizeof(msg), "bleMSG: %s len=%u data=%02X %02X", label, (unsigned)length, pData[0], pData[1]);
  } else if (length == 1) {
    snprintf(msg, sizeof(msg), "bleMSG: %s len=%u data=%02X", label, (unsigned)length, pData[0]);
  } else {
    snprintf(msg, sizeof(msg), "bleMSG: %s len=%u", label, (unsigned)length);
  }
  VLF(msg);
}

void logControllerAction(const char* action)
{
  VF("bleMSG: action=");
  VLF(action);
}

void resetControllerState(bool stopMotion)
{
  if (stopMotion) {
    if (movingNorth || movingSouth || movingEast || movingWest || spiralActive) {
      onStepBle.commandBlind(STOP_ALL);
    }
    if (triggerPress) {
      onStepBle.commandBlind(FOCUS_STOP);
    }
  }

  movingNorth = false;
  movingSouth = false;
  movingWest = false;
  movingEast = false;
  joyActive = false;
  buttonPress = false;
  buttonAPress = false;
  triggerPress = false;
  timerReturn = false;
  pressedOnce = false;
  focusSpd = false;
  spiralActive = false;
  firstRate = true;
  MTimer = 0;
  LTimer = 0;
  pushTimer = 0;
  for (int i = 0; i < GP_NUMBYTES; i++) {
    GpBoxData[i] = 0;
  }
}

void notifyJoystickTask()
{
  if (HandleJS) {
    xTaskNotifyGive(HandleJS);
  }
}

// ===== Focuser functons =====
void StartFocus()
{
  if (FOCUS_INIT == ON) 
  {           
  onStepBle.commandBlind(PARKFOCUS);  //Set 0 position  
  onStepBle.commandBlind(HOMEFOCUS);  // Move half way
  }
}
void ParkFocus()
{
  if (FOCUS_INIT == ON) 
  {           
  onStepBle.commandBlind(HOMEZERO); //Move focus to 0 position   
  onStepBle.commandBlind(PARKFOCUS); // Set 0 position
  }
}

// ===== LED functions =====
void LED_STATE_A()
{
}
void LED_STATE_B()
{
}

// ===== Sound functions =====
void Beep()
{
  if (SOUND == ON) {
    onStepBle.commandBlind(BEEP);
  }
}
void Alert() 
{
  if (SOUND == ON) {
    onStepBle.commandBlind(ALERT);
  }
}
void Click()
{
  if (SOUND == ON) {
    onStepBle.commandBlind(CLICK);
  }
}

// ===== Direction functions =====
void MoveNorth() // Move North
{ 
  if (!movingNorth)
  {
    logControllerAction("move north");
    movingNorth = ON;
    onStepBle.commandBlind(":Mn#");
  }
}
void MoveSouth() // Move South 
{ 
  if (!movingSouth)
  {
    logControllerAction("move south");
    movingSouth = ON;
    onStepBle.commandBlind(":Ms#");
  }
}
void MoveEast() // Move East 
{ 
  if (!movingEast)
  {
    logControllerAction("move east");
    movingEast = ON;
    onStepBle.commandBlind(":Me#");
  }
}
void MoveWest() // Move West 
{ 
  if (!movingWest)
  {
    logControllerAction("move west");
    movingWest = ON;
    onStepBle.commandBlind(":Mw#");
  }
}

//******************************************************************************
// HID notification callback handler.
//******************************************************************************
void notifyCallback(
  NimBLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify)
{
  UNUSED(pBLERemoteCharacteristic);
  UNUSED(isNotify);

  switch (iControllerType)
  {
    default: // For VR Box and MagicSee R1
    if (length == 2) // it always writes 2 byte reports
    { 
      // Emergency stop, any button, not joystick (OnStep will abort guide on joystick touch)
      if (!((0x00 == pData[0]) && (0x00 != pData[1])))
      {  
        if(!((pData[0] == 0x00) && (pData[1] == 0x50)))
        {
          if (statusBle.inGoto || movingNorth || movingSouth || movingEast || movingWest)
          { 
            logInputPacket("stop request", pData, length);
            onStepBle.commandBlind(STOP_ALL);
            spiralActive = false;
            Alert();
            return;
          }
          if(spiralActive)
          {
            logInputPacket("spiral cancel", pData, length);
            spiralActive = false; 
            onStepBle.commandBlind(STOP_ALL);
            Click();
            return;
          }  
        }
      }

      // UP button trap
      if ((pData[0] == 0x00) && (pData[1] == 0x50))
      {
        if (joyActive || movingNorth || movingSouth || movingEast || movingWest) // Joystick movement in progress
        {
          GpBoxData[GP_JOY] = pData[1];
          notifyJoystickTask();
        }  
        else if (buttonPress || triggerPress) // Button has been pressed
        {
          if (HandleBtn)
          {
            GpBoxData[GP_BTN_L] = pData[0];
            GpBoxData[GP_BTN_H] = pData[1];
            vTaskResume(HandleBtn);
          }
        }
      }

      // M button, wake the M button handler task 
      else if (0x00 == pData[0] && 0x00 == pData[1])
      {
        // GpBoxData[GP_BTN_M] = pData[0];
        if (HandleM)
          vTaskResume(HandleM);
      }

      // Joystick, wake the Joy button handler task
      else if ((0x00 == pData[0]) && (0x00 != pData[1]))
      {
        GpBoxData[GP_JOY] = pData[1];
        notifyJoystickTask();
      }

      else
      {
        // other Buttons, wake the BTN task
        GpBoxData[GP_BTN_L] = pData[0];
        GpBoxData[GP_BTN_H] = pData[1];
        if (HandleBtn)
          vTaskResume(HandleBtn);
      }
    }
    else
    {
      logInputPacket("ignored packet", pData, length);
    }
  }
}

// End of notifyCallback

//******************************************************************************
// Connection state change event callback handler.
//******************************************************************************
class MyClientCallback: public NimBLEClientCallbacks
{
  void onConnect(NimBLEClient* pclient) override
  {
    UNUSED(pclient);
    scanTimer = 0;                                      // Reinitalize the scan timer
    resetControllerState(false);
    LED_STATE_A();
    VLF("bleMSG: BLE GamePad connected");
  }
  
  void onDisconnect(NimBLEClient* pclient, int reason) override
  {
    UNUSED(pclient);
    resetControllerState(true);
    haveDevice = false;
    connected = false;
    doConnect = false;
    if (myDevice != nullptr) {
      delete myDevice;
      myDevice = nullptr;
    }
    scanTimer = millis() + SCANTIMER;                  // Restart the scan timer
    LED_STATE_B();
    VF("bleMSG: BLE GamePad disconnected, reason="); VL(reason);
  }
} clientCallbacks;
// End of MyClientCallback

//******************************************************************************
// Validate the server has the correct name and services we are looking for.
// The server must have the HID service.
//******************************************************************************
bool connectToServer()
{
  VLF("bleMSG: connecting to target controller");
  if (myDevice == nullptr) {
    VLF("bleMSG: no device selected");
    return false;
  }
 
  if (pClient != nullptr) {
    if (pClient->isConnected()) {
      pClient->disconnect();
    }
    NimBLEDevice::deleteClient(pClient);
    pClient = nullptr;
  }

  if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS) {
    VLF("Max clients reached - no more connections available");
    return false;
  }

  pClient = NimBLEDevice::createClient();
  if (pClient == nullptr) {
    VLF("Failed to create client");
    return false;
  }

  VLF("New client created");

  pClient->setClientCallbacks(&clientCallbacks, false);
  pClient->setConnectionParams(12, 12, 0, 150);
  pClient->setConnectTimeout(5 * 1000);

  if (!pClient->connect(myDevice)) {
    NimBLEDevice::deleteClient(pClient);
    pClient = nullptr;
    VLF("Failed to connect, deleted client");
    return false;
  }

  VF("Connected to: "); V(pClient->getPeerAddress().toString().c_str()); V(" RSSI: "); VL(pClient->getRssi());
    
  // BLE servers may offer several services, each with unique characteristics
  // we can identify the type of service by using the service UUID
  // Obtain a reference to the service we are after in the remote BLE server.
  // This will return a pointer to the remote service if it has a matching service UUID
  
  NimBLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  VLF("bleMSG: after getService ");   
  if (pRemoteService == nullptr)
  {
    pClient->disconnect();
    return false;
  }
  VLF("bleMSG: setupCharacteristics");   

  /** Now we can read/write/subscribe the characteristics of the services we are interested in */
  NimBLERemoteCharacteristic* pChr = pRemoteService->getCharacteristic(ReportCharUUID);
  if (pChr == nullptr)
  {
    VLF("bleMSG: report characteristic not found");
    pClient->disconnect();
    return false;
  }

  // this seems to be necessary, otherwise can't subscribe
  if (pChr->canRead()) 
  {
     pChr->getUUID();
     pChr->readValue();
  }
  if (pChr->canNotify()) 
  {
    if (!pChr->subscribe(true, notifyCallback)) 
    {
    VLF("bleMSG: cannot subscribe");
    pClient->disconnect();
    return false;
    }
  }
  else 
  {
    VLF("bleMSG: cannot notify");   
    pClient->disconnect();
    return false;
  }
    
  VLF("bleMSG: notifications subscribed");   

  resetControllerState(false);
  doConnect = false;
  connected = true;
  return true; 
}
// End of connectToServer

//******************************************************************************
// Scan for BLE servers and find the first one that advertises the service
// we are looking for.
//******************************************************************************
class ScanCallbacks: public NimBLEScanCallbacks
{
  // Called for each advertising BLE server.
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override
  {
    // We have found a server, see if it has the name we are looking for
    if (!haveDevice) 
    {
      if (advertisedDevice->haveName())
      {
        Scaned_BLE_Address = advertisedDevice->getAddress().toString().c_str();
        Scaned_BLE_Name = advertisedDevice->getName().c_str(); 

        VF("bleMSG: BLE Address = "); VL(Scaned_BLE_Address); 
        VF("bleMSG: Device Name = ");
        VF("["); 
        V(advertisedDevice->getName().c_str());  
        VLF("]");

        if ((Scaned_BLE_Address == (My_BLE_Address)) || (Scaned_BLE_Address == (My_BLE_Address1)))
        {
          // We found a server with the correct Mac, see if it has the service we are
          // interested in (HID)
          if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(serviceUUID))
          {
            // Compare it to known controllers
            iControllerType = CONTROLLER_UNKNOWN;
            if (Scaned_BLE_Name == szR1A)
            iControllerType = CONTROLLER_R1; 
            else if (Scaned_BLE_Name == szR1B)
            iControllerType = CONTROLLER_R1;              
            else if (Scaned_BLE_Name == szVRBOX)
            iControllerType = CONTROLLER_VRBOX; 
            else if (Scaned_BLE_Name == szVREXT)
            iControllerType = CONTROLLER_VRBOX; 

            VF("bleMSG: iControllerType = ");
            VL(iControllerType);
            VF("bleMSG: target controller matched, rssi="); VL(advertisedDevice->getRSSI());

            doConnect = true;
            haveDevice = true;
            scanTimer = 0; // reinitalize the scan timer
            NimBLEDevice::getScan()->stop();
            if (myDevice != nullptr) {
              delete myDevice;
            }
            myDevice = new NimBLEAdvertisedDevice(*advertisedDevice);
          }           
        } 
      }
    }
  }
} scanCallbacks;
// End of MyAdvertisedDeviceCallbacks

// All of these tasks are designed to run forever. The tasks are resumed when
// a notification message is received with new data.

//******************************************************************************
// Joystick Task.
//******************************************************************************
void taskJoyStick(void *parameter)
{
  // Forever loop
  while(true)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait for new joystick data

    // We just woke up, new data is available
    if(INIT_ALIGN == ON) // Set speed for alignment
    {
      if (!statusBle.aligning) firstRate = true; // Reset for multiple aligns
      if (statusBle.aligning && firstRate) // Set guide rate = 48X for align
      {
        firstRate = false; // Ignore after starting align to allow rate changes        
        activeGuideRate = 7; // 48X
        char cmd[5] = ":Rn#"; cmd[2] = '0' + activeGuideRate; // set initial GuideRate
        onStepBle.commandBlind(cmd);
      }
    }
    if (GpBoxData[GP_JOY] == GP_BUTTON_UP) // Joystick is centered 
    { 
      joyActive = false; // Turn off JoyStick active flag

      if (movingNorth || movingSouth || movingEast || movingWest)
      { 
        if (movingNorth) 
        {
          movingNorth = false; 
          onStepBle.commandBlind(":Qn#");
        }
        if (movingSouth) 
        {
          movingSouth = false; 
          onStepBle.commandBlind(":Qs#");
        }
        if (movingEast) 
        {
          movingEast = false; 
          onStepBle.commandBlind(":Qe#");
        }
        if (movingWest) 
        {
          movingWest = false; 
          onStepBle.commandBlind(":Qw#");
        }
      }        
    }
    else // Joystick directional move
    {
      joyActive = true; // JoyStick active flag

      if (iControllerType == CONTROLLER_UNKNOWN || iControllerType == CONTROLLER_R1)
      {
        if (GpBoxData[GP_JOY] == GP_BUTTON_N)
        {
          if (NS_SWAP == OFF) MoveNorth();
          else MoveSouth();
        }
        else if (GpBoxData[GP_JOY] == GP_BUTTON_S)
        {
          if (NS_SWAP == OFF) MoveSouth(); 
          else MoveNorth();
        }
        if (GpBoxData[GP_JOY] == GP_BUTTON_E)
        {
          if (EW_SWAP == OFF) MoveEast();
          else MoveWest();
        }
        else if (GpBoxData[GP_JOY] == GP_BUTTON_W)
        {
          if (EW_SWAP == OFF) MoveWest();
          else MoveEast();
        }
      }
      else if (iControllerType == CONTROLLER_VRBOX)
      {
        if (GpBoxData[GP_JOY] == VR_BUTTON_N)
        {
          if (NS_SWAP == OFF) MoveNorth();
          else MoveSouth();
        }
        else if (GpBoxData[GP_JOY] == VR_BUTTON_S)
        {
          if (NS_SWAP == OFF) MoveSouth(); 
          else MoveNorth();
        }
            
        if (GpBoxData[GP_JOY] == VR_BUTTON_E)
        {
          if (EW_SWAP == OFF) MoveEast();
          else MoveWest();
        }
        else if (GpBoxData[GP_JOY] == VR_BUTTON_W)
        {
          if (EW_SWAP == OFF) MoveWest();
          else MoveEast();
        }
      }
    }
  }
}
// End of taskJoyStick

//******************************************************************************
// M button task
// Double tap of M button within MTIMEOUT miliseconds
//******************************************************************************
void taskButtonM(void *parameter)
{
  while(true)
  {
    vTaskSuspend(NULL); // give up the CPU, wait for new data
    
    // We just woke up, new data is available
    if (M_SINGLE_CLICK == ON) pressedOnce = true;
    
    if (!pressedOnce) // First press, start double click timer
    {
      pressedOnce = true;
      MTimer = millis() + MTIMEOUT;
      continue;
    }
    if (M_BUTTON == ON)
    {
      if (pressedOnce)
      {
        // Set align point when doing an alignment
        if (statusBle.aligning)
        {
          if (statusBle.alignThisStar == statusBle.alignLastStar) Alert();
          else Click();
          onStepBle.commandBlind(ALIGN);
        }
        // Unpark mount if parked        
        else if (statusBle.parked) 
        {
          onStepBle.commandBlind(UNPARK);
          StartFocus();          
          Beep();          
        }
        // Start tracking if "At home"        
        else if (statusBle.atHome && !statusBle.tracking)
        {
          onStepBle.commandBlind(TRACK_ON);
          if(INIT_ALIGN == ON)
          {
            onStepBle.commandBlind(ALIGNSTARS);    
          }
          StartFocus();
          Alert();                   
        }
        // Start tracking if tracking off
        else if (!statusBle.tracking)
        {
          onStepBle.commandBlind(TRACK_ON);
          Beep();          
        }
        // Park the scope                
        else if (!statusBle.parked)
        {
          ParkFocus();
          onStepBle.commandBlind(PARK);                
        }
        pressedOnce = false;          
        MTimer = 0;
      }
    }
  }
}
//  End of taskButtonM

//******************************************************************************
// Buttons handler Task. 
// Press of any button will resume this task. 
//******************************************************************************
void taskBtn(void *parameter)
{
  while(true)
  {
    vTaskSuspend(NULL); // give up the CPU, wait for new data

    // We just woke up, new data is available
    if(GpBoxData[GP_BTN_H] == 0x50) // focus triggers, B and D buttons
    {
      if (GpBoxData[GP_BTN_L] == 0X00) // Button UP
      {
        if(buttonAPress)
        {
            if (LTimer && (LTimer < millis())) // Long press
          {
            onStepBle.commandBlind(SYNC);            
            Click();
          }
          else // A short click
          {
            onStepBle.commandBlind(GOTO_CURRENT); // Button A pressed, goto last location
          }
          LTimer = 0;
          buttonAPress = false;
          buttonPress = false;
        }

        else if (triggerPress)
        {
          // Trigger released, stop focuser 
          if (GpBoxData[GP_BTN_L] == 0x00)
          {
            onStepBle.commandBlind(FOCUS_STOP);        
            pushTimer = 0;
            triggerPress = false;   
            focusSpd = false;
          }
        }
        continue;
      }
      if (GpBoxData[GP_BTN_L] == GP_LOW_TRIGGER)
      {
        // The lower trigger button is pressed
        if (focusSpd && triggerPress)
          {
            onStepBle.commandBlind(FOCUS_HIGH); 
            focusSpd = false;
            Click();
            }
        else 
          {
            onStepBle.commandBlind(FOCUS_LOW);
            pushTimer = millis() + FOCUSTIMER; 
          }
        onStepBle.commandBlind(FOCUS_IN);         
        triggerPress = true;
        continue;
      }
      
      if (GpBoxData[GP_BTN_L] == GP_UPR_TRIGGER)
      {
        // The upper trigger button is pressed
        if (focusSpd && triggerPress)
        {
          onStepBle.commandBlind(FOCUS_HIGH);
          Click(); 
          focusSpd = false;
        }
        else 
        {
          onStepBle.commandBlind(FOCUS_LOW);
          pushTimer = millis() + FOCUSTIMER; 
        }
        onStepBle.commandBlind(FOCUS_OUT);         
        triggerPress = true;
        continue;
      }
    
      else if (GpBoxData[GP_BTN_L] == GP_BUTTON_B)
      {
        // Button B pressed
        if (spiralActive == false)
        {
          spiralActive = true;
          onStepBle.commandBlind(SPIRAL);
          Click();
        }
        else
        {
          spiralActive = false; 
          onStepBle.commandBlind(STOP_ALL);
          Click();
        }         
      }

      else if (GpBoxData[GP_BTN_L] == GP_BUTTON_D)
      {
        // Button D pressed
        (activeGuideRate++);
        if (activeGuideRate < 10) Click();
        if (activeGuideRate < 3) activeGuideRate = 3;
        if (activeGuideRate > 9) activeGuideRate = 9;
        char cmd[5] = ":Rn#"; cmd[2] = '0' + activeGuideRate;
        onStepBle.commandBlind(cmd);
      }

      if (iControllerType == CONTROLLER_VRBOX)
      { 
        if ( GpBoxData[GP_BTN_L] == GP_BUTTON_A)
        {
          LTimer = millis() + LONGTIMER;
          buttonAPress = true;
          buttonPress = true;
        }
        else if (GpBoxData[GP_BTN_L] == GP_BUTTON_C)
        {
          // button C pressed
          (activeGuideRate--);
          if (activeGuideRate > 2) Click(); 
          if (activeGuideRate < 3) activeGuideRate = 3;
          if (activeGuideRate > 9) activeGuideRate = 9;
          char cmd[5] = ":Rn#"; cmd[2] = '0' + activeGuideRate;
          onStepBle.commandBlind(cmd);
        }
      } 
    }

    else if ( GpBoxData[GP_BTN_L] == GP_BUTTON_A)
    {
      LTimer = millis() + LONGTIMER;
      buttonAPress = true;
      buttonPress = true;
    }

    else if (GpBoxData[GP_BTN_L] == GP_BUTTON_C)
    {
      // button C pressed
      (activeGuideRate--);
      if (activeGuideRate > 2) Click(); 
      if (activeGuideRate < 3) activeGuideRate = 3;
      if (activeGuideRate > 9) activeGuideRate = 9;
      char cmd[5] = ":Rn#"; cmd[2] = '0' + activeGuideRate;
      onStepBle.commandBlind(cmd);
    }       
  }
}
//  End of taskButtonBtn


//******************************************************************************
// Initial and Service routines
//******************************************************************************

void BleGamepad::bleInit()
{
  VLF("bleMSG: bleInit");
}

void DoScan() 
{
  if (scanTimer && (scanTimer < millis()))
  {
    // Retrieve a GATT Scanner and set the callback we want to use to be informed 
    // when we have detected a new device.  Specify that we want active scanning
    // and start the scan.
  
    NimBLEScan* pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setScanCallbacks(&scanCallbacks, false);
    pBLEScan->setInterval(5000);
    pBLEScan->setWindow(500);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(1000); // Scan 1 second
      
    pBLEScan->clearResults(); // Delete results fromBLEScan buffer to release memory, solves heap problem
  
    // Restart the scan timer
    scanTimer = millis() + SCANTIMER;
    VLF("bleMSG: Scanning for BLE GamePad");
  }
}
// End of DoScan.

//******************************************************************************
// This is the first (and after disconnect) server connection
//******************************************************************************
void BleGamepad::bleConnTest() 
{
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.

  //VLF("bleMSG: bleConnTest");
  if (doConnect == true) {
    if (!connectToServer()) {
      doConnect = false;
      haveDevice = false;
      scanTimer = millis() + SCANTIMER;
      if (myDevice != nullptr) {
        delete myDevice;
        myDevice = nullptr;
      }
    }
  }
  else
  {
    doConnect = false;
    DoScan();
  }
  //VLF("bleMSG: bleConnTest end");
}
// End of bleConnTest

void BleGamepad::bleSetup()
{
  VLF("bleMSG: bleSetup");
  My_BLE_Address = BLE_GP_ADDR;
  My_BLE_Address1 = BLE_GP_ADDR1;
  MTimer = 0;  
  pushTimer = 0;
  activeGuideRate = statusBle.guideRate;
  BaseType_t xReturned;

  VLF("bleMSG: Starting BLE GamePad Services"); 

  // Create tasks to handle the joystick and buttons
  xReturned = xTaskCreate(taskJoyStick,             // task to handle activity on the joystick.
                          "Joystick",               // String with name of task.
                          TaskStackSize,            // Stack size in 32 bit words.
                          NULL,                     // Parameter passed as input of the task
                          1,                        // Priority of the task.
                          &HandleJS);               // Task handle.

  xReturned = xTaskCreate(taskBtn,             // task to handle activity on the C, D & Triggger buttons.
                          "Btn",              // String with name of task.
                          TaskStackSize,            // Stack size in 32 bit words.
                          NULL,                     // Parameter passed as input of the task
                          1,                        // Priority of the task.
                          &HandleBtn);               // Task handle.

  xReturned = xTaskCreate(taskButtonM,             // task to handle activity on the M button.
                          "ButtonsM",              // String with name of task.
                          TaskStackSize,            // Stack size in 32 bit words.
                          NULL,                     // Parameter passed as input of the task
                          1,                        // Priority of the task.
                          &HandleM);               // Task handle.

  UNUSED(xReturned);

  if (!NimBLEDevice::isInitialized()) {
    NimBLEDevice::init("OnStepX");
  }
  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_SC);

  /** Optional: set the transmit power */
  NimBLEDevice::setPower(3); /** 3dbm */

  if (!connected)
  {
    VLF("bleMSG: !connected, scan");
    // Retrieve a GATT Scanner and set the callback we want to use to be informed 
    // when we have detected a new device.  Specify that we want active scanning
    // and start the scan.
    
    NimBLEScan* pBLEScan = NimBLEDevice::getScan();
    //pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setScanCallbacks(&scanCallbacks, false);
    pBLEScan->setInterval(5000);
    pBLEScan->setWindow(500);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5000, false); // Scan
    //delay(3000); // Delay for scan
  }

  if (!connected)
  {
    VLF("bleMSG: !connected, restart scan timer");
    scanTimer = millis() + SCANTIMER; // Restart the scan timer
  }

  bleConnTest();     
}
// End of bleSetup

void BleGamepad::bleTimers() 
{
  if (connected)
  {
    if (MTimer && (MTimer < millis()))
    { 
      // M button double click allowed time window expired, reset 
      pressedOnce = false;
      MTimer = 0; 
    }
          
    if ((pushTimer && (pushTimer < millis())) && triggerPress)
    {
      // Focus button held down for FOCUSTIMER, speed up focuser
      focusSpd = true;
      pushTimer = 0; 
      if (HandleBtn)
        vTaskResume(HandleBtn);
    }
  }
}
// End of bleTimers


BleGamepad blegamepad;
