/*
   Uses STM32duino with Phono patch. Must add 33 and 95 CAN speeds => Megadrifter's edition
*/
#include <HardwareCAN.h>

/////// === Настройки модуля! === ///////

// Uncomment to enable 'debug()' messages output
#define DEBUG

// Uncomment to enable 'log()' messages output
#define LOG

// Choose output serial port
#define SERIAL Serial2

// Choose CAN pins for each bus to enable dual
#define CAN_GPIO_PINS_MS CAN_GPIO_PA11_PA12
#define CAN_GPIO_PINS_LS CAN_GPIO_PB8_PB9

// Built-in LED
#define PC13ON 0
#define PC13OFF 1

// default delay?
#define DELAY 200

/////// === Глобальные переменные === ///////
/* global variables */
uint8 ecnMode; // temporary, must be enum for state-machine
enum EcnMode {OFF, ECN_TEMP, ECN_VOLT};

long ecnMillis = 0; // size?
short ecnWaitTime = 1000; // pause between ecn screen update in mode 1
uint8 coolantTemp;
uint8 voltage = 0;

// Flags
volatile bool flagHandBrake = false;
volatile bool flag;

// Instanciation of CAN interface
HardwareCAN canBus(CAN1_BASE);
CanMsg msg ;
CanMsg *r_msg;

void setup()
{
  SERIAL.begin(115200);
  SERIAL.println("Hello World!");
  SERIAL.println("Starting LS-module v1.08 2018-12-02");
  debug("checking debug level");
  debug("checking debug with value", 1);
  debugHex("checking debugHex with value 32", 32);
  log("checking log level");
  delay(DELAY);

  debug("Setting globals");
  ecnMode = 0;
  coolantTemp = 40;

  pinMode(PC13, OUTPUT); // LED is ON until end of setup()
  digitalWrite(PC13, PC13ON);
  delay(50);// to see the LED
  log("Initialize the CAN module ...");

  lsCANSetup();        // Initialize the CAN module to LS CAN bus
  log("Initialization LS CAN ON");
  wakeUpBus();
  lsBeep(2);
  panelCheck();
  playWithEcn();

  digitalWrite(PC13, PC13OFF);
}

void loop()
{
  //  debug("loop");
  // ======== receive message and set flags =========
  while ( ( r_msg = canBus.recv() ) != NULL )
  {
    ///// processing the incoming message
    if (r_msg->ID == 0x100) {
      debug("0x100");
    }// do nothing

    else if (r_msg->ID == 0x145) { // engine tempr
      debug("engine tempr");
      if (ECN_TEMP == ecnMode) {
        debug("mode = ", ecnMode);
        coolantTemp = r_msg->Data[3];
      }

    } else if (r_msg->ID == 0x175) { // Steering wheel buttons
      debug("Steering wheel buttons");
      if ( (r_msg->Data[5] == 0x20) && (r_msg->Data[6] == 0x01) ) {
        //       left knob down
        debug("left knob down");
        ecnMode++;
        debug("mode = ",ecnMode);
        log("ECN mode on / +1");
        delay(100); // bad way to avoid multiple change
        #ifdef DEBUG
        lsBeep(ecnMode);
        #endif
      } else if ( (r_msg->Data[5] == 0x10) && (r_msg->Data[6] == 0x1F) ) {
        debug("left knob up");
        ecnMode = 0;
        debug("mode = ",ecnMode);
        log("ECN mode off");
        lsShowEcn(0x0F, 0xF0, 0xFF); // temp
      }

    } else if (r_msg->ID == 0x230) {
      debug("Doors/locks");
      printMsg();

    
    } else if (r_msg->ID == 0x370) {
      debug("handbrake, fog lights, etc...");
      if ((r_msg->Data[1]) & 0x01) {
        if (!flagHandBrake) {
          log("handbrake was DOWN, now UP");
          flagHandBrake = true;
        }
      } else {
        if (flagHandBrake) {
          log("handbrake was UP, now DOWN");
          flagHandBrake = false;
        }
      }

    } else if (r_msg->ID == 0x500) { // voltage
      debug("voltage");
      if (ECN_VOLT == ecnMode) {
        voltage = r_msg->Data[1]+28;
        debug("read voltage");
      }
      //========================
      //    } else if (r_msg->ID == 0x) {
      //if (r_msg->Data[1])
    } else {
    }  // end if

    canBus.free();

  }
  // close while
  // ======== check flags and execute actions =========
  if ((ECN_TEMP == ecnMode) && (millis() > ecnMillis)) {
    debug("(1 == ecnMode) && (millis() > ecnMillis)");
    debug("Coolant: ", coolantTemp - 40);

    ecnMillis = millis() + ecnWaitTime;
    // process coolant
    uint8 d0 = 0x0C;
    uint8 d1 = 0xEE;
    uint8 d2 = 0xEE;
    uint8 ampl = 0;
    if (coolantTemp < 40) {
      ampl = 40 - coolantTemp; // amplitude
      d1 = 0x0F;
    } else {
      ampl = coolantTemp - 40;
      if (ampl < 100) {
        d1 = 0x00;
      } else {
        d1 = 0x01;
        ampl = ampl - 100;
      }
    }
    d2 = ampl / 10 * 16 + ampl % 10; // to hex but show like dec;
    lsShowEcn(d0, d1, d2);
    if (flagHandBrake) {
      uint8 tempToSpeed;
      if (coolantTemp < 40) {
        tempToSpeed = (200 + ampl);
      } else {
        tempToSpeed = coolantTemp - 40;
      }
      debug("calculate tempToSpeed:", tempToSpeed);
      speedometer(tempToSpeed);
    }
  } else if ((ECN_VOLT == ecnMode) && (millis() > ecnMillis)) {
    // process voltage
    ecnMillis = millis() + ecnWaitTime;
    uint8 d0 = 0x00;
    uint8 d1 = 0x00;
    uint8 d2 = 0xEE;
    
    if (voltage < 100) {
      debug("<10,0");
      d2 = voltage;
    } else {
      debug(">10,0");
      d1 = 0x01;
      d2 = voltage - 100;
    }
    d2 = d2 / 10 * 16 + d2 % 10; // to hex but show like dec;
    lsShowEcn(d0, d1, d2);
    if (flagHandBrake) {
// todo !! make out to tacho
      debug("voltage-100 to tahometer");
      uint8 toTaho = (voltage>100)?(voltage-100):0;
      tahometer(toTaho);
    }
  }
}
// close void loop
