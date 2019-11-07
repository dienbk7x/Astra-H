
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
   Print out received message to UART out
*/
void printMsg(CanMsg *r_msg) {
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


void AC_trigger() {

  int AC_TRIGGER_DELAY = 200;

  digitalWrite(PC13, PC13ON);
  delay(AC_TRIGGER_DELAY);
  // turn right 1 click
  log("turn right 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0x01);
  delay(AC_TRIGGER_DELAY);
  // press
  log("press");
  SendCANmessage(0x208, 6, 0x01, 0x17, 0x00);
  delay(70);
  SendCANmessage(0x208, 6, 0x00, 0x17, 0x00);
  delay(AC_TRIGGER_DELAY);

  // turn left 1 click
  log("turn left 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);
  delay(30);
  // turn left 1 click
  log("turn left 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);
  delay(AC_TRIGGER_DELAY);
  // turn left 1 click
  log("turn left 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);
  delay(30);
  // turn left 1 click
  log("turn left 1 click");
  SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);
  delay(AC_TRIGGER_DELAY);
  // press
  log("press");
  SendCANmessage(0x208, 6, 0x01, 0x17, 0x00);
  delay(70);
  SendCANmessage(0x208, 6, 0x00, 0x17, 0x00);
  delay(AC_TRIGGER_DELAY);
}