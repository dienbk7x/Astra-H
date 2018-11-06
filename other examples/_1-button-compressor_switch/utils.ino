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

void CANSetup(void)
{
  CAN_STATUS Stat ;
  // Initialize CAN module
  canBus.map(CAN_GPIO_PA11_PA12);
  Stat = canBus.begin(CAN_SPEED_95, CAN_MODE_NORMAL);

  //  canBus.filter(0, 0, 0);
  canBus.filter(0, 0x206 << 21, 0xFFFFFFFF) ;   // filter 0 only allows standard identifier 0x206
  canBus.filter(1, 0x208 << 21, 0xFFFFFFFF) ;   // filter 1 only allows standard identifier 0x208
  canBus.set_irq_mode();              // Use irq mode (recommended)
  Stat = canBus.status();
  if (Stat != CAN_OK)
  { // Initialization failed
    Serial2.print("Initialization failed");
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
  Serial2.println("turn right 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0x01);
  delay(DELAY);
  // press
  Serial2.println("press");
  SendCANmessage(0x208, 6, 0x01, 0x17, 0x00);
  delay(70);
  SendCANmessage(0x208, 6, 0x00, 0x17, 0x00);
  delay(DELAY);

  // turn left 1 click
  Serial2.println("turn left 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);
  delay(30);
  // turn left 1 click
  Serial2.println("turn left 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);
  delay(DELAY);
  // turn left 1 click
  Serial2.println("turn left 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);
  delay(30);
  // turn left 1 click
  Serial2.println("turn left 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);
  delay(DELAY);
  // press
  Serial2.println("press");
  SendCANmessage(0x208, 6, 0x01, 0x17, 0x00);
  delay(70);
  SendCANmessage(0x208, 6, 0x00, 0x17, 0x00);
  delay(DELAY);
}

void wakeUpBus() {
  Serial2.println("send wake up");
  SendCANmessage(0x100, 0); // wake up bus?
}

void wakeUpScreen() {
  SendCANmessage(0x697, 8, 0x47, 0x00, 0x60, 0x00, 0x02, 0x00, 0x00, 0x80); // wake up screen
  Serial2.println("send wake screen");
}
