/*
   Uses STM32duino with Phono patch. Must add 33 and 95 CAN speeds => Megadrifter's edition
*/
#include <HardwareCAN.h>
#include "includes/ls_defines.h"
#include "includes/ls_module.h"

String VERSION = "1.23";
String DATE = "2020-06-10";

//#define ARROWS_TEST
/////// ============= Настройки модуля! | User settings! ============= ///////
// todo Вынести в settings.h
// Uncomment to enable 'debug()' messages output
#define DEBUG

// Uncomment to enable 'log()' messages output
#define LOG

// Choose output serial port
#define UART Serial2

// Set timeout for sending CAN messages
#define CAN_SEND_TIMEOUT 200

// Choose CAN pins for each bus to enable dual
#define CAN_GPIO_PINS_MS CAN_GPIO_PA11_PA12
#define CAN_GPIO_PINS_LS CAN_GPIO_PB8_PB9


/////// ============= Настройки системы | System settings ============= ///////
// Built-in LED
#define PC13ON 0
#define PC13OFF 1

// default delay?
#define DELAY 500

String line10 = "====================";


/////// ============= Глобальные переменные | Global variables ============= ///////
/* global variables */
enum EcnMode {
  OFF=0,
  ECN_TEMP_VOLT,      // температура и напряжение
  ECN_SPEED,          // точная скорость
  ECN_SPORT,          // точная скорость + спорт
  ECN_ESP_OFF,        // спорт -ESP
  ECN_DOORS,          // мониторинг дверей
  ECN_RETURN,         // крайний режим для возврата в ноль
  ECN_DOORS_AUTO=20,     // открытые двери (с возвратом в предыдущий режим)
  ECN_UNDERVOLTAGE,   // для низкого напряжения
  ECN_OVERHEAT,       // для перегрева
  ECN_STROBS,         // для стробов
  ECN_STOPS          // для дежопинга
};
// EcnMode ecnMode = OFF; // temporary, must be enum for state-machine
// EcnMode savedEcnMode = OFF;
byte ecnMode = OFF; // temporary, must be enum for state-machine
byte savedEcnMode = OFF;

//byte activeBus = 0;
enum ActiveBus {
  NO_BUS = 0,
  LS_BUS = 1,
  MS_BUS = 2
};
ActiveBus activeBus = NO_BUS; // try "enum" as class

/* Remote key  */
uint8 keyState = 0x00;
char keyNum = 0;
byte keyCode0 = 0x00;
byte keyCode1 = 0x00;

/* ECN msg sending interval */
long ecnMillis = 0; // size?
short ecnWaitTime = 300; // pause between ecn screen update in mode 1
/* for left knob distinct operation */
long btnMillis = 0; // size?
short btnWaitTime = 250; // pause between steering wheel buttons read

/* Vehicle active parameters */
int coolantTemp;
int voltage = 0;

/* Speed and all this stuff */
byte gear = 0;
uint8 gearFactor = 0;
byte recommendedGear = 0;
uint8 speed = 0; // up to 256
uint8 speedPrev = 0; // up to 256
uint8 speed400 = 0; // up to 256
uint8 speed400Prev = 0; // up to 256
short dV = 0; // speed increace or decreace
short dV400 = 0; // speed increace or decreace at 400 ms interval
long dVMillisPrev = 0;
long dtSpeed400 = 0;
float accelG = 0;

int taho = 0;

String msg; // for better logging

/* for incoming messages */
String messageUart;
uint32_t timeUart = 0; //Variable for the USART buffer fill timer


// Flags
volatile bool flagHandBrake = false; // флаг обнаружения поднятого ручника
volatile bool flagDoorsOpen = false; // флаг обнаружения открытой двери
volatile bool flagButtonPressed = false; // флаг для однократной обработки нажатия бобышки при переключении режима
volatile bool flagDoorsAcknowledge = false; // флаг квитирования режима двери авто
volatile bool flagThrottle = false;  // флаг нажатой педали газа
volatile bool flagBackwards = false;  // флаг заднего хода
volatile bool flagFastBraking = false;  // флаг быстрого снижения скорости
volatile bool flagUartReceived = false;  // флаг заготовка
volatile bool flagTopStopSignal = false;  // Горит верхний стоп (был активирован программно, гаснет через 4 секунды сам)

volatile bool flagSportOn = true;  // флаг спорт режима
long sportMillis = 0; // size?
short sportWaitTime = 500; // pause between sport mode message // may be eliminated
long espOffMillis = 0; // size?
short espOffWaitTime = 500; // pause between sport mode message  // may be eliminated

volatile bool flag = false;  // флаг заготовка

// Saved data of different IDs
uint8 lsId305Data[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Instanciation of CAN interface
HardwareCAN canBus(CAN1_BASE);
CanMsg *r_msg;

void setup()
{
  delay(DELAY);
  UART.begin(115200);
  UART.println("Hello World!");
  UART.print("Starting LS-module v ");
  UART.print(VERSION);
  UART.print(" ");
  UART.println(DATE);
  #ifdef DEBUG
  debug("checking debug level");
  debug("checking debug with value", 1);
  debugHex("checking debugHex with value 32", 32);
  #endif
  #ifdef LOG
  log("checking log level");
  #endif
  delay(DELAY);

  #ifdef DEBUG
  debug("Setting globals");
  #endif
/////// ============= SET INITIAL MODE HERE =============   ///////
//  ecnMode = ECN_TEMP_VOLT;

  coolantTemp = 40;

  pinMode(PC13, OUTPUT); // LED is ON until end of setup()
  digitalWrite(PC13, PC13ON);
  delay(50);// to see the LED
  log("Initialize the CAN module ...");

  lsCANSetup();        // Initialize the CAN module to LS CAN bus
  log("Initialization LS CAN ON");
  wakeUpBus();
  lsBeep(2);
//  delay(1000); // time to start engine
  #ifdef ARROWS_TEST 
  panelCheck();
  #endif
  // playWithEcn();

  digitalWrite(PC13, PC13OFF);
}

void loop()
{
  //  debug("loop");
  // ======== receive message and set flags ===================================================================
  while ( ( r_msg = canBus.recv() ) != NULL )
 {
    ///// processing the incoming CAN message
    if (r_msg->ID == 0x100) { // wake-up message
      // debug("0x100");
    }// do nothing
//######################################################################################################
     else if (r_msg->ID == LS_ID_KEY)
     { // key position
//    else if (r_msg->ID == 0x170) { // key position
         if (keyState != r_msg->Data[LS_KEY_DATA_BYTE])
         {
           keyState = r_msg->Data[LS_KEY_DATA_BYTE];

           switch (keyState) {
    //      switch (r_msg->Data[LS_KEY_DATA_BYTE]) {
            case 0:
              UART.print("keyState not set!!");
              break;

            case KEY_LOCKED:
              ecnMode = OFF;
              break;

            case KEY_IGN_OFF:
              break;

            case KEY_IGN_ON:
              ecnMode = ECN_TEMP_VOLT;
              break;

            case KEY_STARTER_ON:
               #ifdef DEBUG
               debug("KEY_STARTER_ON");
               #endif
              delay(1800); // delay to pass voltage drop at starter run
              lsBeep(1);
              break;

            case KEY_STARTER_OFF:
              break;

            default:
              break;
          }
      }
    }
//######################################################################################################
    else if ((r_msg->ID == 0x108) /*&& ((keyState & KEY_IGN) == KEY_IGN)*/ ) { // speed + taho
      taho = (r_msg->Data[1]<<6) + (r_msg->Data[2]>>2);
      // 90 === 900rpm
      speedPrev = speed;
      speed = (r_msg->Data[4]<<1) + (r_msg->Data[5]>>7);
      dV = speed - speedPrev; // usually 0, 1 or -1
      flagThrottle = (r_msg->Data[0] & 0x20)?true:false; // не жестко соответствует педалированию


      //--
        dtSpeed400 = millis() - dVMillisPrev;

        // process low speed  //
        if ((speed < 6) && !flagBackwards) {
            msg = "LOWSP";
            lsTopStopSignalSet(true); // включаю верхний стоп

        } else if (dtSpeed400 > 380) { // если прошло 400 и более миллисекунд


          if (true == flagBackwards) {
            #ifdef DEBUG
            debug("BACKW");
            #endif

            lsTopStopSignalBlink(2, 50); // мигалка стопа на задний ход
            msg = "BACKW";
          } else

      //-------------- process decelerations by 400 ms--------------//
                 if (dtSpeed400 < 1000) { // если более, то начинаем сначала без обработки
            dV400 = speed - speed400Prev; // измеряем разницу с текущим


            // check for speed down without active braking and with released throttle
            if ((dV400 < 0) && (flagThrottle == false)) { // если торможение двигателем
              msg = "DECEL";
              lsTopStopSignalSet(true); // включаю верхний стоп
            } else if ((dV400 < -1) && (ECN_SPORT == ecnMode)) { // если торможение двигателем
              lsBeep(0x1e, 0x02, 0x04);
              msg = "DECEL2";
              lsTopStopSignalSet(true); // включаю верхний стоп

            } else if (dV400 > 0) { // тупо разгон пошел
              msg = "ACCEL";
              lsTopStopSignalSet(false);
            }

            // check high deceleration
        #ifdef DEBUG
            accelG = dV400  /3.6  * 1000 / dtSpeed400 / 9.8; // it is of float type
        #endif
            //  for more accuracy may need to monitor dTAHO/dt but only when transmission is jointed

            if ( dV400 < -8 ) {
//            if ( accelG < -0.50 ) { // при торможении сильнее 0,50 g -- можно и без расчета, по dV400
            //   dV400   g calc
            //    -11   -0.78
            //    -10   -0.71
            //     -9   -0.64 *
            //     -8   -0.57
            //     -7   -0.48
            //
              #ifdef DEBUG
//              debug("back turn lights on");
if (ECN_SPORT == ecnMode) {
              lsBeep(0x04);
} /////// end if ECN_MODE_PLUS
              msg = "EMRBR";
              #endif
              lsBackTurnLights1000(); // зажечь задние поворотники на 1000 мс
              flagFastBraking = true;
            } else {flagFastBraking = false;}
          }

          dVMillisPrev = millis(); //+ 400; // hardcoded interval !! Increment time
          speed400Prev = speed; // запоминаем предыдущее значение скорости
        }
      //--
      //-------------- END process decelerations by 400 ms--------------//
      if ((ECN_SPEED == ecnMode) || (ECN_SPORT == ecnMode) || (ECN_ESP_OFF == ecnMode)) {
        // ------ gear ------------

        uint8 gears = 0;
        uint8 brakes = 0;

        if ((speed > 3) && (ECN_ESP_OFF != ecnMode)) { // пока не завязался на сцепление, отсекаем околонулевую скорость
        // byte calculateGear(uint8 speed, int taho) {
          gearFactor = (speed * 10) / (taho/100); // 80*10 / 3000/100
          if (gearFactor < 10) { // определяем передачу
            gear = 1;
          } else if (gearFactor < 15) {
            gear = 2;
          } else if (gearFactor < 25) {
            gear = 3;
          } else if (gearFactor < 30) {
            gear = 4;
          } else if (gearFactor < 35) {
            gear = 5;
          }
         // return gear
         // }

        // byte recommendGear(byte gear, int taho) {
          recommendedGear = gear;
          if (taho > 3100) {
            recommendedGear = gear + 1;
          } else if (taho < 1800) {
            if (gear>0) {
              recommendedGear = gear - 1;
            }
          }
          gears = 10*recommendedGear + gear;
         // return gear
         // }
        // ------ END gear ------------
        }

        if (ECN_ESP_OFF == ecnMode){
//            lsShowEcn(0x0F,0xFE,0x52); // alike "OFF ESP"
            msg = "ESPOF";

        } else {
            if (flagTopStopSignal) {brakes+=100;}
            if (flagFastBraking) {brakes+=200;}
//            gears += brakes;
            lsShowEcnDecimal(brakes+gears, speed);
        }

        #ifdef DEBUG
        // todo add logTimeout, log
            UART.print(millis());
            UART.print(";tah;");
            UART.print(taho);
            UART.print(";spd;");
            UART.print(speed);

//            UART.print(";gearFactor;");
//            UART.print(gearFactor);
            UART.print(";gear;");
            UART.print(gear);

            UART.print(";dV;");
            UART.print(dV);
            UART.print(";dV400;");
            UART.print(dV400);
            UART.print(";dt400;");
            UART.print(dtSpeed400);
            UART.print(";g;");
            UART.print(accelG);

            UART.print(";Gas;");
            UART.print(flagThrottle); // ?"1":"0"
            UART.print(";Backw;");
            UART.print(flagBackwards);

            UART.print(";EmBrk;");
            UART.print(flagFastBraking);
            UART.print(";TopStop;");
            UART.print(flagTopStopSignal);
            UART.print(";keyState;");
            UART.print(keyState);

            UART.print(";");
            UART.print(msg);
            UART.println(";");
        #endif
        msg = "     ";
//        flagTopStopSignal = false;
      }

    /*
    } else if (r_msg->ID == 0x135) { // open/close locks
    кнопку нажал (закрыть!)   135 BC  00  FD  70
    кнопку отпустил	          135 3C  00  FD  70
    кнопку нажал (открыть!)   135 7C  00  FD  70
    кнопку отпустил	          135 3C  00  FD  70
    открыть с ключа 	      135 7C  06  FD  70
    открыть с ключа 	      135 3C  06  FD  70
                                  |    |   \___\___ID ключа??
                                  |   06= с брелка
                                  |   00= с кнопки в кабине
                                  BC = нажать закрыть
                                  7C = нажать открыть
                                  3C = отпустить
    */

//######################################################################################################
    } else if (r_msg->ID == 0x145) { // engine tempr
      if (ECN_TEMP_VOLT == ecnMode) {
        // debug("mode = ", ecnMode);
        coolantTemp = r_msg->Data[3];
      }

//######################################################################################################
    // закрытие стекол при постановке на охрану.
    } else if (r_msg->ID == 0x160) { // open/close from distance
#ifdef DEBUG
printMsg();
#endif
      keyNum = r_msg->Data[0];
      keyCode0 = r_msg->Data[2];
      keyCode1 = r_msg->Data[3];

      if (r_msg->Data[1]==0x80) { // press close 2-nd time
        lsCloseWindows();

      } else if (r_msg->Data[1]==0x20)  { // press open 2-nd time
        lsOpenWindows(true); // half open
      }

//######################################################################################################
    } else if (r_msg->ID == 0x175) { // Steering wheel buttons
      // debug("Steering wheel buttons");

      if ( (r_msg->Data[5] == 0x20) && (r_msg->Data[6] == 0x01) ) { // left knob down
        // debug("left knob down");
        if (flagButtonPressed) {
        // ничего не делаем до отпускания
        } else { // если не была нажата, то переключаем и ставим флаг, что нажата кнопка

          if (ecnMode == ECN_DOORS_AUTO)  { // режим дверей? тогда
            flagDoorsAcknowledge = true;
            ecnMode = savedEcnMode;
          } else {
            ecnMode++; // работает только для int ((
          }

          flagButtonPressed = true;
          #ifdef DEBUG
          debug("mode = ", ecnMode);
          lsBeep(ecnMode);
          #endif
        }

      } else if ( (r_msg->Data[5] == 0x10) &&
           (r_msg->Data[6] == 0x1F) &&
           (r_msg->Data[0] == 0x08)) {  // left knob up + lights pull
        ecnMode = ECN_STROBS;
        flagButtonPressed = true;

      } else if ( (r_msg->Data[5] == 0x11) &&
                  (r_msg->Data[6] == 0x1F) &&
                  (r_msg->Data[7] == 0x01) ) {  // both knobs up
        lsDoThanks();

      } else if ( (r_msg->Data[5] == 0x22) &&
                  (r_msg->Data[6] == 0x01) &&
                  (r_msg->Data[7] == 0x1F) ) {  // both knobs down
        ecnMode = ECN_STOPS;          // для дежопинга

      } else if ( (r_msg->Data[5] == 0x10) &&
                  (r_msg->Data[6] == 0x1F) &&
                  (OFF != ecnMode)) {  //   left knob up
//        {savedEcnMode = ecnMode;}
        if (flagButtonPressed) {
        // ничего не делаем до отпускания
        } else { // если не была нажата, то переключаем и ставим флаг, что нажата кнопка
        // косяк, что отпускание всегда отключает режим!
          ecnMode = OFF;
          flagButtonPressed = true;
          log("ECN mode off");
          lsShowEcn(0x0F, 0xF0, 0xFF);
        }

      }

      if (r_msg->Data[5] == 0x00) {
        flagButtonPressed = false;
      }

//######################################################################################################
    } else if (r_msg->ID == 0x230) { // doors status

      if ( (ECN_DOORS == ecnMode) || (ecnMode == ECN_DOORS_AUTO) ) { // режим дверей? тогда выводим
        // todo insert timeout?
        // записать и отобразить // todo extract method
        uint8 d0 = 0x10; // 1 + front left
        uint8 d1 = 0x00; // rear left + bagage
        uint8 d2 = 0x00; // rear right + front right
        if (r_msg->Data[2] & 0x40) {d0 += 0x0F;}  // 1F0000
        if (r_msg->Data[2] & 0x10) {d2 += 0x0F;}  // 10000F
        if (r_msg->Data[2] & 0x04) {d1 += 0x0A;}  // 100A00
        if (r_msg->Data[1] & 0x40) {d1 += 0xb0;}  // 10b000
        if (r_msg->Data[1] & 0x10) {d2 += 0xb0;}  // 1000b0
        lsShowEcn(d0, d1, d2);
      } else { // другой режим -- тогда <при открытых> переключаем на авто
          if (flagDoorsAcknowledge == false) {
            savedEcnMode = ecnMode;
            ecnMode = ECN_DOORS_AUTO;
          }
      }

      if ( (r_msg->Data[2]) || (r_msg->Data[1]) ) { // не нули - двери открыты
//        flagDoorsOpen = true;
      } else { // нули - двери закрыты
        flagDoorsAcknowledge = false;//        flagDoorsOpen = false;
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
    
//######################################################################################################
    } else if (r_msg->ID == 0x305) { // IPC (central buttons + lights)
      for(byte i = 0; i<8; i++) {
          lsId305Data[i] = r_msg->Data[i];
      }
      #ifdef DEBUG
      printMsg();
      #endif

      if (flagButtonPressed && (r_msg->Data[6] & 0x01)  && (speed == 0)) {
          lsOpenRearDoor();
      }

      if (ECN_SPORT == ecnMode) {
          lsSendSportOn();
          lsIpcIndicatorNotFastenedOn();
          lsIpcIndicatorSportOn();
      }
      else if (ECN_ESP_OFF == ecnMode) {
          lsSendEspOff();
          lsShowEcn(0x0F,0xFE,0x52); // alike "OFF ESP"
          lsIpcIndicatorNotFastenedOn();
          lsIpcIndicatorSportOn();
      }
      else {
          // lsIpcIndicatorSportOff();
          lsIpcIndicatorNotFastenedOff();
          // lsIpcIndicatorEspOff();
      }
//######################################################################################################
    } else if (r_msg->ID == 0x350) { // backwards drive direction
      if ((r_msg->Data[0]) & 0x10) {
        flagBackwards = true;
      } else {
        flagBackwards = false;
      }

//######################################################################################################
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

//######################################################################################################
    } else if (r_msg->ID == 0x500) { // voltage
      // debug("voltage");
      if (ECN_TEMP_VOLT== ecnMode) {
        voltage = r_msg->Data[1]+28;
        // debug("read voltage");
      }
      //========================
      //    } else if (r_msg->ID == 0x) {
      //if (r_msg->Data[1])
//######################################################################################################
    } else {
    }  // end if
//######################################################################################################
//######################################################################################################

    canBus.free();

  }
  // close while
  // ======== check flags and execute actions ======================================================================
//######################################################################################################
  if ((ECN_TEMP_VOLT == ecnMode) && (millis() > ecnMillis)) {
    UART.print("Cool;");
    UART.print(coolantTemp - 40);
    UART.print(";Volt;");
    UART.println(voltage);
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
//######################################################################################################
  } else if (ECN_DOORS == ecnMode) {
    // lsShowEcn(0x0d, r_msg->Data[2], r_msg->Data[3]); // уже выводится сразу же
//######################################################################################################
  } else if (ECN_RETURN == ecnMode) {
    ecnMode = OFF; 
    lsShowEcn(0x0F, 0xF0, 0xFF);
//######################################################################################################
  } else if (ECN_STROBS == ecnMode) {
    lsDoStrob();
//######################################################################################################
  } else if (ECN_STOPS == ecnMode) {
    lsDoStops();
//######################################################################################################
  } else if ((ECN_SPORT == ecnMode) && (millis() > sportMillis)) {
    sportMillis = millis() + sportWaitTime;
        lsSendSportOn();
        lsIpcIndicatorNotFastenedOn();
        lsIpcIndicatorSportOn();
//######################################################################################################
  } else if ( (ECN_ESP_OFF == ecnMode)  && (millis() > espOffMillis)) {
    espOffMillis = millis() + espOffWaitTime;
        debug("SEND ESP OFF");
        lsSendEspOff();
        lsShowEcn(0x0F,0xFE,0x52); // alike "OFF ESP"
        lsIpcIndicatorNotFastenedOn();
        lsIpcIndicatorSportOn();
//######################################################################################################
  }
//######################################################################################################
//######################################################################################################

  // ======== Receive a message from UART =======================================================================
  if ((millis() - timeUart > 200) && (!flagUartReceived)) {   //delay needed to fillup buffers
    messageUart = readUart();
//    flagUartReceived = true;
    timeUart = millis();
  }
  if ((messageUart != "")) { // recognize and execute command
  UART.print("received message: ");
  UART.println(messageUart);
      if (messageUart=="help") {
        ecnMode = 0;
        log(line10);
        log("List of commands:");
        log("sniffer");
        log("mode00");
        log("mode++");
        log("lsCANSetup");
        log("wakeUpBus");
        log("playWithEcn");
        log("panelCheck");
        log("lsBeep");
        log("lsDoThanks");
        log("BackTurn1000");
        log("lsTopStopSignalSet");
        log("lsTopStopSignalSwitch");
        log("lsDoStrob");
        log("lsDoStops");
        log("lsCloseWindows");
        log("lsOpenWindows");
        log("lsOpenWindows2");
        log("lsOpenRearDoor");
        log("SportOn");
        log("EspOff");
//        log("send:id:d0:d1:d2:d3:d4:d5:d6:d7");
        delay(3500);
      } else if (messageUart=="sniffer") {
        lsSniffer();
      } else if (messageUart=="lsDoStrob") {
        lsDoStrob();
      } else if (messageUart=="lsDoStops") {
        lsDoStops();
      } else if (messageUart=="mode00") {
        ecnMode = OFF;
      } else if (messageUart=="mode++") {
        ecnMode++;
      } else if (messageUart=="lsCANSetup") {
        lsCANSetup();
      } else if (messageUart=="wakeUpBus") {
        wakeUpBus();
      } else if (messageUart=="playWithEcn") {
        playWithEcn();
      } else if (messageUart=="panelCheck") {
        panelCheck();
      } else if (messageUart=="lsBeep") {
        lsBeep();
      } else if (messageUart=="lsDoThanks") {
        lsDoThanks();
      } else if (messageUart=="BackTurn1000") {
        lsBackTurnLights1000();
      } else if (messageUart=="lsTopStopSignalSet") {
        lsTopStopSignalSet(true);
      } else if (messageUart=="lsTopStopSignalSwitch") {
        lsTopStopSignalSwitch();
      } else if (messageUart=="lsCloseWindows") {
        lsCloseWindows();
      } else if (messageUart=="lsOpenWindows") {
        lsOpenWindows();
      } else if (messageUart=="lsOpenWindows2") {
        lsOpenWindows(true);
      } else if (messageUart=="lsOpenRearDoor") {
        lsOpenRearDoor();
      } else if (messageUart=="SportOn") {
        ecnMode = ECN_SPORT;
      } else if (messageUart=="EspOff") {
        ecnMode = ECN_ESP_OFF;
      }
      messageUart = "";
//      flagUartReceived = false;
    }

}
// close void loop
