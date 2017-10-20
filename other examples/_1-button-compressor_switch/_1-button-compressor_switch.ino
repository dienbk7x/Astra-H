#include <HardwareCAN.h>
/*
 * Uses STM32duino with Phono patch. Must add 33 and 95 CAN speeds
 */

// Instanciation of CAN interface
HardwareCAN canBus(CAN1_BASE);
CanMsg msg ;
CanMsg *r_msg;

void CANSetup(void) 
{
  CAN_STATUS Stat ;
  // Initialize CAN module
  canBus.map(CAN_GPIO_PB8_PB9);
  Stat = canBus.begin(CAN_SPEED_95, CAN_MODE_NORMAL);    

  canBus.filter(0, 0, 0);
  canBus.set_irq_mode();              // Use irq mode (recommended)
  Stat = canBus.status();
  if (Stat != CAN_OK)  
     {// Initialization failed
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
  while(mbx == CAN_TX_NO_MBX) ;       // Waiting outbound frames will eventually be sent, unless there is a CAN bus failure.
  return mbx ;
}

// Send message
void SendCANmessage(long id=0x100, byte dlength=8, byte d0=0x00, byte d1=0x00, byte d2=0x00, byte d3=0x00, byte d4=0x00, byte d5=0x00, byte d6=0x00, byte d7=0x00)
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

#define PC13ON 0
#define PC13OFF 1
#define DELAY 180
/* global variables */
volatile bool flag_blocked;

void setup() 
{
	Serial2.begin(115200); // output to A2 pin
	Serial2.println("Hello World!");
	Serial2.println("Starting \"1-button-compressor switch\" v11 final lite");

	pinMode(PC13, OUTPUT); // LED
	digitalWrite(PC13, PC13ON);
	
	digitalWrite(PC13, PC13OFF);
	Serial2.println("Initialize the CAN module ...");
	CANSetup() ;        // Initialize the CAN module and prepare the message structures.
	Serial2.println("Initialization OK");
	flag_blocked=false;
	SendCANmessage(0x100, 0); // wake up bus?
	SendCANmessage(0x697, 8, 0x47, 0x00, 0x60, 0x00, 0x02, 0x00, 0x00, 0x80); // wake up screen
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
	delay(30);
	SendCANmessage(0x208, 6, 0x00, 0x17, 0x00);
	delay(DELAY);

	// turn left 1 click
	Serial2.println("turn left 1 click");
	SendCANmessage(0x208, 6, 0x08, 0x16, 0xff); 
	delay(DELAY);
	// turn left 1 click
	Serial2.println("turn left 1 click");
	SendCANmessage(0x208, 6, 0x08, 0x16, 0xff); 
	delay(DELAY);
	// press
	Serial2.println("press");
	SendCANmessage(0x208, 6, 0x01, 0x17, 0x00);
	delay(30);
	SendCANmessage(0x208, 6, 0x00, 0x17, 0x00);
	delay(DELAY);
}

void loop() 
{
	while ( ( r_msg = canBus.recv() ) != NULL ) 
	{
		///// processing the incoming message
		if (r_msg->ID==0x206) // steering wheel buttons
		{

			// setting the flag_blocked flag [08] [83 левое колесико] [01 вниз]
			if (r_msg->Data[1]==0x83) 
			{
				if (r_msg->Data[0]==0x01)	
				{
					flag_blocked = true;
					digitalWrite(PC13, PC13ON);
					Serial2.println("Blocking button is pressed");
				}
				else 
				{
					flag_blocked = false;
					digitalWrite(PC13, PC13OFF);
					Serial2.println("Blocking button is released");
				}
			}
		}	
		else if (r_msg->ID==0x208) //climate controls
		{
			// check block button pressed
			if (flag_blocked)
			{   // if block pressed, just skip it
				// do nothing
			}
			else 
			{	
				// check if the climate control menu is pressed
				if ( 
					(r_msg->Data[0]==0x01) and 
					(r_msg->Data[1]==0x17) and 
					(r_msg->Data[2]==0x00) //?
					)
				{   // AC triggering script
				Serial2.println("blocking is NOT pressed");
				Serial2.println("Running AC triggering script");
				AC_trigger();
				Serial2.println("Done.");
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
