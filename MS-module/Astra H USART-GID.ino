#include <HardwareCAN.h>
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
#define MS_BTN_LEFT 0xD6
#define MS_BTN_CD_RADIO 0xE0
#define MS_BTN_SETTINGS 0xFF
#define MS_WHEEL_BUTTONS_ID 0x206
#define MS_BTN_STATION 0x81
#define MS_BTN_MODE 0x82
#define MS_BTN_LEFT_WHEEL_TURN 0x83
#define MS_BTN_LEFT_WHEEL 0x84
#define MS_BTN_NEXT 0x91
#define MS_BTN_PREV 0x92
#define MS_MEDIA_ID 0x6C1
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
#define MS_BACKWARDS_BIT 0x04
#define MS_WAKEUP_ID 0x697
// Define buttons state bytes
#define BTN_PRESSED 0x01
#define BTN_RELEASED 0x00
#define WHEEL_PRESSED 0x08
#define WHEEL_TURN_DOWN 0xFF
#define WHEEL_TURN_ 0x01
// Limit time to flag a CAN error
#define CAN_TIMEOUT 100
#define CAN_DELAY 0 // 10        // ms between two processings of incoming messages
#define CAN_SEND_RATE 10 // 200   // ms between two successive sendings
#define PC13ON 0
#define PC13OFF 1
#define DELAY 250

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                         ASTRA H VARIABLES AND FUNCTIONS                             //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

//*****************************Text formatting*****************************************//
//**************By default, bold text is displayed with left alignment.****************//
String Bold(String Text) {
  char char_Bold[] = {0x1b, 0x5b, 0x66, 0x53, 0x5f, 0x67, 0x6d, 0x00};
  return (String(char_Bold) + Text);
}
String Normal(String Text) {
  char char_Bold[] = {0x1b, 0x5b, 0x66, 0x53, 0x5f, 0x64, 0x6d, 0x00};
  return (String(char_Bold) + Text);
}
String Right(String Text) {
  char char_Bold[] = {0x1b, 0x5b, 0x72, 0x6d, 0x00};
  return (String(char_Bold) + Text);
}
String Central(String Text) {
  char char_Bold[] = {0x1b, 0x5b, 0x63, 0x6d, 0x00};
  return (String(char_Bold) + Text);
}
//**************Variables
bool pause = 0;
int d_mode = 0;
int VOLTAGE = 131;
int T_ENG = 1314;
int SPEED = 0;
int RPM = 0;
uint32_t time_request_ecc = 0;          //Variable for the parameter request timer from ECC
uint32_t time_send = 0;                 //Variable for the burst transfer timer in CAN
uint32_t Time_USART = 0;                //Variable for the USART buffer fill timer
uint32_t Time_Update_Message = 0;
String Prev_Message;
String Message_USART;
String message;


String Data_USART() {
  String Buffer_USART;
  char u;
  while (Serial2.available() > 0 && u != '\n') { //read serial buffer until \n
    char u = Serial2.read();
    if (u != 0xD) Buffer_USART += u;  // skip \r
  }
  Serial2.print(Buffer_USART);
  return Buffer_USART;
}

String data_to_str(int data) {
  String strf = String(data);
  int y = strf.length() - 1;
  char x = strf.charAt(y);
  strf.setCharAt(y, ',');
  strf += x;
  return strf;
}

//String text_construct(){}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                             CAN-BUS VARIABLES AND FUNCTIONS                         //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//********************************************************************************************//
//FOR CORRECT WORK, IT IS REQUIRED TO INCLUDE CAN_MCR_TXFP In the HardwareCAN.cpp FILE line 26//
//********************************************************************************************//
// Instanciation of CAN interface
HardwareCAN canBus(CAN1_BASE);
CanMsg msg ;
CanMsg *r_msg;
bool bus_err = 0;
void CANSetup(void)
{
  CAN_STATUS Stat ;
  // Initialize CAN module
  canBus.map(CAN_GPIO_PB8_PB9);
  Stat = canBus.begin(CAN_SPEED_95, CAN_MODE_NORMAL);
  canBus.filter(0, 0x201, 0xFFFFFFFF);
  canBus.filter(1, 0x206, 0xFFFFFFFF);
  canBus.filter(2, 0x6C1, 0xFFFFFFFF);
  canBus.filter(3, 0x548, 0xFFFFFFFF);
  canBus.set_irq_mode();              // Use irq mode (recommended)
  Stat = canBus.status();
  if (Stat != CAN_OK)
  { // Initialization failed
    Serial2.print("Initialization failed");
  }
}

void SendCANmessage(long id = 0x100, byte dlength = 8, byte d0 = 0x00, byte d1 = 0x00, byte d2 = 0x00, byte d3 = 0x00, byte d4 = 0x00, byte d5 = 0x00, byte d6 = 0x00, byte d7 = 0x00)
{
  msg.IDE = CAN_ID_STD;
  msg.RTR = CAN_RTR_DATA;
  msg.ID = id ;
  msg.DLC = dlength;                   // Number of data bytes to follow
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
    Serial2.print("CAN-Bus send error");
  }
  // Send this frame
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                       TRANSFORM TEXT AND FUNCTIONS SEND TO DIS                      //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
byte data[255];
int l = 5;
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
void byte_append(byte* array1, int* len1, byte add_byte)
{
  if (*len1 < 254)
  {
    array1[*len1] = add_byte;
    *len1 += 1;
  }
}
// ******************* Append Unicode text to the end of the byte array ***********************
void append_data_str(String text, byte* data_byte, int* length_byte)
{
  int text_len = text.length();
  char char_array[text_len + 1];
  text.toCharArray(char_array, text_len + 1);
  for (int i = 0; (i < text_len) & (*length_byte < 254); i++)
  {
    data_byte[*length_byte] = 0x00;
    data_byte[*length_byte + 1] = byte(char_array[i]);
    *length_byte += 2;
  }
}
// ************************** String generation  ****************************
void append_generate_string(byte id, String text, byte* byte_array, int* array_length, int number = 0)
{
  String str = utf8ascii(text);
  int str_len = str.length();
  int p = *array_length;
  byte_append(byte_array, &p, id);
  if (number > 0)
  {
    byte_append(byte_array, &p, byte(str_len + 2));
    byte_append(byte_array, &p, 0x27);
    byte_append(byte_array, &p, byte(127 + number));
    byte_append(byte_array, &p, 0x00);
    byte_append(byte_array, &p, 0x20);
  }
  else
  {
    byte_append(byte_array, &p, byte(str_len));
  }
  append_data_str(str, data, &p);
  *array_length = p;
}
void generate_aux_message(String title, byte* byte_array, int* array_length, int mode)
{
  int p = *array_length;
  byte_append(byte_array, &p, 0x03);
  append_generate_string(0x10, title, byte_array, &p);
  byte_array[4] = byte(p - 5);
  byte_array[3] = 0x00;
  byte_array[2] = 0xC0;
  byte_array[1] = byte(p - 2);
  byte_array[0] = 0x10;
  *array_length = p;
}
void message_to_DIS(String title, int mode = 0, byte* byte_array = data)
{
  if ((title != Prev_Message) || Prev_Message == "") {
    l = 5;
    generate_aux_message(title, byte_array, &l, mode);
    Prev_Message = title;
  }
  int num = 0;
  int pos = 0;
  byte line[8];
  for (int i = 0; i < l; i++)
  { if (pos == 8)
    {
      num += 1;
      if (num == 16)
      {
        num = 0;
      }
      SendCANmessage(MS_MEDIA_ID, 8, line[0], line[1], line[2], line[3], line[4], line[5], line[6], line[7]);
      pos = 0;
      line[pos] = byte(num + 32);
      pos += 1;
    }
    line[pos] = byte_array[i];
    pos += 1;
  }
  for (int i = pos; pos < 8; i++)
  {
    line[pos] = 0;
    pos += 1;
  }
  SendCANmessage(MS_MEDIA_ID, 8, line[0], line[1], line[2], line[3], line[4], line[5], line[6], line[7]);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                                  SETUP FUNCTIONS                                    //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
void setup() {
  CANSetup() ;
  Serial2.begin(115200); // USART2 on A2-A3 pins
  Serial2.println("Starting Astra-H CAN-Shild");
  pinMode(PC13, OUTPUT); // LED
  pinMode(PC14, OUTPUT); // LED ERR
  digitalWrite(PC13, PC13ON);
  digitalWrite(PC13, PC13OFF);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                                   LOOP FUNCTIONS                                    //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
void loop() {
  bool BTN_STATION = 0;
  bool BTN_MODE = 0;
  bool BTN_NEXT = 0;
  bool BTN_PREV = 0;
  if ((millis() - time_request_ecc) > 1000) {
    SendCANmessage(0x0248, 8, 0x06, 0xAA, 0x01, 0x01, 0x07, 0x10, 0x11, 0x00);
    time_request_ecc = millis();
  }
  if (canBus.available() > 0)
  { r_msg = canBus.recv();
    Serial2.print(millis());
    Serial2.print("; ");
    Serial2.print(r_msg->ID, HEX);
    Serial2.print(" # ");
    Serial2.print(r_msg->Data[0], HEX);       Serial2.print(" ");
    Serial2.print(r_msg->Data[1], HEX);        Serial2.print(" ");
    Serial2.print(r_msg->Data[2], HEX);       Serial2.print(" ");
    Serial2.print(r_msg->Data[3], HEX);       Serial2.print(" ");
    Serial2.print(r_msg->Data[4], HEX);       Serial2.print(" ");
    Serial2.print(r_msg->Data[5], HEX);       Serial2.print(" ");
    Serial2.print(r_msg->Data[6], HEX);       Serial2.print(" ");
    Serial2.println(r_msg->Data[7], HEX);
    switch (r_msg->ID)
    {
      case MS_WHEEL_BUTTONS_ID: {
          switch (r_msg->Data[1]) {
            case MS_BTN_STATION: {
                if ((r_msg->Data[0] == BTN_PRESSED) && ((r_msg->Data[2]) == 0x00)) BTN_STATION = 1;
                break;
              }
            case MS_BTN_MODE: {
                if ((r_msg->Data[0] == BTN_PRESSED) && ((r_msg->Data[2]) == 0x00)) BTN_MODE = 1; //Set flag button on
                break;
              }
            case MS_BTN_NEXT: {
                if ((r_msg->Data[0] == BTN_PRESSED) && ((r_msg->Data[2]) == 0x00)) BTN_NEXT = 1; //Set flag button on
                break;
              }
            case MS_BTN_PREV: {
                if ((r_msg->Data[0] == BTN_PRESSED) && ((r_msg->Data[2]) == 0x00)) BTN_PREV = 1; //Set flag button on
                break;
              }
          }
          break;
        }
      case MS_MEDIA_BUTTONS_ID: {
          if ((r_msg->Data[0] == BTN_PRESSED) && ((r_msg->Data[1]) > 0x30) && ((r_msg->Data[1]) < 0x35) && ((r_msg->Data[2]) == 0x00))
            d_mode = int((r_msg->Data[1]) - 0x30);
          if ((r_msg->Data[1]) == MS_BTN_OK)
            d_mode = 0;
          Serial2.print("MEDIA BUTTON PRESS: " + d_mode);
          break;
        }
      case MS_MEDIA_ID: {                                                   //If EHU in AUX-Mode
          if (((r_msg->Data[0]) == 0x10 && (r_msg->Data[1]) == 0x2E
               && (r_msg->Data[2]) == 0xC0 && (r_msg->Data[3]) == 0x00
               && (r_msg->Data[4]) == 0x2B && (r_msg->Data[5]) == 0x03
               && (r_msg->Data[6]) == 0x01 && (r_msg->Data[7]) == 0x01) ||
              ((r_msg->Data[0]) == 0x10 && (r_msg->Data[1]) == 0x22
               && (r_msg->Data[2]) == 0xC0 && (r_msg->Data[3]) == 0x00
               && (r_msg->Data[4]) == 0x1F && (r_msg->Data[5]) == 0x03
               && (r_msg->Data[6]) == 0x10 && (r_msg->Data[7]) == 0x0E) ||
              ((r_msg->Data[0]) == 0x10 && (r_msg->Data[1]) == 0x22
               && (r_msg->Data[2]) == 0x40 && (r_msg->Data[3]) == 0x00
               && (r_msg->Data[4]) == 0x2F && (r_msg->Data[5]) == 0x03
               && (r_msg->Data[6]) == 0x01 && (r_msg->Data[7]) == 0x03)) {
            delay(1);
            digitalWrite(PC13, PC13ON); // LED shows that recieved data is being printed out
            SendCANmessage(MS_MEDIA_ID, 8, 0x10, 0x05, 0xC0, 0x00, 0x03, 0x03, 0x10, 0x00); //Corrupt message
            Serial2.println("Corrupt message");
            digitalWrite(PC13, PC13OFF);
          }
          break;
        }
      case MS_ECC_ID: {
          if (r_msg->Data[0] == MS_BATTERY) {
            VOLTAGE = (r_msg->Data[0]);
          }
          if (r_msg->Data[0] == MS_ENGINE_TEMP) {
            T_ENG = (((r_msg->Data[3]) * 256) + (r_msg->Data[4]));
          }
          if (r_msg->Data[0] == MS_RPM_SPEED) {
            RPM = ((r_msg->Data[1]) * 256) + (r_msg->Data[2]);
            SPEED = r_msg->Data[4];
          }
          break;
        }
    }
  }
  canBus.free();


  if (millis() - Time_USART > 200) {   //delay needed to fillup buffers
    Message_USART = Data_USART();
    Time_USART = millis();
  }
/*
  if ((millis() - Time_Update_Message) > 2000) {
    message = (Normal(data_to_str(T_ENG) + "C" + "/" + data_to_str(VOLTAGE) + "V"));
    message += Central(Bold("USB 12/256"))
               Time_Update_Message = millis();
  }

  if (message == "") {
    message = (Normal(data_to_str(T_ENG) + "C" + "/" + data_to_str(VOLTAGE) + "V"));
  }

*/

  if (Message_USART != "") {
    message = Central(Bold(Message_USART));
//    Time_Update_Message = millis();
  }

  if (millis() - time_send > 500) { //Update display
    message_to_DIS(message);
//    Serial2.println(message);
    time_send = millis();
  }
}
