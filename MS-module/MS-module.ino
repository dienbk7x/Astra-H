#include "HardwareCAN/HardwareCAN.h"
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                                  ASTRA H DEFINITIONS                                //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
// Define the values of the MS CAN identifiers
#define MS_MEDIA_BUTTONS_ID 0x201
#define MS_BTN_BC 0x01
#define MS_BTN_1 0x31
#define MS_BTN_2 0x32
#define MS_BTN_3 0x33
#define MS_BTN_4 0x34
#define MS_BTN_5 0x35
#define MS_BTN_6 0x36
#define MS_BTN_7 0x37
#define MS_BTN_8 0x38
#define MS_BTN_9 0x39
#define MS_BTN_OK 0x6F
#define MS_BTN_RIHGT 0x6C
#define MS_BTN_LEFT 0x6D
#define MS_BTN_CD_RADIO 0xE0
#define MS_BTN_SETTINGS 0xFF
#define MS_WHEEL_BUTTONS_ID 0x206
#define MS_BTN_STATION 0x81
#define MS_BTN_MODE 0x82
#define MS_BTN_LEFT_WHEEL_TURN 0x83
#define MS_BTN_LEFT_WHEEL 0x84
#define MS_BTN_NEXT 0x91
#define MS_BTN_PREV 0x92
#define MS_BTN_VOL 0x93
#define MS_MEDIA_ID 0x6C1
#define MS_CLIMATE_INFO_ID 0x6C8
#define MS_ECC_ID 0x548
#define MS_BATTERY 0x07
#define MS_ENGINE_TEMP 0x10
#define MS_RPM_SPEED 0x10
#define MS_IGNITION_STATE_ID 0x450
#define MS_IGNITION_NO_KEY 0x00
#define MS_IGNITION_KEY_PRESENT 0x04
#define MS_IGNITION_1 0x05
#define MS_IGNITION_START 0x06
#define MS_SPEED_RPM_ID 0x4E8
#define MS_RANGE_ID 0x4EE
#define MS_BACKWARDS_BIT 0x04
#define MS_CLEAR_DISPLAY 0x0691
#define MS_WAKEUP_ID 0x697
#define MS_TIME_CLOCK_ID 0x180
#define MS_WINDOW_ID 0x2B0
#define MS_TEMP_OUT_DOOR_ID 0x683
// Define buttons state bytes
#define BTN_PRESSED 0x01
#define BTN_RELEASED 0x00
#define WHEEL_PRESSED 0x08
#define WHEEL_TURN_DOWN 0xFF
#define WHEEL_TURN_UP 0x01
// Limit time to flag a CAN error
#define CAN_TIMEOUT 100
#define CAN_DELAY 0 // 10        // ms between two processings of incoming messages
#define CAN_SEND_RATE 10 // 200   // ms between two successive sendings
#define PC13ON 0
#define PC13OFF 1
#define DELAY 250
//#define DEBUG  //Debug activation will increase the ant of memory by ~16k

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                         ASTRA H VARIABLES AND FUNCTIONS                             //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//****************************************Variables************************************//
bool key_acc = 0;
bool test_mode = 0;
bool alarm = 0;
bool Blink = 0;
bool REVERSE = 0; //задний ход вкл/выкл
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
//void message_to_DIS (String);
//void message_to_DIS_album (String);
//void message_to_DIS_artist (String);
//String Bold(String);
//String Normal(String);
//String Right(String);
//String Central(String);
//void CAN_message_process(CanMsg*);
//String data_to_str(int, int);
//String data_to_time(int);
//************************************************************************************//

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                             CAN-BUS VARIABLES AND FUNCTIONS                         //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//********************************************************************************************//
//FOR CORRECT WORK, IT IS REQUIRED TO INCLUDE CAN_MCR_TXFP In the HardwareCAN.cpp FILE line 26//
//********************************************************************************************//
void CANSetup(void)
{
  CAN_STATUS Stat ;
  // Initialize CAN module
  canBus.map(CAN_GPIO_PB8_PB9);
  Stat = canBus.begin(CAN_SPEED_95, CAN_MODE_NORMAL);
  //canBus.filter(0, CAN_FIFO0, 0, 0); //без фильтра
  canBus.filter(0, CAN_FIFO0, MS_WINDOW_ID, 0xFFFFFFFF);
  canBus.filter(1, CAN_FIFO1, MS_WHEEL_BUTTONS_ID, 0xFFFFFFFF);
  canBus.filter(2, CAN_FIFO0, MS_ECC_ID, 0xFFFFFFFF);
  canBus.filter(3, CAN_FIFO1, MS_MEDIA_ID, 0xFFFFFFFF);
  canBus.filter(4, CAN_FIFO0, MS_IGNITION_STATE_ID, 0xFFFFFFFF);
  canBus.filter(5, CAN_FIFO1, MS_TIME_CLOCK_ID, 0xFFFFFFFF);
  canBus.filter(6, CAN_FIFO0, MS_SPEED_RPM_ID, 0xFFFFFFFF);
  canBus.filter(7, CAN_FIFO1, MS_RANGE_ID, 0xFFFFFFFF);
  canBus.filter(8, CAN_FIFO0, MS_TEMP_OUT_DOOR_ID, 0xFFFFFFFF);
  canBus.filter(9, CAN_FIFO1, MS_CLIMATE_INFO_ID, 0xFFFFFFFF);
  canBus.set_irq_mode();
  nvic_irq_set_priority(NVIC_CAN_RX1, 0);
  nvic_irq_set_priority(NVIC_USB_LP_CAN_RX0, 0);
  nvic_irq_set_priority(NVIC_USART2, 1);
  Stat = canBus.status();
#ifdef DEBUG
  if (Stat != CAN_OK) Serial2.print("Initialization failed");
#endif
}

void SendCANmessage(long id = 0x100, byte dlength = 8, byte d0 = 0x00, byte d1 = 0x00, byte d2 = 0x00, byte d3 = 0x00, byte d4 = 0x00, byte d5 = 0x00, byte d6 = 0x00, byte d7 = 0x00)
{ CanMsg msg ;
  msg.IDE = CAN_ID_STD;
  msg.RTR = CAN_RTR_DATA;
  msg.ID = id ;
  msg.DLC = dlength;
  msg.Data[0] = d0 ;
  msg.Data[1] = d1 ;
  msg.Data[2] = d2 ;
  msg.Data[3] = d3 ;
  msg.Data[4] = d4 ;
  msg.Data[5] = d5 ;
  msg.Data[6] = d6 ;
  msg.Data[7] = d7 ;
  uint32_t time = millis();
  CAN_TX_MBX MBX;
  do    {
    MBX = canBus.send(&msg);
  }
  while ((MBX == CAN_TX_NO_MBX) && ((millis() - time) < 200)) ;
  if ((millis() - time) >= 200) {
    canBus.cancel(CAN_TX_MBX0);
    canBus.cancel(CAN_TX_MBX1);
    canBus.cancel(CAN_TX_MBX2);
#ifdef DEBUG
    Serial2.print("CAN-Bus send error");
#endif
  }
}
void btn_function(byte data4, byte data2) {
  SendCANmessage(0x201, 3, 0x01, data4, data2);
  SendCANmessage(0x201, 3, 0x00, data4, data2);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                       TRANSFORM TEXT AND FUNCTIONS SEND TO DIS                      //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

static byte data[255];  //Data Buffer for Package Preparation
static int data_lenght = 5;  //Useful length of data array for package preparation

// ******************* UTF8-Decoder: convert UTF8-string to extended ASCII ***********************
static byte c1;  // Last character buffer
// Convert a single Character from UTF8 to Extended ASCII
// Return "0" if a byte has to be ignored

byte utf8ascii(byte ascii) {
  if ( ascii < 128 ) // Standard ASCII-set 0..0x7F handling
  { c1 = 0;
    return ( ascii );
  }
  // get previous input
  byte last = c1;   // get last char
  c1 = ascii;       // remember actual character
  switch (last)     // conversion depending on first UTF8-character
  { case 0xC2: return  (ascii);  break;
    case 0xC3: return  (ascii | 0xC0);  break;
    case 0x82: if (ascii == 0xAC) return (0x80);   // special case Euro-symbol
  }
  return  (0);                                     // otherwise: return zero, if character has to be ignored
}
// convert String object from UTF8 String to Extended ASCII
String utf8ascii(String s)
{
  String r = "";
  char c;
  for (int i = 0; i < s.length(); i++)
  {
    c = utf8ascii(s.charAt(i));
    if (c != 0) r += c;
  }
  return r;
}
// ******************* Append bytes to the end of the array ***********************
void byte_append(byte add_byte)
{
  if (data_lenght < 254)
  {
    data[data_lenght] = add_byte;
    data_lenght += 1;
  }
}
// ******************* Transform extended ASCII to UTF16 and append text to the end of the byte array ***************
void append_data_str(byte id, String text)
{
  String str = utf8ascii(text);
  int text_len = str.length();
  byte_append(id);
  byte_append(byte(text_len));
  char char_array[text_len + 1];
  str.toCharArray(char_array, text_len + 1);
  for (int i = 0; (i < text_len) & (data_lenght < 254); i++)
  {
    data[data_lenght] = 0x00;
    data[data_lenght + 1] = byte(char_array[i]);
    data_lenght += 2;
  }
}
// ************************** String TITLE generation  ****************************
void generate_aux_message(String title)
{
  byte_append(0x03);
  //byte_append(0x07); telephone
  //append_data_str(0x02,"Aux");
  //append_data_str(0x01," ");
  //append_data_str(0x02, title); telephone
  append_data_str(0x10, title);
  //append_data_str(0x11,artist);
  //append_data_str(0x12,album);
  //byte_append(0x01);  //End of message.
  //data[6] = 0x010;
  //data[5] = 0x03;
  data[4] = byte(data_lenght - 5);
  data[3] = 0x00;
  data[2] = 0x40;  //0xC0;
  data[1] = byte(data_lenght - 2);
  data[0] = 0x10;
}
void message_to_DIS (String title)
{
  if ((Prev_Message != title) || Prev_Message == "") {
    data_lenght = 5;
    generate_aux_message(title);
    Prev_Message = title;
  }
  int num = 0;
  int pos = 0;
  byte line[8];
  for (int i = 0; i < data_lenght; i++)
  { if (pos == 8)
    {
      num += 1;
      if (num == 16) num = 0;
      if (key_acc == 1) {
        SendCANmessage(MS_CLEAR_DISPLAY, 8, 0x41, 0x00, 0x60, 0x11, 0x02, 0x00, 0x20, 0x08);
      }
      SendCANmessage(MS_MEDIA_ID, 8, line[0], line[1], line[2], line[3], line[4], line[5], line[6], line[7]);
      pos = 0;
      line[pos] = byte(num + 32);
      pos += 1;
    }
    line[pos] = data[i];
    pos += 1;
  }
  for (int i = pos; pos < 8; i++)
  {
    line[pos] = 0;
    pos += 1;
  }
  SendCANmessage(MS_MEDIA_ID, 8, line[0], line[1], line[2], line[3], line[4], line[5], line[6], line[7]);
}
// ************************** String ALBUM generation  ****************************
void generate_album_message(String album)
{
  byte_append(0x03);
  append_data_str(0x12, album);
  data[4] = byte(data_lenght - 5);
  data[3] = 0x00;
  data[2] = 0x40;  //0xC0;
  data[1] = byte(data_lenght - 2);
  data[0] = 0x10;
}
void message_to_DIS_album (String album)
{
  if ((Prev_Message != album) || Prev_Message == "") {
    data_lenght = 5;
    generate_album_message(album);
    Prev_Message = album;
  }
  int num = 0;
  int pos = 0;
  byte line[8];
  for (int i = 0; i < data_lenght; i++)
  { if (pos == 8)
    {
      num += 1;
      if (num == 16) num = 0;
      if (key_acc == 1) {
        SendCANmessage(MS_CLEAR_DISPLAY, 8, 0x41, 0x00, 0x60, 0x11, 0x02, 0x00, 0x20, 0x08);
      }
      SendCANmessage(MS_MEDIA_ID, 8, line[0], line[1], line[2], line[3], line[4], line[5], line[6], line[7]);
      pos = 0;
      line[pos] = byte(num + 32);
      pos += 1;
    }
    line[pos] = data[i];
    pos += 1;
  }
  for (int i = pos; pos < 8; i++)
  {
    line[pos] = 0;
    pos += 1;
  }
  SendCANmessage(MS_MEDIA_ID, 8, line[0], line[1], line[2], line[3], line[4], line[5], line[6], line[7]);
}
// ************************** String ARTIST generation  ****************************
void generate_artist_message(String artist)
{
  byte_append(0x03);
  append_data_str(0x11, artist);
  data[4] = byte(data_lenght - 5);
  data[3] = 0x00;
  data[2] = 0x40;  //0xC0;
  data[1] = byte(data_lenght - 2);
  data[0] = 0x10;
}
void message_to_DIS_artist (String artist)
{
  if ((Prev_Message != artist) || Prev_Message == "") {
    data_lenght = 5;
    generate_artist_message(artist);
    Prev_Message = artist;
  }
  int num = 0;
  int pos = 0;
  byte line[8];
  for (int i = 0; i < data_lenght; i++)
  { if (pos == 8)
    {
      num += 1;
      if (num == 16) num = 0;
      if (key_acc == 1) {
        SendCANmessage(MS_CLEAR_DISPLAY, 8, 0x41, 0x00, 0x60, 0x11, 0x02, 0x00, 0x20, 0x08);
      }
      SendCANmessage(MS_MEDIA_ID, 8, line[0], line[1], line[2], line[3], line[4], line[5], line[6], line[7]);
      pos = 0;
      line[pos] = byte(num + 32);
      pos += 1;
    }
    line[pos] = data[i];
    pos += 1;
  }
  for (int i = pos; pos < 8; i++)
  {
    line[pos] = 0;
    pos += 1;
  }
  SendCANmessage(MS_MEDIA_ID, 8, line[0], line[1], line[2], line[3], line[4], line[5], line[6], line[7]);
}

//*****************************Text formatting*****************************************//
//**************By default, bold text is displayed with left alignment.****************//
String Bold(String Text) {
  char char_Bold[] = {0x1b, 0x5b, 0x66, 0x53, 0x5f, 0x67, 0x6d, 0x00};
  return (String(char_Bold) + Text);
}
String Normal(String Text) {
  char char_Normal[] = {0x1b, 0x5b, 0x66, 0x53, 0x5f, 0x64, 0x6d, 0x00};
  return (String(char_Normal) + Text);
}
String Right(String Text) {
  char char_Right[] = {0x1b, 0x5b, 0x72, 0x6d, 0x00};
  return (String(char_Right) + Text);
}
String Central(String Text) {
  char char_Central[] = {0x1b, 0x5b, 0x63, 0x6d, 0x00};
  return (String(char_Central) + Text);
}

String data_to_str(int data, int digits) {
  String strf = String(data);
  int y = strf.length() - 1;
  if (digits == 1) {
    char x = strf.charAt(y);
    strf.setCharAt(y, ',');
    strf += (x);
  }
  else strf = strf.substring(0, y);
  return strf;
}
String data_to_time(int data3) {
  String strtime = String(data3);
  int m;
  m = atoi(strtime.c_str());
  if (m >= 10) {
    strtime = strtime;
  }
  else strtime = "0" + strtime;
  return strtime;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                                  PROCESSING CAN-MESSAGES                            //
//                All processing and HEXoding of CAN messages is here.                 //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
void CAN_message_process(CanMsg *can_msg) {
  switch (can_msg->ID)  {
    case MS_WHEEL_BUTTONS_ID: {
        switch (can_msg->Data[1]) {
          case MS_BTN_VOL: {
              if (can_msg->Data[0] == WHEEL_PRESSED) {
                if (can_msg->Data[2] == WHEEL_TURN_UP) {
                  digitalWrite(PB12, LOW); btn = millis();
                  Message_USART = "VOLUME UP";
                }
                if (can_msg->Data[2] == WHEEL_TURN_DOWN) {
                  digitalWrite(PB13, LOW); btn = millis();
                  Message_USART = "VOLUME DOWN";
                }
              }
              break;
            }
          case MS_BTN_NEXT: {
              if ((can_msg->Data[0] == BTN_PRESSED) && (can_msg->Data[2] == BTN_RELEASED)) {
                digitalWrite(PB14, LOW); btn = millis();
                Message_USART = "NEXT TRACK";
              }
              break;
            }
          case MS_BTN_PREV: {
              if ((can_msg->Data[0] == BTN_PRESSED) && (can_msg->Data[2] == BTN_RELEASED)) {
                digitalWrite(PB15, LOW); btn = millis();
                Message_USART = "PREVIOUS TRACK";
              }
              break;
            }
          case MS_BTN_STATION: {
              if (can_msg->Data[0] == BTN_PRESSED) {
                if (( window != 2) && ((can_msg->Data[2]) == BTN_RELEASED)) {
                  //
                  btn = millis();
                }
                if (( window == 2) && ((can_msg->Data[2]) == BTN_RELEASED)) {
                  btn_function(MS_BTN_BC, 0x00);
                  btn = millis();
                }
              }
              break;
            }
          case MS_BTN_MODE: {
              if ((can_msg->Data[0] == BTN_PRESSED) && (can_msg->Data[2] == BTN_RELEASED)) {
                //
                btn = millis();
              }
              break;
            }
          case MS_BTN_LEFT_WHEEL: {
              if (can_msg->Data[0] == BTN_PRESSED) {
                if ((can_msg->Data[2]) == 0x05) {
                  btn_function(MS_BTN_SETTINGS, 0x00);
                  btn = millis();
                }
                if (data2 != can_msg->Data[2])  {
                  data2 = can_msg->Data[2];
                  if (data2 == 0x3C) {
                    btn_function(MS_BTN_SETTINGS, 0x00);
                    btn_function(MS_BTN_SETTINGS, data2);
                    btn = millis();
                    if (test_mode == 0)  {
                      test_mode = 1;
                      Message_USART = "TEST MODE ON";
                    }
                    else if (test_mode == 1 )  {
                      test_mode = 0;
                      Message_USART = "TEST MODE OFF";
                    }
                  }
                }
              }
              break;
            }
        }
        break;
      }
    case MS_ECC_ID: {
        if (can_msg->Data[0] == MS_BATTERY) {
          VOLTAGE = (can_msg->Data[2]);
          if (VOLTAGE != p_VOLTAGE) {
            Serial2.println("<voltage:" + data_to_str(VOLTAGE, 1) + ">");
            p_VOLTAGE = VOLTAGE;
          }
        }
        if (can_msg->Data[0] == MS_ENGINE_TEMP) {
          T_ENG = (((can_msg->Data[3]) * 256) + (can_msg->Data[4]));
          if (T_ENG != p_T_ENG) {
            Serial2.println("<t_eng:" + data_to_str(T_ENG, 0) + ">");
            p_T_ENG = T_ENG;
          }
        }
        break;
      }
    case MS_SPEED_RPM_ID: {
        if (can_msg->Data[0] == 0x46) {
          SPEED = ((((can_msg->Data[4]) << 8) + (can_msg->Data[5])) >> 7);
          RPM = ((((can_msg->Data[2]) << 8) + (can_msg->Data[3])) >> 2);
          if ((can_msg->Data[6]) != 0x04) { //задний ход выкл
            REVERSE = 0;
          }
          if ((can_msg->Data[6]) == 0x04) { //задний ход вкл
            REVERSE = 1;
          }
        }
        break;
      }
    case MS_TIME_CLOCK_ID: {
        if (can_msg->Data[0] == 0x46) {
          DAY = ((can_msg->Data[4]) >> 3);
          MONTH = (can_msg->Data[3]);
          YEAR = (can_msg->Data[2]);
        }
        break;
      }
    case MS_RANGE_ID: {
        if (can_msg->Data[0] == 0x46) {
          RANGE = ((((can_msg->Data[2]) << 8) + (can_msg->Data[3])) / 2);
          if (RANGE != p_RANGE) {
            Serial2.println("<range:" + String(RANGE) + ">");
            p_RANGE = RANGE;
          }
        }
        break;
      }
    case MS_CLIMATE_INFO_ID: {
        if (can_msg->Data[0] == 0x21 && can_msg->Data[1] == 0x00 && can_msg->Data[6] == 0xB0 && can_msg->Data[7] == 0x24) {
          CTemp1 = can_msg->Data[2];
          CTemp2 = can_msg->Data[4];
        }
        if (can_msg->Data[0] == 0x22 && can_msg->Data[1] == 0x01 && can_msg->Data[2] == 0xE0) {
          CNapr = can_msg->Data[3] - 0x21;
        }
        if (can_msg->Data[0] == 0x22 && can_msg->Data[4] == 0x25) {
          if (can_msg->Data[5] == 0x01) {
            CEco = 0x30;
          }
          if (can_msg->Data[5] == 0x03) {
            CEco = 0x31;
          }
        }
        if (CEco == 0x30) {
          if (can_msg->Data[0] == 0x23 && can_msg->Data[1] == 0x26) {
            CSpeed = can_msg->Data [6];
            if (CSpeed != p_CSpeed) {
              Serial2.println("<CSpeed:" + String(CSpeed) + ">");
              p_CSpeed = CSpeed;
            }
          }
        }
        if (CEco == 0x31) {
          if (can_msg->Data[0] == 0x24 && can_msg->Data[1] == 0x50) {
            CSpeed = can_msg->Data[3];
            if (CSpeed != p_CSpeed) {
              Serial2.println("<CSpeed:" + String(CSpeed) + ">");
              p_CSpeed = CSpeed;
            }
          }
        }
        if (can_msg->Data[0] == 0x21 && can_msg->Data[1] == 0x00 && can_msg->Data[6] == 0xB0 && can_msg->Data[7] == 0xA3) {
          CTemp1 = can_msg->Data[2];
          CTemp2 = can_msg->Data[4];
          if ((CTemp1 != p_CTemp1) || (CTemp2 != p_CTemp2)) {
            Serial2.println("<CTemp:" + String(CTemp1) + String(CTemp2) + ">");
            p_CTemp1 = CTemp1;
            p_CTemp2 = CTemp2;
          }
        }
        if (can_msg->Data[0] == 0x21 && can_msg->Data[1] == 0xE0 && can_msg->Data[3] == 0xA4) {
          CNapr = can_msg->Data[2] - 0x21;
          if (CNapr != p_CNapr) {
            Serial2.println("<CNapr:" + String(CNapr) + ">");
            p_CNapr = CNapr;
          }
        }
        if (can_msg->Data[0] == 0x10 && can_msg->Data[6] == 0x25) {
          if (can_msg->Data[7] == 0x01) {
            CEco = 0x30;
          }
          if (can_msg->Data[7] == 0x03) {
            CEco = 0x31;
          }
          if (CEco != p_CEco) {
            Serial2.println("<CEco:" + String(CEco) + ">");
            p_CEco = CEco;
          }
        }
        if (can_msg->Data[0] == 0x22 && can_msg->Data[1] == 0x50) {
          CSpeed = can_msg->Data[3];
          if (CSpeed != p_CSpeed) {
            Serial2.println("<CSpeed:" + String(CSpeed) + ">");
            p_CSpeed = CSpeed;
          }
        }
        if ( can_msg->Data[0] == 0x25 && can_msg->Data[1] == 0xA5 && can_msg->Data[2] == 0x02  && can_msg->Data[4] == 0x50 && can_msg->Data[5] == 0x00 && can_msg->Data[6] == 0x41 && can_msg->Data[7] == 0x59) {
          if (can_msg->Data[3] == 0xE0) {
            CNapr = 0x38;
            CSpeed = 0x41;
            if (CNapr != p_CNapr) {
              Serial2.println("<CNapr:" + String(CNapr) + ">");
              p_CNapr = CNapr;
            }
            if (CSpeed != p_CSpeed) {
              Serial2.println("<CSpeed:" + String(CSpeed) + ">");
              p_CSpeed = CSpeed;
            }
          }
        }
        break;
      }
    case MS_TEMP_OUT_DOOR_ID: {
        if ((can_msg->Data[0] == 0x46) && (COutT != p_COutT)) {
          COutT = (can_msg->Data[2] / 2) - 40;
          intCOutT = (can_msg->Data[2] / 2) - 40;
          if (intCOutT < 0) {
            Serial2.println("<COutT:" + '-' + String(COutT) + ">");
          }
          else
          {
            Serial2.println("<COutT:" + String(COutT) + ">");
          }
          p_COutT = COutT;
        }
        break;
      }
    //***************************************************************************
    case MS_IGNITION_STATE_ID: {
        if ((can_msg->Data[2] ==  MS_IGNITION_NO_KEY) || (can_msg->Data[2] ==  MS_IGNITION_1)) {
          key_acc = 0;
        }
        if ((can_msg->Data[2] ==  MS_IGNITION_KEY_PRESENT) || (can_msg->Data[2] == MS_IGNITION_START)) {
          key_acc = 1;
        }
        break;
      }
    case MS_WINDOW_ID: {
        if (can_msg->Data[1] == 0x0C && can_msg->Data[2] == 0x00 && can_msg->Data[3] == 0xC8) { //main
          window =  1;
        }
        if (can_msg->Data[1] == 0x04 && can_msg->Data[2] == 0x08 && can_msg->Data[3] == 0xC8) { //bc
          window =  2;
        }
        if (((can_msg->Data[1] == 0x0C) || (can_msg->Data[1] == 0x0D)) && can_msg->Data[2] == 0x08 && can_msg->Data[3] == 0xC8) { //settings
          window =  3;
        }
        if (can_msg->Data[1] == 0x0A && can_msg->Data[2] == 0x00 && can_msg->Data[3] == 0xC8) { //climate
          window =  4;
        }
        break;
      }
  }
#ifdef DEBUG
  char scan[40];
  sprintf (scan, "\n % d: % 04X # %02X %02X %02X %02X %02X %02X %02X %02X ", millis(),
           can_msg->ID, can_msg->Data[0], can_msg->Data[1], can_msg->Data[2], can_msg->Data[3],
           can_msg->Data[4], can_msg->Data[5], can_msg->Data[6], can_msg->Data[7]);
  Serial2.print(scan);
#endif
}


//********************Filling an array with USART characters**************************
String Data_USART() {
  String Buffer_USART;
  char u;
  while (Serial2.available() > 0 && u != '\n') { //read serial buffer until \n
    char u = Serial2.read();
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
//void CANSetup(void);
//void SendCANmessage(long, byte, byte, byte, byte, byte, byte, byte, byte, byte);
//void btn_function(byte, byte);
//*************************************************************************************//

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                                  SETUP FUNCTIONS                                    //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
void setup() {
  Serial2.begin(115200); // USART2 on A2-A3 pins
  CANSetup() ;
  pinMode(PB12, OUTPUT_OPEN_DRAIN);
  pinMode(PB13, OUTPUT_OPEN_DRAIN);
  pinMode(PB14, OUTPUT_OPEN_DRAIN);
  pinMode(PB15, OUTPUT_OPEN_DRAIN);
  pinMode(PC13, OUTPUT); // LED
  pinMode(PC14, OUTPUT); // LED ERR
  digitalWrite(PC13, PC13ON);
  digitalWrite(PC13, PC13OFF);
  digitalWrite(PB12, HIGH);
  digitalWrite(PB13, HIGH);
  digitalWrite(PB14, HIGH);
  digitalWrite(PB15, HIGH);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                                   LOOP FUNCTIONS                                    //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
void loop() {
  if ((millis() - btn) > 150) {
    digitalWrite(PB12, HIGH);
    digitalWrite(PB13, HIGH);
    digitalWrite(PB14, HIGH);
    digitalWrite(PB15, HIGH);
    digitalWrite(PC13, HIGH);
  }
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
  //*************************Receiving a message with USART2****************************
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
  while (canBus.available() > 0)
  { CAN_message_process(canBus.recv());
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

