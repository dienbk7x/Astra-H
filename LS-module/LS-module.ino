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
bool ecnMode; // temporary, must be enum for state-machine
long ecnMillis = 0; // size?
short ecnWaitTime = 1000; // pause between ecn screen update in mode 1
int coolantTemp;

// Flags
volatile bool flag_blocked;

// Instanciation of CAN interface
HardwareCAN canBus(CAN1_BASE);
CanMsg msg ;
CanMsg *r_msg;

void setup()
{
  SERIAL.begin(115200);
  SERIAL.println("Hello World!");
  SERIAL.println("Starting LS-module v1.02 2018-11-22");
  debug("checking debug level");
  log("checking log level");

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
      if (1 == ecnMode) {
        coolantTemp = r_msg->Data[3] - 40;
      }
    } else if (r_msg->ID == 0x175) {
      debug("Steering wheel buttons");
      if ( (r_msg->Data[5] == 0x20) && (r_msg->Data[6] == 0x01) ) {
        //       left knob down
        debug("left knob down");
        ecnMode = 1;
        log("ECN mode on");
      } else if ( (r_msg->Data[5] == 0x10) && (r_msg->Data[6] == 0x1F) ) {
        debug("left knob down");
        ecnMode = 0;
        log("ECN mode off");
        lsShowEcn(0x0F, 0xF0, 0xFF); // temp
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
  if ((1 == ecnMode) && (millis() > ecnMillis)) {
    debug("(1 == ecnMode) && (millis() > ecnMillis)");
    debug("coolant");
    SERIAL.println(coolantTemp - 40); // !!!!!!!!!!! bad

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
  }
}
// close void loop
