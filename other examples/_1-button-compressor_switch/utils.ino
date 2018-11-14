void debug(String str) {
#ifdef DEBUG
  SERIAL.print(millis());
  SERIAL.print("\t");
  SERIAL.println(str);
#endif
}

void log(String str) {
#ifdef LOG
  SERIAL.println(str);
#endif
}

void msCANSetup(void)
{
  CAN_STATUS Stat ;
  afio_init(); // this will restart subsystem and make pins A11A12 work
  // Initialize CAN module
  canBus.map(CAN_GPIO_PINS_MS);
  Stat = canBus.begin(CAN_SPEED_95, CAN_MODE_NORMAL);

  canBus.filter(0, 0, 0);
  //  canBus.filter(0, 0x206 << 21, 0xFFFFFFFF) ;   // filter 0 only allows standard identifier 0x206
  //  canBus.filter(1, 0x208 << 21, 0xFFFFFFFF) ;   // filter 1 only allows standard identifier 0x208
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
  afio_init(); // this will restart subsystem and make pins A11A12 work
  CAN_STATUS Stat ;
  // Initialize CAN module
  canBus.map(CAN_GPIO_PINS_LS);
  Stat = canBus.begin(CAN_SPEED_33, CAN_MODE_NORMAL);

  canBus.filter(0, 0, 0);
  //  canBus.filter(0, 0x206 << 21, 0xFFFFFFFF) ;   // filter 0 only allows standard identifier 0x206
  //  canBus.filter(1, 0x208 << 21, 0xFFFFFFFF) ;   // filter 1 only allows standard identifier 0x208
  canBus.set_irq_mode();              // Use irq mode (recommended)
  Stat = canBus.status();
  if (Stat != CAN_OK)
  { // Initialization failed
    digitalWrite(PC13, LOW);
    delay(10000);
    log("Initialization failed");
  }
}

CAN_TX_MBX CANsend(CanMsg *pmsg) // Should be moved to the library?!
{
  CAN_TX_MBX mbx;
  do
  {
    mbx = canBus.send(pmsg) ;
  }
  while (mbx == CAN_TX_NO_MBX) ;      // Waiting outbound frames will eventually be sent, unless there is a CAN bus failure.
  return mbx ;
}

// Send message
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
  CANsend(&msg) ;      // Send this frame
}

void AC_trigger()
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

void wakeUpBus() {
  log("send wake up");
  SendCANmessage(0x100, 0); // wake up bus?
}

void wakeUpScreen() {
  SendCANmessage(0x697, 8, 0x47, 0x00, 0x60, 0x00, 0x02, 0x00, 0x00, 0x80); // wake up screen
  log("send wake screen");
}

/**
   шлет цифры на дисплей ошибок
   три двухзначных числа
*/
void showEcn(uint8 d0, uint8 d1, uint8 d2) {
  log("==>sending message to ECN screen");
  SendCANmessage(0x5e8, 8, 0x81, d0, d1, d2, 0x00, 0x00, 0x00, 0x00);
  log("==sent");
}

void playWithEcn() {
  log("playing with ECN digits");
  delay(50);
  debug("000000");
  showEcn(0x00, 0x00, 0x00);
  delay(200);
  debug("111111");
  showEcn(0x11, 0x11, 0x11);
  delay(200);
  debug("222222");
  showEcn(0x22, 0x22, 0x22);
  delay(200);
  debug("333333");
  showEcn(0x33, 0x33, 0x33);
  delay(200);
  debug("222222");
  showEcn(0x22, 0x22, 0x22);
  delay(200);
  debug("111111");
  showEcn(0x11, 0x11, 0x11);
  delay(200);
  debug("000000");
  showEcn(0x00, 0x00, 0x00);
  canBus.free();
}

/**
   издать звуковой сигнал
*/
void lsBeep(uint8 wait = 0x1e, uint8 count = 0x03, uint8 length = 0x33) {
  log("==>making beep!");
  //пример сообщения ls.sendMessage(0x280,5,0x70,0x05,0x1e,0x03,0x33);
  SendCANmessage(0x280, 5, 0x70, 0x05, wait, count, length, 0, 0, 0);
  log("== end making beep!");

}

void lsBeep() {
  lsBeep(0x1e, 0x03, 0x33);
}
