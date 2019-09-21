#include <HardwareCAN.h>
/*
   Uses STM32duino with Phono patch. Must add 33 and 95 CAN speeds
*/
/////// === Настройки модуля! === ///////
// Choose output serial port
#define UART Serial2
// Choose CAN pins
#define CAN_GPIO_PINS_MS CAN_GPIO_PA11_PA12
#define CAN_GPIO_PINS_LS CAN_GPIO_PB8_PB9
// Uncomment to enable 'debug()' messages output
#define DEBUG
// Uncomment to enable 'log()' messages output
#define LOG

#define PC13ON 0
#define PC13OFF 1
#define DELAY 200
/* global variables */
volatile bool flag_blocked;
// Instanciation of CAN interface
HardwareCAN canBus(CAN1_BASE);
CanMsg msg ;
CanMsg *r_msg;

void setup()
{
  UART.begin(115200); // output to A2 pin
  UART.println("Hello World!");
  UART.println("Starting \"1-button-compressor switch plus\" v21 2018-11-22");
  debug("checking debug level");
  log("checking log level");


  pinMode(PC13, OUTPUT); // LED
  digitalWrite(PC13, PC13ON);
  delay(50);// to see the LED
  log("Initialize the CAN module ...");
  
  msCANSetup();        // Initialize the CAN module
  log("Initialization MS CAN ON");
  flag_blocked = false;
  log("flag_blocked is set to " + flag_blocked);
  wakeUpBus();
  delay(100);
  wakeUpScreen(); // only MS
  delay(100);

  lsCANSetup();        // Initialize the CAN module
  log("Initialization LS CAN ON");
  wakeUpBus();
  lsBeep(2);
  lsPanelCheck();
  
  playWithEcn();

  msCANSetup();
  log("end set up");
  digitalWrite(PC13, PC13OFF);
}

void loop()
{
  //  debug("loop");
  while ( ( r_msg = canBus.recv() ) != NULL )
  {
    ///// processing the incoming message
    if (r_msg->ID == 0x206) // steering wheel buttons
    {
      debug("steering wheel buttons");
      // setting the flag_blocked flag [01 нажата кнопка] [81 пресет/верхняя на руле] []
      if (r_msg->Data[1] == 0x81)
      {
        if (r_msg->Data[0] == 0x01)
        {
          flag_blocked = true;
          digitalWrite(PC13, PC13ON);
          log("Blocking button is pressed");
        }
        else
        {
          flag_blocked = false;
          digitalWrite(PC13, PC13OFF);
          Serial2.println("Blocking button is released");
        }
      }
    }
    else if (r_msg->ID == 0x208) //climate controls
    {
      debug("climate controls");
      // check block button pressed
      if (flag_blocked)
      { // if block pressed, just skip it
        // do nothing
      }
      else
      {
        // check if the climate control menu is pressed
        if (
          (r_msg->Data[0] == 0x01) and
          (r_msg->Data[1] == 0x17) and
          (r_msg->Data[2] == 0x00) //?
        )
        { // AC triggering script
          log("blocking is NOT pressed");
          log("Running AC triggering script");
          AC_trigger();
          log("Done.");
          digitalWrite(PC13, PC13OFF);
        }
        //end AC triggering script
      }
      // end if
    }
    canBus.free();

  }
  // close while
}
// close void loop
