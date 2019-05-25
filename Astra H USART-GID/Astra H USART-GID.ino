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
//#define DEBUG  //Debug activation will increase the ant of memory by ~16k

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
//**************Variables****************//
bool alarm = 0;
bool Blink = 0;
bool AUX_mode = 0;
int d_mode = 0;
int VOLTAGE = 131;
int T_ENG = 1000;
int SPEED = 0;
int RPM = 0;
uint32_t time_request_ecc = 0;          //Variable for the parameter request timer from ECC
uint32_t time_send = 0;                 //Variable for the burst transfer timer in CAN
uint32_t Time_USART = 0;                //Variable for the USART buffer fill timer
uint32_t Time_Update_Message = 0;       //Variable for return to the main message after receiving a USART message
uint32_t Pause_Update_DIS = 0;          //Variable to pause the DIS update for the duration of the data transfer EHU 
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
#ifdef DEBUG
  Serial2.print(Buffer_USART);
#endif
  return Buffer_USART;
}

String data_to_str(int data, int digits) {  //Convert temperature and voltage data to the corresponding type string
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

String Alarm(bool Sinchro) {
  if (Sinchro) return (">>TEMP ENGINE-" + data_to_str(T_ENG, 0) + "°C<<");
  else       return ("--TEMP ENGINE-" + data_to_str(T_ENG, 0) + "°C--");
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                             CAN-BUS VARIABLES AND FUNCTIONS                         //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//********************************************************************************************//
//FOR CORRECT WORK, IT IS REQUIRED TO INCLUDE CAN_MCR_TXFP In the HardwareCAN.cpp FILE line 26//
//********************************************************************************************//
// Instanciation of CAN interface
#define CAN_RX_QUEUE_SIZE 36  //The size of the incoming message buffer. If there is a loss of incoming packets, you need to increase the value.
HardwareCAN canBus(CAN1_BASE);
CanMsg msg ;
CanMsg *r_msg;
void CANSetup(void)
{
  CAN_STATUS Stat ;
  // Initialize CAN module
  canBus.map(CAN_GPIO_PB8_PB9);
  Stat = canBus.begin(CAN_SPEED_95, CAN_MODE_NORMAL);
  canBus.filter(0, CAN_FIFO0, 0x201, 0xFFFFFFFF);
  canBus.filter(1, CAN_FIFO0, 0x206, 0xFFFFFFFF);
  canBus.filter(2, CAN_FIFO0, 0x548, 0xFFFFFFFF);
  canBus.filter(3, CAN_FIFO1, 0x6C1, 0xFFFFFFFF);
  canBus.set_irq_mode();              // Use irq mode (recommended)
  nvic_irq_set_priority(NVIC_USB_LP_CAN_RX0,0);
  nvic_irq_set_priority(NVIC_USART2,1);
  Stat = canBus.status();
#ifdef DEBUG
  if (Stat != CAN_OK) Serial2.print("Initialization failed");
#endif
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
 #ifdef DEBUG
    Serial2.print("CAN-Bus send error");
#endif
  }
  // Send this frame
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                       TRANSFORM TEXT AND FUNCTIONS SEND TO DIS                      //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
byte data[255];  //Data Buffer for Package Preparation
int data_lenght = 5;  //Useful length of data array for package preparation
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
// ************************** String generation  ****************************
void generate_aux_message(String title)
{
  byte_append(0x03);
  //append_data_str(0x02,"Aux");
  //append_data_str(0x01," ");
  append_data_str(0x10, title);
  //append_data_str(0x11,artist);
  //append_data_str(0x12,album);
  //byte_append(0x01);  //End of message.
  data[4] = byte(data_lenght - 5);
  data[3] = 0x00;
  data[2] = 0xC0;
  data[1] = byte(data_lenght - 2);
  data[0] = 0x10;

}
// ************************** Generation package and send  ****************************
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
  if ((millis() - time_request_ecc) > 1000) {
    SendCANmessage(0x0248, 8, 0x06, 0xAA, 0x01, 0x01, 0x07, 0x10, 0x11, 0x00);
    time_request_ecc = millis();
  }
  if (canBus.available() > 0) //There are unprocessed messages in the queue
  { r_msg = canBus.recv();    //Get a message from the queue
    switch (r_msg->ID)
    {
      case MS_WHEEL_BUTTONS_ID: {
          switch (r_msg->Data[1]) {
            case MS_BTN_STATION: {
                if ((r_msg->Data[0] == BTN_PRESSED) && ((r_msg->Data[2]) == 0x00)) 
 #ifdef DEBUG               
                Serial2.print("\nBTN_STATION Pressed");
 #endif               
                break;
              }
            case MS_BTN_MODE: {
                if ((r_msg->Data[0] == BTN_PRESSED) && ((r_msg->Data[2]) == 0x00)) 
 #ifdef DEBUG                
                Serial2.print("\nBTN_MODE Pressed");
 #endif                
                break;
              }
            case MS_BTN_NEXT: {
                if ((r_msg->Data[0] == BTN_PRESSED) && ((r_msg->Data[2]) == 0x00)) {
 #ifdef DEBUG               
                Serial2.print("\nBTN_NEXT Pressed");
 #endif      
                }
                break;
              }
            case MS_BTN_PREV: {
                if ((r_msg->Data[0] == BTN_PRESSED) && ((r_msg->Data[2]) == 0x00)) {
 #ifdef DEBUG               
                Serial2.print("\nBTN_PREV Pressed");
 #endif                        
                }
                break;
              }
          }
          break;
        }
      case MS_MEDIA_BUTTONS_ID: {
          if ((r_msg->Data[0] == BTN_PRESSED) &&
              ((r_msg->Data[1]) > 0x30) &&
              ((r_msg->Data[1]) < 0x35) &&
              ((r_msg->Data[2]) == 0x00))
            d_mode = int((r_msg->Data[1]) - 0x30);
          if ((r_msg->Data[1]) == MS_BTN_OK)
            d_mode = 0;
#ifdef DEBUG
          Serial2.print("MEDIA BUTTON PRESS: " + d_mode);
#endif
          break;
        }
      case MS_MEDIA_ID: {                                                   //If EHU in AUX-Mode
          Pause_Update_DIS = millis();
          if ((((r_msg->Data[0]) == 0x10) | ((r_msg->Data[0]) == 0x21)) && AUX_mode)  {
            //            if (((r_msg->Data[0]) == 0x21) && AUX_mode)  {
            delay(1);
            SendCANmessage(MS_MEDIA_ID, 8, 0x21, 0x3A, 0xC0, 0x00, 0x37, 0x03, 0x10, 0x1A); //Corrupt message
#ifdef DEBUG
            Serial2.print("\nCorrupt message");
#endif
          }
          if ((r_msg->Data[0]) == 0x24) {
            if (((r_msg->Data[3]) != 0x41) && ((r_msg->Data[5]) != 0x75) && ((r_msg->Data[7]) != 0x78))
            {
              AUX_mode = 0;
#ifdef DEBUG
              Serial2.print("\nAUX OFF");
#endif
            }
            else {
              if (AUX_mode == 0) {
                delay(200);
                AUX_mode = 1;
#ifdef DEBUG
                Serial2.print("\nAUX ON");
#endif
              }
            }
          }

          break;
        }
      case MS_ECC_ID: {
          if (r_msg->Data[0] == MS_BATTERY) {
            VOLTAGE = (r_msg->Data[2]);
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
#ifdef DEBUG
    char scan[40];
    sprintf (scan, "\n%d: %04X # %02X %02X %02X %02X %02X %02X %02X %02X ", millis(),
             r_msg->ID, r_msg->Data[0], r_msg->Data[1], r_msg->Data[2], r_msg->Data[3],
             r_msg->Data[4], r_msg->Data[5], r_msg->Data[6], r_msg->Data[7]);
    Serial2.print(scan);
#endif
  }
  canBus.free(); //The message is processed, delete from the buffer, shift the queue


  if (T_ENG > 1080) {
    alarm = 1;
    message = Alarm(Blink);
  }
  else alarm = 0;

  if (millis() - Time_USART > 200) {   //delay needed to fillup buffers
    Message_USART = Data_USART();
    Time_USART = millis();
  }

  if (((millis() - Time_Update_Message) > 3000) && !alarm) {
    message = Normal(data_to_str(T_ENG, 0)) + "°C" + "/" + data_to_str(VOLTAGE, 1) + "V";
    message += Right(Bold("USB:232/480"));
    Time_Update_Message = millis();  //To return to the main message after receiving a USART message
  }

  if ((message == "") && !alarm) message = Normal(data_to_str(T_ENG, 0)) + "°C" + "/" + data_to_str(VOLTAGE, 1) + "V";

  if ((Message_USART != "") && !alarm) {
    message = Central(Bold(Message_USART));
    Time_Update_Message = millis();
  }

  if (((millis() - time_send) > 1000) && AUX_mode && ((millis() - Pause_Update_DIS) > 50)) { //Update display
    message_to_DIS(message);
    if (Blink) Blink = 0;
    else Blink = 1;
    //    Serial2.println(message);
    time_send = millis();
  }
}
