/*
   Uses STM32duino with Phono patch. Must add 33 and 95 CAN speeds => Megadrifter's edition
*/
#include <HardwareCAN.h>

/////// === Настройки модуля! === ///////

// Uncomment to enable 'debug()' messages output
#define DEBUG  //  Please define also in utils.ino!!

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
#define DELAY 500

/////// === Глобальные переменные === ///////
/* global variables */
enum EcnMode {
  OFF=0, 
  ECN_TEMP_VOLT,
  ECN_SPEED,
  ECN_DOORS
};
// enum EcnMode ecnMode = OFF; // temporary, must be enum for state-machine
// enum EcnMode savedEcnMode = OFF;
byte ecnMode = OFF; // temporary, must be enum for state-machine
byte savedEcnMode = OFF;

long ecnMillis = 0; // size?
short ecnWaitTime = 500; // pause between ecn screen update in mode 1
long btnMillis = 0; // size?
short btnWaitTime = 250; // pause between steering wheel buttons read
uint8 coolantTemp;
uint8 voltage = 0;
uint8 speed = 0;

// Flags
volatile bool flagHandBrake = false;
volatile bool flagDoorsOpen = false;
volatile bool flag;

// Instanciation of CAN interface
HardwareCAN canBus(CAN1_BASE);
CanMsg msg ;
CanMsg *r_msg;

void setup()
{
  delay(DELAY);
  SERIAL.begin(115200);
  SERIAL.println("Hello World!");
  SERIAL.println("Starting LS-module v1.11 2019-06-01");
  debug("checking debug level");
  debug("checking debug with value", 1);
  debugHex("checking debugHex with value 32", 32);
  log("checking log level");
  delay(DELAY);

  debug("Setting globals");
  ecnMode = ECN_TEMP_VOLT;
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
  // playWithEcn();

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

    else if (r_msg->ID == 0x108) { // speed + taho
      speed = (r_msg->Data[1]<<1) + (r_msg->Data[2]>>7);
      // taho = (r_msg->Data[4]<<8) + (r_msg->Data[5]>>2)
      if (ECN_SPEED == ecnMode) {
      debug("speed ", speed);
      lsShowEcnDecimal(speed);
      }
    }

    else if (r_msg->ID == 0x145) { // engine tempr
      debug("engine tempr");
      if (ECN_TEMP_VOLT == ecnMode) {
        debug("mode = ", ecnMode);
        coolantTemp = r_msg->Data[3];
      }

    } else if (r_msg->ID == 0x175) { // Steering wheel buttons
      
      if ( millis() > btnMillis ) {
        btnMillis = millis() + btnWaitTime;
      } else break;

      debug("Steering wheel buttons");
      if ( (r_msg->Data[5] == 0x20) && (r_msg->Data[6] == 0x01) ) {
        //       left knob down
        debug("left knob down");
        ecnMode++; // работает только для int ((
        debug("mode = ",ecnMode);
        log("ECN mode on / +1");
        delay(100); // bad way to avoid multiple change
        #ifdef DEBUG
        lsBeep(ecnMode);
        #endif
      } else if ( (r_msg->Data[5] == 0x10) && 
                  (r_msg->Data[6] == 0x1F) ) {
        debug("left knob up");
        ecnMode = OFF;
        debug("mode = ",ecnMode);
        log("ECN mode off");
        lsShowEcn(0x0F, 0xF0, 0xFF); // temp
      } else if ( (r_msg->Data[5] == 0x11) && 
                  (r_msg->Data[6] == 0x1F) && 
                  (r_msg->Data[7] == 0x01) ) {
        debug("both knobs up");
        lsDoThanks();
      }

      if ( (r_msg->Data[5] == 0x10) && 
           (r_msg->Data[6] == 0x1F) &&
           (r_msg->Data[0] == 0x08)) {
        debug("left knob up + lights pull");
        lsDoStrob();
      }  

    } else if (r_msg->ID == 0x230) {
      debug("Doors/locks");
      printMsg();
      // flagDoorsOpen = false; // r_msg->Data[2] | r_msg->Data[3]
      if ((r_msg->Data[2]!=0x00) || 
          (r_msg->Data[3]!=0x00)) {
        flagDoorsOpen = true;
        if (ecnMode != ECN_DOORS) {
          savedEcnMode = ecnMode;
          ecnMode = ECN_DOORS;
        }
        lsShowEcn(0x0d, r_msg->Data[2], r_msg->Data[3]);
      } else {
        flagDoorsOpen = false;
        if (ecnMode == ECN_DOORS) {
          lsShowEcn(0x0d, r_msg->Data[2], r_msg->Data[3]);
          ecnMode = savedEcnMode;
        }
      }

/*
230   [7]  00 00 40 00 00 00 00 открыта левая дверь
230   [7]  00 00 10 00 00 00 00 открыта правая дверь
230   [7]  00 00 50 00 00 00 00 открыты обе двери
*/
    
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
      if (ECN_TEMP_VOLT== ecnMode) {
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
  if ((ECN_TEMP_VOLT == ecnMode) && (millis() > ecnMillis)) {
    debug("(1 == ecnMode) && (millis() > ecnMillis)");
    debug("Coolant: ", coolantTemp - 40);

    ecnMillis = millis() + ecnWaitTime;
    // process coolant
    uint8 d0 = 0x00; // temp[1,2]
    uint8 d1 = 0x00; // temp[3], volt[1]
    uint8 d2 = 0x00; // volt[2,3]

    uint8 ampl = 0;
    if (coolantTemp < 40) {
      ampl = 40 - coolantTemp; // amplitude
      d0 = 0xF0;
    } else {
      ampl = coolantTemp - 40;
      if (ampl < 100) {
        d0 = 0xC0;
      } else {
        d0 = 0x10;
        ampl = ampl - 100;
      }
    }
    d0 += ampl/10;
    d1 += ampl % 10 * 16;
    // d2 += ampl / 10 * 16 + ampl % 10; // to hex but show like dec;
    // lsShowEcn(d0, d1, d2);

    // process voltage
    if (voltage < 100) {
      debug("<10,0");
      d2 = voltage;
    } else {
      debug(">10,0");
      d1 += 0x01;
      d2 = voltage - 100;
    }
    d2 = d2 / 10 * 16 + d2 % 10; // to hex but show like dec;
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

      debug("voltage-100 to tahometer");
      uint8 toTaho = (voltage>100)?(voltage-100):0;
      tahometer(toTaho);
    }
  } else if (ECN_DOORS == ecnMode) 
  {
    // lsShowEcn(0x0d, r_msg->Data[2], r_msg->Data[3]);
  }
  
}
// close void loop
