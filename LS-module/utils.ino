// ======== GENERAL ==========
// #define DEBUG
// #define LOG

/**
   print out extra messages that are needed for debug only
*/
void debug(String str) {
#ifdef DEBUG
  UART.print(millis());
  UART.print("\t");
  UART.println(str);
#endif
}

/**
 * print out extra message + value
 */
void debug(String str, int val) {
#ifdef DEBUG
  UART.print(millis());
  UART.print("\t");
  UART.print(str);
  UART.print("\t");
  UART.println(val);
#endif
}

/**
 * print out extra message + HEX value
 */
void debugHex(String str, int val) {
#ifdef DEBUG    
  UART.print(millis());
  UART.print("\t");
  UART.print(str);
  UART.print("\t");
  UART.println(val, HEX);
#endif
}

/**
 * print some string data to UART (bluetooth)
 */
void log(String str) {
#ifdef LOG
  UART.print(millis());
  UART.print("\t");
  UART.println(str);
#endif
}

/**
 * Fill an array with USART characters (full string from HW buffer)
 */
String readUart() {
  String buffer;
  char u;
  while (UART.available() > 0 && u != '\n') { //read serial buffer until \n
    char u = UART.read();
    if ((u != 0xD)) buffer += u;  // skip \r
  }
  buffer.remove(buffer.length() - 1);
//#ifdef DEBUG
//  UART.print("received message: ");
//  UART.println(buffer);
//  UART.print("received length: ");
//  UART.println(buffer.length());
//#endif
//  flagUartReceived = true;
  return buffer;
}

// ======== CAN related ==========

void msCANSetup(void)
{
  activeBus = MS_BUS;
  CAN_STATUS Stat ;
  afio_init(); // this will restart subsystem and make pins A11A12 work
  // Initialize CAN module
  canBus.map(CAN_GPIO_PINS_MS);
  Stat = canBus.begin(CAN_SPEED_95, CAN_MODE_NORMAL);
  canBus.free();

  //  canBus.filter(0, 0, 0);
  canBus.filter(0, 0x206 << 21, 0xFFFFFFFF) ;   // filter 0 only allows standard identifier 0x206
  canBus.filter(1, 0x208 << 21, 0xFFFFFFFF) ;   // filter 1 only allows standard identifier 0x208
  canBus.set_irq_mode();              // Use irq mode (recommended)
  Stat = canBus.status();
  if (Stat != CAN_OK)
  { // Initialization failed
    digitalWrite(PC13, LOW);
    delay(10000);
    log("Initialization failed");
  }
}


void lsCANSetup(void)
{
  activeBus = LS_BUS;
  CAN_STATUS Stat ;
  afio_init(); // this will restart subsystem and make pins A11A12 work
  // Initialize CAN module
  canBus.map(CAN_GPIO_PINS_LS);
  Stat = canBus.begin(CAN_SPEED_33, CAN_MODE_NORMAL);
  canBus.free();

   debug("setting filters...");
//  canBus.filter(0, 0, 0);
   canBus.filter(0, 0x100 << 21, 0xFFFFFFFF) ; // nothing
   canBus.filter(1, 0x108 << 21, 0xFFFFFFFF) ; // speed, taho
   canBus.filter(2, 0x145 << 21, 0xFFFFFFFF) ; // engine tempr
   canBus.filter(3, 0x160 << 21, 0xFFFFFFFF) ; // open/close from distance
   canBus.filter(4, 0x175 << 21, 0xFFFFFFFF) ; // Steering wheel buttons
   canBus.filter(5, 0x230 << 21, 0xFFFFFFFF) ; // doors and locls
   canBus.filter(6, 0x350 << 21, 0xFFFFFFFF) ; // backwards drive direction
   canBus.filter(7, 0x370 << 21, 0xFFFFFFFF) ; // handbrake, fog lights, etc...
   canBus.filter(8, 0x500 << 21, 0xFFFFFFFF) ; // voltage
   canBus.filter(9, 0x170 << 21, 0xFFFFFFFF) ; // KEY
   canBus.filter(10, 0x305 << 21, 0xFFFFFFFF) ; // IPC (central buttons + lights)
   debug("filters are set.");
  canBus.set_irq_mode();              // Use irq mode (recommended)
  Stat = canBus.status();
  if (Stat != CAN_OK)
  { // Initialization failed
    digitalWrite(PC13, LOW);
    delay(10000);
    log("Initialization failed");
  }
}


void canRestart(void){
  switch (activeBus) {
    case LS_BUS:
      lsCANSetup();
      break;
    case MS_BUS:
      msCANSetup();
      break;
  }
}

/**
   Print out received message to UART out
*/
// void printMsg(CanMsg *r_msg) {
void printMsg(void) {
#ifdef DEBUG
  digitalWrite(PC13, PC13ON); // LED shows that recieved data is being printed out
  UART.print(millis());
  UART.print("\t");
  UART.print(r_msg->ID, HEX);
  UART.print(" # ");
  UART.print(r_msg->Data[0], HEX);
  UART.print(" ");
  UART.print(r_msg->Data[1], HEX);
  UART.print(" ");
  UART.print(r_msg->Data[2], HEX);
  UART.print(" ");
  UART.print(r_msg->Data[3], HEX);
  UART.print(" ");
  UART.print(r_msg->Data[4], HEX);
  UART.print(" ");
  UART.print(r_msg->Data[5], HEX);
  UART.print(" ");
  UART.print(r_msg->Data[6], HEX);
  UART.print(" ");
  UART.println(r_msg->Data[7], HEX);
  digitalWrite(PC13, PC13OFF);
#endif
}

/**
 * Send message
 * optimized by https://github.com/Gegerd
*/
void SendCANmessage(long id = 0x100, byte dlength = 8, byte d0 = 0x00, byte d1 = 0x00, byte d2 = 0x00, byte d3 = 0x00, byte d4 = 0x00, byte d5 = 0x00, byte d6 = 0x00, byte d7 = 0x00)
{ CanMsg msg ;
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
  while ((MBX == CAN_TX_NO_MBX) && ((millis() - time) < CAN_SEND_TIMEOUT)) ;
  if ((millis() - time) >= CAN_SEND_TIMEOUT) {
    canBus.cancel(CAN_TX_MBX0);
    canBus.cancel(CAN_TX_MBX1);
    canBus.cancel(CAN_TX_MBX2);
    #ifdef DEBUG
    UART.print("CAN send error");
    #endif
    canRestart();
  }
  // Send this frame
}

void uartToCAN(String messageUart);
/*
/* strtok example *//*
#include <stdio.h>
#include <string.h>

int main ()
{
  char str[] ="- This, a sample string.";
  char * pch;
  printf ("Splitting string \"%s\" into tokens:\n",str);
  pch = strtok (str," ,.-");
  while (pch != NULL)
  {
    printf ("%s\n",pch);
    pch = strtok (NULL, " ,.-");
  }
  return 0;
}

String myString;
char c;
int Index1,Index2,Index3, azi;
String secondValue, thirdValue;

 if (myString.length()>0)
{
Index1 = myString.indexOf(':');
Index2 = myString.indexOf(':', Index1+1);
Index3 = myString.indexOf(':', Index2+1);

secondValue = myString.substring(Index1+1, Index2);
thirdValue = myString.substring(Index2+1, Index3);


Serial.println(secondValue);
Serial.println(thirdValue);

myString="";
}
delay(1000);
}

char nibble2c(char c)
{
   if ((c>='0') && (c<='9'))
      return c-'0' ;
   if ((c>='A') && (c<='F'))
      return c+10-'A' ;
   if ((c>='a') && (c<='a'))
      return c+10-'a' ;
   return -1 ;
}
*/

void wakeUpBus() {
  log("send wake up");
  SendCANmessage(0x100, 0); // wake up bus?
  canBus.free();
}

// ======== LS CAN ====================================================
#ifdef CAN_GPIO_PINS_LS
// ======== IPC приборка ====================================
/**
   шлет цифры на дисплей ошибок
   три двухзначных числа
*/
void lsShowEcn(uint8 d0, uint8 d1, uint8 d2) {
//  log("==>sending message to ECN screen");
  SendCANmessage(0x5e8, 8, 0x81, d0, d1, d2, 0x00, 0x00, 0x00, 0x00);
//  log("==sent");
}

/**
 * Show one value (000000 -- 399999)
 *                           ^?
 */
void lsShowEcnDecimal(long value) { // Show one value
  uint8 d0 = 0x00;
  uint8 d1 = 0x00;
  uint8 d2 = 0x00;
  if (value >= 10000) { // а можно сделать циклом
    char d01 = value / 100000; // first digit
    value -= 100000 * d01;
    char d02 = value / 10000; // first digit
    value -= 10000 * d02;
    d0 = d01*0x10 + d02;
  }
  if (value >= 100) { 
    char d01 = value / 1000; // first digit
    value -= 1000 * d01;
    char d02 = value / 100; // first digit
    value -= 100 * d02;
    d1 = d01*0x10 + d02;
  }
  if (value > 0) { 
    char d01 = value / 10; // first digit
    value -= 10 * d01;
    char d02 = value  ; // first digit
    value -= d02;
    d2 = d01*0x10 + d02;
  }
  // debug("converted to HEX");
  lsShowEcn(d0, d1, d2);
}

/**
 * Show two values (000, 000 -- 399, 999)
 *                              ^?
 */
void lsShowEcnDecimal(long value1, long value2) { // Show two values (3 digits each)
  uint8 d0 = 0x00;
  uint8 d1 = 0x00;
  uint8 d2 = 0x00;

  /************ value2 ************/
  if (value2 > 999) { // too big
    d1 += 0x0E;   // ... E..
    while (value2>100){
      value2 -=100;
    }
  } else if (value2 >= 100) {
    char d01 = value2 / 100; // first digit
    value2 -= 100 * d01;

    d1 += d01;   // ... 1..
  }

  if (value2 > 0) {
    char d01 = value2 / 10; // first digit
    value2 -= 10 * d01;
    char d02 = value2  ; // first digit
    value2 -= d02;

    d2 = d01*0x10 + d02;   // ... .12
  }

  /************ value1 ************/
  if (value1 > 399) { // too big
    d0 = 0x0E; // 0E. ...
    d1+= 0xE0; // ..E ...

  } else {
    if (value1 >= 100) {
      char d01 = value1 / 100; // first digit
      value1 -= 100 * d01;

      d0 += d01 * 0x10; // 1.. ...
    }
    if (value1 > 0) {
      char d01 = value1 / 10; // first digit
      value1 -= 10 * d01;
      char d02 = value1  ; // second digit

      d0 += d01;  // .1. ...
      d1 += d02*0x10; // ..2 ...
    }
  }
//  debug("converted to HEX");
  lsShowEcn(d0, d1, d2);
}

void playWithEcn() {
  log("playing with ECN digits");
  delay(200);
  debug("111111");
  lsShowEcn(0x11, 0x11, 0x11);
  delay(200);
  debug("222222");
  lsShowEcn(0x22, 0x22, 0x22);
  delay(200);
  debug("333333");
  lsShowEcn(0x33, 0x33, 0x33);
  delay(200);
  debug("222222");
  lsShowEcn(0x22, 0x22, 0x22);
  delay(200);
  debug("111111");
  lsShowEcn(0x11, 0x11, 0x11);
  delay(200);
  debug("000000");
  lsShowEcn(0x00, 0x00, 0x00);
  delay(200);
  canBus.free();
}

/**
   Тестстрелок - кратковременно до максимума
*/
void panelCheck() {
  log("==>making panelCheck!");
  // delay(300);
  SendCANmessage(0x255, 8, 0x05, 0xAE, 0x06, 0x01, 0x8A, 0x00, 0x00, 0x00); // speed
  delay(50);
  SendCANmessage(0x255, 8, 0x05, 0xAE, 0x07, 0x01, 0x7F, 0x00, 0x00, 0x00); // tacho
  delay(50);
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x08, 0x01, 0xFF, 0x00, 0x00, 0x00); // fuel
  delay(900);
  SendCANmessage(0x255, 8, 0x05, 0xAE, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00);
  delay(50);
  SendCANmessage(0x255, 8, 0x05, 0xAE, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00);
  delay(50);
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00);
  log("== end making panelCheck!");
}

/**
   Вывод на спидометр
*/
void speedometer (int speed) {
  uint8 data = speed / 2;
  SendCANmessage(0x255, 8, 0x05, 0xAE, 0x06, 0x01, data, 0x00, 0x00, 0x00); // speed

}

/**
   Вывод на тахометр. Число от 0 до 80
*/
void tahometer (int taho) {
  uint8 data = map(taho, 0,80, 0x00, 0x7F);
SendCANmessage(0x255, 8, 0x05, 0xAE, 0x07, 0x01, data, 0x00, 0x00, 0x00); // tacho
}
// ======== Индикаторы =========
/**
   Индикатор непристегнутого ремня
*/
void lsIpcIndicatorNotFastenedOn() {
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x02, 0x08, 0x08, 0x00, 0x00, 0x00);
}
void lsIpcIndicatorNotFastenedOff() {
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x02, 0x08, 0x00, 0x00, 0x00, 0x00);
}

/**
   Индикатор на кнопке спорт
*/
void lsIpcIndicatorSportOn() {
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x0E, 0x20, 0x20, 0x00, 0x00, 0x00);
}
void lsIpcIndicatorSportOff() {
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x0E, 0x20, 0x00, 0x00, 0x00, 0x00);
}

/**
   Индикатор ESP
*/
void lsIpcIndicatorEspOn() {
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00);
}
void lsIpcIndicatorEspOff() {
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00);
}

/**
   Индикатор задних ПТФ
*/
void lsIpcIndicatorFogBackOn() {
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00);
}
void lsIpcIndicatorFogBackOff() {
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00);
}

// ======== CIM подрулевой ====================================
/**
    издать звуковой сигнал
      перерыв = wait*10 мс
      длительность = length*10мс
*/
void lsBeep(uint8 wait, uint8 count, uint8 length) {
//  log("==>making beep!");
  //пример сообщения ls.sendMessage(0x280,5,0x70,0x05,0x1e,0x03,0x33);
  SendCANmessage(0x280, 5, 0x70, 0x05, wait, count, length, 0, 0, 0);
//  log("== end making beep!");

}

/**
   Сигналы с задержкой 0,1 сек.
*/
void lsBeep(uint8 count) {
  lsBeep(0x0A, count, 0x33);
}
/**
    Стандартный сигнал как при включении полугабарита
*/
void lsBeep() {
  lsBeep(0x1e, 0x03, 0x33);
}


// ======== фары ====================================
/**
   Какая-то фара
*/
void lsLightsTurnLeftBackOn() {
  SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0xC0, 0xC0, 0x00, 0x00, 0x00);
}
void lsLightsTurnLeftBackOnOff() {
  SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);
}


// ======== разное ====================================

/**
 * Мигнуть два раза задними поворотниками
 */
void lsDoThanks(){
  debug("Start blinking back");
  lsBeep(0x01);
  // вкл задние аварийки
  SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0xC0, 0xC0, 0x00, 0x00, 0x00);
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x01, 0x04, 0x04, 0x00, 0x00, 0x00);
  delay(250);

  // выкл задние аварийки
  SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00);
  delay(250);

  // вкл задние аварийки
  SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0xC0, 0xC0, 0x00, 0x00, 0x00);
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x01, 0x04, 0x04, 0x00, 0x00, 0x00);
  delay(250);

  // выкл задние аварийки
  SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00);
  lsBeep(0x01);
  debug("Stop blinking back");
}

/**
 * Зажечь задние поворотники
 */
void lsBackTurnLights1000(){
#ifdef DEBUG
  debug("back turn lights on");
#endif
  lsBeep(0x04);
//  SendCANmessage(0x250, 8, 0x06, 0xAE, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00); // this is for FRONT lamps!
  SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0xC0, 0xC0, 0x00, 0x00, 0x00);
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00);
  delay(1000);

//  SendCANmessage(0x250, 8, 0x06, 0xAE, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);
  SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00);
}

void lsDoStrob(){
  debug("lsDoStrob. To turn off press knob down");
//  for (int i=0; i<1; i++){
  delay(300);
  lsShowEcn(0x10, 0x11, 0x11);
  lsBeep(0x08, 0x02, 0x20);
  // зажечь
  SendCANmessage(0x250, 8, 0x06, 0xAE, 0x02, 0x03, 0x02, 0x00, 0x00, 0x00);
  delay(100);
  // погасить
  SendCANmessage(0x250, 8, 0x06, 0xAE, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00);
  delay(100);
  // зажечь
  SendCANmessage(0x250, 8, 0x06, 0xAE, 0x02, 0x03, 0x02, 0x00, 0x00, 0x00);
  delay(100);
  // погасить
  SendCANmessage(0x250, 8, 0x06, 0xAE, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00);
  delay(100);
  // индикатор дальнего
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00);
  delay(300);

  lsShowEcn(0x11, 0x11, 0x01);
  // зажечь
  SendCANmessage(0x250, 8, 0x06, 0xAE, 0x02, 0x03, 0x01, 0x00, 0x00, 0x00);
  delay(100);
  // погасить
  SendCANmessage(0x250, 8, 0x06, 0xAE, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00);
  delay(100);
  // зажечь
  SendCANmessage(0x250, 8, 0x06, 0xAE, 0x02, 0x03, 0x01, 0x00, 0x00, 0x00);
  delay(100);
  // погасить
  SendCANmessage(0x250, 8, 0x06, 0xAE, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00);
  delay(100);
  // индикатор дальнего
  SendCANmessage(0x255, 8, 0x04, 0xAE, 0x01, 0x04, 0x04, 0x00, 0x00, 0x00);
//  }

}

void lsDoStops(){
  debug("lsDoStops. To turn off press knob down");

  lsIpcIndicatorFogBackOn();
  lsShowEcn(0x33, 0x88, 0xEE);
  lsBeep(0x08, 0x01, 0x20);

  // вкл задние аварийки
  // SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0xC0, 0xC0, 0x00, 0x00, 0x00);

  //Мигнуть допстопом times раз с интервалом del
  lsTopStopSignalBlink(3, 50);

  // выкл задние аварийки
  // SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);

  //Мигнуть допстопом times раз с интервалом del
  lsTopStopSignalBlink(3, 50);

  // индикатор
  lsIpcIndicatorFogBackOff();
}


/**
 * Зажечь допстоп
 */
void lsTopStopSignalSet(bool turnOn) {
  if (turnOn) {
  SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0x00, 0x00, 0x04, 0x04, 0x00); // 3-rd stop
  flagTopStopSignal = true;
//#ifdef DEBUG
//debug("STOP ON");
//#endif

  } else {
  SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00); // 3-rd stop OFF
  flagTopStopSignal = false;
//#ifdef DEBUG
//debug("STOP OFF");
//#endif
  }
}

/**
 * Изменить состояние допстоп
 */
void lsTopStopSignalSwitch() {
lsTopStopSignalSet(!flagTopStopSignal);
//#ifdef DEBUG
//debug("SWITCHING STOP");
//delay(300);
//#endif
}

/**
 * Мигнуть допстопом times раз с интервалом del
 */
void lsTopStopSignalBlink(byte times, int del) {
  for (byte i=0;i<times;i++){
    lsTopStopSignalSet(true);
    delay(del);
    lsTopStopSignalSet(false);
    delay(del);
  }
}

/**
 * Погасить допстоп
void lsTopStopSignalUnset() {
  SendCANmessage(0x251, 8, 0x06, 0xAE, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00); // 3-rd stop OFF
  flagTopStopSignal = false;
}
 */

/**
 *  Закрыть окна
 *  Использует глобальные переменные keyNum, keyCode0, keyCode1
 */
void lsCloseWindows() {
  lsTopStopSignalSet(true); // включаю верхний стоп
  SendCANmessage(0x160, 4, keyNum, 0x80, keyCode0, keyCode1, 0, 0, 0, 0); // hold close on remote
  delay(500);
  SendCANmessage(0x160, 4, keyNum, 0xC0, keyCode0, keyCode1, 0, 0, 0, 0); // hold close on remote
  delay(4000);
  SendCANmessage(0x160, 4, keyNum, 0x00, keyCode0, keyCode1, 0, 0, 0, 0); //release on remote
  lsTopStopSignalSet(false); // выключаю верхний стоп
}
/**
 *  Открыть окна
 */
void lsOpenWindows(bool half) {
  int holdDelay = half?2000:4000;
  lsTopStopSignalSet(true); // включаю верхний стоп
  delay(500);
  SendCANmessage(0x160, 4, keyNum, 0x30, keyCode0, keyCode1, 0, 0, 0, 0); // hold open on remote
  delay(holdDelay);
  SendCANmessage(0x160, 4, keyNum, 0x00, keyCode0, keyCode1, 0, 0, 0, 0); //release on remote
  lsTopStopSignalSet(false); // выключаю верхний стоп
}
void lsOpenWindows() {
lsOpenWindows(false);
}

/**
  must send periodically
  test lsId305Data[i]  may cause fantom button pressing
*/
void lsSendSportOn(void){
  SendCANmessage(0x305, 7,
    lsId305Data[0],
    lsId305Data[1],
    lsId305Data[2],
    lsId305Data[3],
    lsId305Data[4] | 0x3A,
    lsId305Data[5] | 0x81,
    lsId305Data[6],
    0x00
  ); //  0	 0	 0	 0	 3A	 81	 0
}

/**
  must send periodically
*/
void lsSendEspOff(void){
  SendCANmessage(0x305, 7,
    lsId305Data[0],
    lsId305Data[1],
    lsId305Data[2],
    lsId305Data[3] | 0x08,
    lsId305Data[4] | 0x3B,
    lsId305Data[5] | 0x81,
    lsId305Data[6],
    0x00
  ); //  0	 0	 0	 0x08 3B 81 0
}

// == на будущее
// 251#04.AE.03.04.04.00.00.00 открытие багажника
void lsOpenRearDoor(){
  SendCANmessage(0x251, 8, 0x04, 0xAE, 0x03, 0x04, 0x04, 0x00, 0x00, 0x00);
}

/* заметки
LS CAN
305   [7]  00 00 40 00 10 40 00 включеные габаритов
305   [7]  00 00 C0 00 10 40 00 включен ближний
305   [7]  00 00 00 00 3A 41 00 включен sport-режим

350   [4]  06 02 00 00 птф выключены 
350   [4]  26 02 00 00 передние птф включены

230   [7]  00 00 40 00 00 00 00 открыта левая дверь
230   [7]  00 00 10 00 00 00 00 открыта правая дверь
230   [7]  00 00 50 00 00 00 00 открыты обе двери
230   [7]  00 40 00 00 00 00 00 открыта левая задняя дверь
230   [7]  00 10 00 00 00 00 00 открыта правая задняя дверь
230   [7]  00 50 00 00 00 00 00 открыты обе задние двери
230   [7]  00 00 04 00 00 00 00 открыт багажник

230   [7]  05 00 00 00 00 09 00 двери заперты

11A   [7]  01 11 81 08 01 00 00 - первая передача
11A   [7]  02 11 82 08 01 00 00 - вторая передача
11A   [7]  03 11 83 08 01 00 00 - третья передача
11A   [7]  0B 11 8B 08 01 00 00 - D1
11A   [7]  0B 31 8B 18 01 00 00 - D3 (при включенном зимнем режиме)
11A   [7]  09 91 89 68 01 00 00 - нейтралка
11A   [7]  08 81 80 70 01 00 00 - задняя
11A   [7]  0A A1 8A 78 01 00 00 - паркинг

145   [8]  04 01 18 8F E0 04 00 00 - зимний режим включен
145   [8]  00 01 18 8F E0 04 00 00 - зимний режим выключен
*/

#endif
// ======== MS CAN ==========
#ifdef CAN_GPIO_PINS_MS
void msAcTrigger()
{
  digitalWrite(PC13, PC13ON);
  delay(DELAY);
  // turn right 1 click
  log("turn right 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0x01);
  delay(DELAY);
  // press
  log("press");
  SendCANmessage(0x208, 6, 0x01, 0x17, 0x00);
  delay(70);
  SendCANmessage(0x208, 6, 0x00, 0x17, 0x00);
  delay(DELAY);

  // turn left 1 click
  log("turn left 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);
  delay(30);
  // turn left 1 click
  log("turn left 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);
  delay(DELAY);
  // turn left 1 click
  log("turn left 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);
  delay(30);
  // turn left 1 click
  log("turn left 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);
  delay(DELAY);
  // press
  log("press");
  SendCANmessage(0x208, 6, 0x01, 0x17, 0x00);
  delay(70);
  SendCANmessage(0x208, 6, 0x00, 0x17, 0x00);
  delay(DELAY);
}

void msWakeUpScreen() {
  SendCANmessage(0x697, 8, 0x47, 0x00, 0x60, 0x00, 0x02, 0x00, 0x00, 0x80); // wake up screen
  log("send wake screen");
  canBus.free();
}
#endif