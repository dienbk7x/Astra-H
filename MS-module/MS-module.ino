/**
 * Based on https://github.com/alex161rus/Opel-Astra-H (which is based on https://github.com/Gegerd/Astra-H)
 */

#include "HardwareCAN/HardwareCAN.h"
#include "includes/defines.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                         MODULE OPTIONS                                              //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

// DEBUG activates messages printout
// Debug activation will increase the ant of memory by ~16k
#define DEBUG

// whether to translate media buttons to HW output
//#define HW_MEDIA_CONTROL

// choose UART pins
#define UART Serial1
// Choose CAN pins
#define CAN_GPIO_PINS_MS CAN_GPIO_PB8_PB9

// Limit time to flag a CAN error
#define PC13ON 0
#define PC13OFF 1

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                         ASTRA H VARIABLES AND FUNCTIONS                             //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//****************************************Variables************************************//
bool key_acc = 0;
bool test_mode = 0;
bool alarm = 0;
bool Blink = 0;
bool REVERSE = 0; //задний ход вкл/выкл /// todo автор зря переменную ввёл большими буквами. не java-style
volatile bool flag_blocked; // to block AC Trigger function (advised to turn off CD30MP3)
int VOLTAGE = 131;
int p_VOLTAGE = 0;
int T_ENG = 1000;
int p_T_ENG = 0;
int SPEED = 0;
int RPM = 0;
int DAY = 0;
int MONTH = 0;
int YEAR = 0;
byte data2 = 0;
byte data4 = 0;
int RANGE = 0;
int p_RANGE = 0;
int window = 0;
int intCOutT;
char CTemp1 = 0;
char CTemp2 = 0;
char CNapr;
char CSpeed;
char CEco;
char COutT;
char p_CTemp1;
char p_CTemp2;
char p_CNapr;
char p_CSpeed;
char p_CEco;
char p_COutT;
uint32_t btn = 0;
uint32_t time_request_ecc = 0;
uint32_t time_send = 0;
uint32_t time_send_album = 0;
uint32_t time_send_artist = 0;
uint32_t Time_USART = 0;
uint32_t Time_Update_Message = 0;
String Prev_Message;
String Message_USART;
String message = "OPEL MEDIA SYSTEM";
String message_album = "";
String message_artist = "";
String p_message_album = "";
String p_message_artist = "";
String p_message = "";
String message_temp = "   ";
//********************************Tab function prototypes*****************************//
//Announcement of function prototypes from other tabs for correct function call.
void message_to_DIS (String);
void message_to_DIS_album (String);
void message_to_DIS_artist (String);
String Bold(String);
String Normal(String);
String Right(String);
String Central(String);
void CAN_message_process(CanMsg*);
String data_to_str(int, int);
String data_to_time(int);
//************************************************************************************//
//********************Filling an array with USART characters**************************
String Data_USART() {
  String Buffer_USART;
  char u;
  while (UART.available() > 0 && u != '\n') { //read serial buffer until \n
    char u = UART.read();
    if (u != 0xD) Buffer_USART += u;  // skip \r
  }
  Buffer_USART.remove(Buffer_USART.length() - 1);
  return Buffer_USART;
}
//*********************Generate alarm text******************************************
String Alarm(bool event) {
  if (event) return (data_to_str(T_ENG, 0));
  else       return ("   ");
}
//*************************************************************************************//
//Function prototypes and variables for working with CAN bus, function body in the "Init CAN" tab
#define CAN_RX_QUEUE_SIZE 36
HardwareCAN canBus(CAN1_BASE);
void CANSetup(void);
void SendCANmessage(long, byte, byte, byte, byte, byte, byte, byte, byte, byte);
void btn_function(byte, byte);
//*************************************************************************************//

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                                  SETUP FUNCTIONS                                    //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
void setup() {
  UART.begin(115200); // USART on
  CANSetup() ;

  pinMode(PC13, OUTPUT); // LED
  pinMode(PC14, OUTPUT); // LED ERR
#ifdef HW_MEDIA_CONTROL
  pinMode(PB12, OUTPUT_OPEN_DRAIN);
  pinMode(PB13, OUTPUT_OPEN_DRAIN);
  pinMode(PB14, OUTPUT_OPEN_DRAIN);
  pinMode(PB15, OUTPUT_OPEN_DRAIN);
  digitalWrite(PB12, HIGH);
  digitalWrite(PB13, HIGH);
  digitalWrite(PB14, HIGH);
  digitalWrite(PB15, HIGH);
#endif
  digitalWrite(PC13, PC13ON);
  digitalWrite(PC13, PC13OFF);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                                   LOOP FUNCTIONS                                    //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
void loop() {

#ifdef HW_MEDIA_CONTROL
  if ((millis() - btn) > 150) {
    digitalWrite(PB12, HIGH);
    digitalWrite(PB13, HIGH);
    digitalWrite(PB14, HIGH);
    digitalWrite(PB15, HIGH);
    digitalWrite(PC13, HIGH);
  }
#endif

  //*******************************Request for data from the ECC************************
  if ((key_acc == 1) &&  ((millis() - time_request_ecc) > 1000)) {
    SendCANmessage(0x0248, 8, 0x06, 0xAA, 0x01, 0x01, 0x07, 0x10, 0x11, 0x00);
    time_request_ecc = millis();
  }
  //*******************Generate an engine temperature alarm*****************************
  if (T_ENG > 1080) {
    alarm = 1;
    message_temp = Alarm(Blink);
  }
  else alarm = 0;
  if (alarm == 0) {
    message_temp = data_to_str(T_ENG, 0);
  }
  //*************************Receiving a message with USART****************************
  if ((key_acc == 1) && (millis() - Time_USART > 200)) {
    Message_USART = Data_USART();
    Time_USART = millis();
  }
  if ((key_acc == 1) && (Message_USART != "")) {
    if ( Message_USART == "run_bc") {
      btn_function(MS_BTN_BC, 0x00);
    }
    if ( Message_USART == "run_settings") {
      btn_function(MS_BTN_SETTINGS, 0x00);
    }
    if ((Message_USART != "run_bc") && ( Message_USART != "run_settings")) {
      message = Central(Bold(Message_USART));
    }
    Time_Update_Message = millis();
  }
  //******************************* Parameter display **********************************
  if ((key_acc == 1) && ((millis() - Time_Update_Message) > 500)) {
    if (test_mode == 1) {
      Message_USART = "TEST MODE";
    }
    else Message_USART = "OPEL MEDIA SYSTEM";
    Time_Update_Message = millis();
  }
  //***************Check CAN message buffer and process message*************************
  while (canBus.available() > 0) {
  CAN_message_process(canBus.recv());
    canBus.free();
  }
  //******************************* Update display string title **********************************
  if ((key_acc == 1) && ((millis() - time_send) > 500) || (p_message != message )) {
    message_to_DIS(message);
    p_message = message;
    time_send = millis();
  }
  //******************************* Update display string album **********************************
  if ((key_acc == 1) && ((millis() - time_send_album) > 500) || (p_message_album != message_album )) {
    message_album = Central((message_temp) + "°C" + " " + data_to_str(VOLTAGE, 1) + "V" + " " + String(SPEED) + "km/h" + " " + String(RPM) + "rpm");
    message_to_DIS_album(message_album);
    p_message_album = message_album;
    time_send_album = millis();
    if (Blink) {
      Blink = 0;
    }
    else Blink = 1;
  }
  //******************************* Update display string artist **********************************
  if ((key_acc == 1) && ((millis() - time_send_artist) > 500) || (p_message_artist != message_artist )) {
    message_artist = Central(("OCTATOK ") + String(RANGE) + "km" + "    " + data_to_time(DAY) + "." + data_to_time(MONTH) + "." + "20" + data_to_time(YEAR));
    message_to_DIS_artist(message_artist);
    p_message_artist = message_artist;
    time_send_artist = millis();
  }
}
