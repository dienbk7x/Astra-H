/*
   Uses STM32duino with Phono patch. Must add 33 and 95 CAN speeds => Megadrifter's edition
*/
#include <HardwareCAN.h>

#define __VERSION 1.14
#define __DATE 2019-06-14

/////// === Настройки модуля! === ///////

// Uncomment to enable 'debug()' messages output
#define DEBUG

// Uncomment to enable 'log()' messages output
#define LOG

// Choose output serial port
#define SERIAL Serial2

// Set timeout for sending CAN messages
#define CAN_SEND_TIMEOUT 200

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
  ECN_TEMP_VOLT,      // температура и напряжение 
  ECN_SPEED,          // точная скорость
  ECN_DOORS,          // мониторинг дверей 
  ECN_RETURN,         // крайний режим для возврата в ноль
  ECN_DOORS_AUTO,     // открытые двери (с возвратом в предыдущий режим)
  ECN_UNDERVOLTAGE,   // для низкого напряжения
  ECN_OVERHEAT,       // для перегрева
  ECN_STROBS          // для стробов ?
};
// enum EcnMode ecnMode = OFF; // temporary, must be enum for state-machine
// enum EcnMode savedEcnMode = OFF;
byte ecnMode = OFF; // temporary, must be enum for state-machine
byte savedEcnMode = OFF;

long ecnMillis = 0; // size?
short ecnWaitTime = 300; // pause between ecn screen update in mode 1
long btnMillis = 0; // size?
short btnWaitTime = 250; // pause between steering wheel buttons read
int coolantTemp;
int voltage = 0;
uint8 speed = 0; // up to 256
int taho = 0;

// Flags
volatile bool flagHandBrake = false; // флаг обнаружения поднятого ручника
volatile bool flagDoorsOpen = false; // флаг обнаружения открытой двери
volatile bool flagButtonPressed = false; // флаг для однократной обработки нажатия бобышки при переключении режима
//volatile bool flagDoorsAcknowledged = false; // флаг квитирования режима двери авто
volatile bool flag;  // флаг заготовка

// Instanciation of CAN interface
HardwareCAN canBus(CAN1_BASE);
CanMsg *r_msg;

void setup()
{
  delay(DELAY);
  SERIAL.begin(115200);
  SERIAL.println("Hello World!");
  SERIAL.println("Starting LS-module v __VERSION __DATE");
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
  delay(1000); // time to start engine
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
      // debug("0x100");
    }// do nothing

    else if (r_msg->ID == 0x108) { // speed + taho
      speed = (r_msg->Data[4]<<1) + (r_msg->Data[5]>>7);
      taho = (r_msg->Data[1]<<6) + (r_msg->Data[2]>>2);
      // 90 === 900rpm
      if (ECN_SPEED == ecnMode) {
        // gear factor. TODO: log and count gear
        uint8 gear = (speed * 10) / (taho/100); // 80*10 / 3000/100
        debug("speed: ", speed);
        debug("taho: ", taho);
        debug("gear factor:", gear);
        #ifdef DEBUG
            SERIAL.print("gear;");
            SERIAL.print(millis());
            SERIAL.print(";");
            SERIAL.print(taho);
            SERIAL.print(";");
            SERIAL.print(speed);
            SERIAL.println(";");
        #endif
        lsShowEcnDecimal(gear, speed); // experimental! needs to be checked
      }
    }

    else if (r_msg->ID == 0x135) { // open/close locks
    /*
    кнопку нажал (закрыть!)	  135	BC	00	FD	70
    кнопку отпустил	          135	3C	00	FD	70
    кнопку нажал (открыть!)		135	7C	00	FD	70
    кнопку отпустил	        	135	3C	00	FD	70
    открыть с ключа 	        135	7C	06	FD	70
    открыть с ключа 	        135	3C	06	FD	70
                                  |    |   \___\___ID ключа??
                                  |   06= с брелка
                                  |   00= с кнопки в кабине
                                  BC = нажать закрыть
                                  7C = нажать открыть
                                  3C = отпустить
    */
  //  todo закрытие стекол по троекратному нажатию закрывания.
    }

    else if (r_msg->ID == 0x145) { // engine tempr
      if (ECN_TEMP_VOLT == ecnMode) {
        // debug("mode = ", ecnMode);
        coolantTemp = r_msg->Data[3];
      }

    } else if (r_msg->ID == 0x175) { // Steering wheel buttons
      // debug("Steering wheel buttons");
      if ( (r_msg->Data[5] == 0x20) && (r_msg->Data[6] == 0x01) ) {
        //       left knob down
        // debug("left knob down");
        if (flagButtonPressed) {
        // ничего не делаем до отпускания
        } else { // если не была нажата, то переключаем и ставим флаг, что нажата кнопка
          ecnMode++; // работает только для int ((
          flagButtonPressed = true;
          debug("mode = ", ecnMode);
          log("ECN mode on / +1");
          #ifdef DEBUG
          lsBeep(ecnMode);
          #endif
        }

      } else if ( (r_msg->Data[5] == 0x10) && 
                  (r_msg->Data[6] == 0x1F) ) {  //   left knob up
//        if (OFF != ecnMode) {savedEcnMode = ecnMode;}
        ecnMode = OFF;
        log("ECN mode off");
        lsShowEcn(0x0F, 0xF0, 0xFF); 
      } else if ( (r_msg->Data[5] == 0x11) && 
                  (r_msg->Data[6] == 0x1F) && 
                  (r_msg->Data[7] == 0x01) ) {  // both knobs up
        lsDoThanks();
      } 

      if ( (r_msg->Data[5] == 0x10) && 
           (r_msg->Data[6] == 0x1F) &&
           (r_msg->Data[0] == 0x08)) {  // left knob up + lights pull
        ecnMode = ECN_STROBS;
      } 

      if (r_msg->Data[5] == 0x00) {
        flagButtonPressed = false;
      }

    } else if (r_msg->ID == 0x230) {

      if ( (ECN_DOORS == ecnMode) || (ecnMode == ECN_DOORS_AUTO) ) { // режим дверей? тогда выводим
        // todo insert timeout?
        // записать и отобразить // todo extract method
        uint8 d0 = 0x40; // 1 + front left
        uint8 d1 = 0x00; // rear left + bagage
        uint8 d2 = 0x00; // rear right + front right
        if (r_msg->Data[2] & 0x40) {d0 += 0x0F;}  // 1F0000
        if (r_msg->Data[2] & 0x10) {d2 += 0x0F;}  // 10000F
        if (r_msg->Data[2] & 0x04) {d1 += 0x0A;}  // 100A00
        if (r_msg->Data[1] & 0x40) {d1 += 0xb0;}  // 10b000
        if (r_msg->Data[1] & 0x10) {d2 += 0xb0;}  // 1000b0
        lsShowEcn(d0, d1, d2);
      } else { // другой режим -- тогда при открытых переключаем на авто
//        todo  сделать возможность переключения режима (flagDoorsAcknowledge ?)
          savedEcnMode = ecnMode;
          ecnMode = ECN_DOORS_AUTO;
      }

      if ( (r_msg->Data[2]) || (r_msg->Data[1]) ) { // не нули - двери открыты
//        flagDoorsOpen = true;
      } else { // нули - двери закрыты
//        flagDoorsOpen = false;
        if (ecnMode == ECN_DOORS_AUTO) {
          ecnMode = savedEcnMode;
        }
      }

/*
230   [7]  00 00 40 00 00 00 00 открыта левая дверь
230   [7]  00 00 10 00 00 00 00 открыта правая дверь
230   [7]  00 00 50 00 00 00 00 открыты обе двери
задние пассажирские в data[1]
*/
    
    } else if (r_msg->ID == 0x370) {
      // debug("handbrake, fog lights, etc...");
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
      // debug("voltage");
      if (ECN_TEMP_VOLT== ecnMode) {
        voltage = r_msg->Data[1]+28;
        // debug("read voltage");
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
//     debug("(1 == ecnMode) && (millis() > ecnMillis)");
    debug("Coolant: ", coolantTemp - 40);
    debug("voltage * 10: ", voltage);

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
//       debug("<10,0");
      d2 = voltage;
    } else {
//       debug(">10,0");
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
      // debug("calculate tempToSpeed:", tempToSpeed);
      speedometer(tempToSpeed);

      // debug("voltage-100 to tahometer");
      uint8 toTaho = (voltage>100)?(voltage-100):0;
      tahometer(toTaho);
    }
  } else if (ECN_DOORS == ecnMode) {
    // lsShowEcn(0x0d, r_msg->Data[2], r_msg->Data[3]); // уже выводится сразу же
  } else if (ECN_RETURN == ecnMode) {
    ecnMode = OFF; 
    lsShowEcn(0x0F, 0xF0, 0xFF);
  } else if (ECN_STROBS == ecnMode) {
    lsDoStrob();
  }
  
}
// close void loop
