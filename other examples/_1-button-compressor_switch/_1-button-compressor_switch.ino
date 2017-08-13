//#define DEBUGMODE

#include <HardwareCAN.h>
/*
 * Uses STM32duino with Phono patch. Must add 33 and 95 CAN speeds
 */

// Instanciation of CAN interface
HardwareCAN canBus(CAN1_BASE);
CanMsg msg ;
CanMsg *r_msg;

void CANSetup(void) // Should be moved to the library and choose speed here?!
{
  CAN_STATUS Stat ;

  // Initialize CAN module
  canBus.map(CAN_GPIO_PB8_PB9);       // This setting is already wired in the Olimexino-STM32 board
  Stat = canBus.begin(CAN_SPEED_95, CAN_MODE_NORMAL);    

  canBus.filter(0, 0, 0);
  canBus.set_irq_mode();              // Use irq mode (recommended), so the handling of incoming messages
                                      // will be performed at ease in a task or in the loop. The software fifo is 16 cells long, 
                                      // allowing at least 15 ms before processing the fifo is needed at 125 kbps
  Stat = canBus.status();
  if (Stat != CAN_OK)  
     {/* Your own error processing here */ ;   // Initialization failed
     Serial.print("Initialization failed");
     Serial1.print("Initialization failed");
     }
}

CAN_TX_MBX CANsend(CanMsg *pmsg) // Should be moved to the library?!
{
  CAN_TX_MBX mbx;
  do 
  {
    mbx = canBus.send(pmsg) ;
#ifdef USE_MULTITASK
    vTaskDelay( 1 ) ;                 // Infinite loops are not multitasking-friendly
#endif
  }
  while(mbx == CAN_TX_NO_MBX) ;       // Waiting outbound frames will eventually be sent, unless there is a CAN bus failure.
  return mbx ;
}

// Send message
// Prepare and send a frame containing some value 
void SendCANmessage(long id=0x001, byte dlength=8, byte d0=0x00, byte d1=0x00, byte d2=0x00, byte d3=0x00, byte d4=0x00, byte d5=0x00, byte d6=0x00, byte d7=0x00)
{
  // Initialize the message structure
  // A CAN structure includes the following fields:
  msg.IDE = CAN_ID_STD;          // Indicates a standard identifier ; CAN_ID_EXT would mean this frame uses an extended identifier
  msg.RTR = CAN_RTR_DATA;        // Indicated this is a data frame, as opposed to a remote frame (would then be CAN_RTR_REMOTE)
  msg.ID = id ;                  // Identifier of the frame : 0-2047 (0-0x3ff) for standard idenfiers; 0-0x1fffffff for extended identifiers
  msg.DLC = dlength;                   // Number of data bytes to follow

  // Prepare frame : send something
  msg.Data[0] = d0 ;
  msg.Data[1] = d1 ;
  msg.Data[2] = d2 ;
  msg.Data[3] = d3 ;
  msg.Data[4] = d4 ;
  msg.Data[5] = d5 ;
  msg.Data[6] = d6 ;
  msg.Data[7] = d7 ;

//  digitalWrite(PC13, LOW);    // turn the onboard LED on
  CANsend(&msg) ;      // Send this frame
/*		// print a copy of sent message
		Serial1.println("Sending msg:");
		Serial1.print(id, HEX);
		Serial1.print(" #");
		
		for (int i=0; i<dlength;++i)
		{
			Serial1.print(" ");
			Serial1.print(msg.Data[i], HEX);
		}
		Serial1.println();
  
//  delay(18);              
//  digitalWrite(PC13, HIGH);   // turn the LED off 
//  delay(10);  
*/
}

#define PC13ON 0
#define PC13OFF 1
#define DELAY 250
/* global variables */
volatile bool blocked;
volatile bool usbMode;
/* climate control */
volatile int climate_temperature;
//volatile enum { } climate_temperature;
volatile int climate_fanspeed;
//volatile enum { all=0x52, up,up_middle, middle, middle_down, down, up_down, dir_auto} climate_direction;
volatile uint8 climate_direction;


void setup() 
{
	Serial1.begin(115200);
	Serial1.println("Hello World!");
	Serial1.println("Starting \"1-button-compressor switch\" v8 program");

	pinMode(PC13, OUTPUT); // LED
	digitalWrite(PC13, PC13ON);
	
	pinMode(28, OUTPUT); // B12 = 16+12 = 28
	digitalWrite(28, LOW);
	pinMode(28, INPUT); // B12 = 16+12 = 28
	usbMode = digitalRead(28);
	if (usbMode)
	{
		Serial1.println("Entering USB mode");
		Serial.begin(115200);
		Serial.println("Entering USB mode");
		while (1)
		{
			Serial.print(".");
			Serial.print(".");
			delay(1000);
		}
	}
	else
	{
	  // put your setup code here, to run once:
	  digitalWrite(PC13, PC13OFF);
//	  Serial1.println("Initialize the CAN module ...");
	  CANSetup() ;        // Initialize the CAN module and prepare the message structures.
//	  Serial1.println("Initialization OK");
	  blocked=false;
//	  SendCANmessage(0x208, 0); // dummy
	  SendCANmessage(0x697, 8, 0x47, 0x00, 0x60, 0x00, 0x02, 0x00, 0x00, 0x80); // wake up screen
	}
}

void loop() 
{
while ( ( r_msg = canBus.recv() ) != NULL ) 
{
	#ifdef DEBUGMODE	
			// printing data to serial. may be switched off for faster work
			digitalWrite(PC13, PC13ON); // LED shows that recieved data is being printed out
			Serial1.print(r_msg->ID, HEX);
			Serial1.print(" # ");
			Serial1.print(r_msg->Data[0], HEX);
			Serial1.print(" ");
			Serial1.print(r_msg->Data[1], HEX);
			Serial1.print(" ");
			Serial1.print(r_msg->Data[2], HEX);
			Serial1.print(" ");
			Serial1.print(r_msg->Data[3], HEX);
			Serial1.print(" ");
			Serial1.print(r_msg->Data[4], HEX);
			Serial1.print(" ");
			Serial1.print(r_msg->Data[5], HEX);
			Serial1.print(" ");
			Serial1.print(r_msg->Data[6], HEX);
			Serial1.print(" ");
			Serial1.println(r_msg->Data[7], HEX);
			digitalWrite(PC13, PC13OFF);
	#endif
	///// processing the incoming message
    switch(r_msg->ID) 
	{
	case 0x206: // steering wheel buttons
		
		// setting the blocked flag [08] [83 левое колесико] [01 вниз]
		if (r_msg->Data[1]==0x82) 
		{
			if (r_msg->Data[0]==0x01)	
			{
				blocked = true;
//				Serial1.println("Blocking button is pressed");
			}
			else 
			{
				blocked = false;
//				Serial1.println("Blocking button is released");
			}
		}
	break;
	
    case 0x208: //climate controls
		// check OK button pressed
		if (blocked == false)
		{	// if OK pressed, just skip it
			// check if the climate control menu is pressed
			if ( 
				(r_msg->Data[0]==0x01) and 
				(r_msg->Data[1]==0x17) and 
				(r_msg->Data[2]==0x00)
				)
			{   // AC triggering script
				Serial1.println("OK is NOT pressed");
				Serial1.println("Running AC triggering script");
				digitalWrite(PC13, PC13ON);
				delay(DELAY);
				// turn right 1 click
				Serial1.println("turn right 1 click");
				SendCANmessage(0x208, 6, 0x08, 0x16, 0x01); 
				delay(DELAY);
				// press
				Serial1.println("press");
				SendCANmessage(0x208, 6, 0x01, 0x17, 0x00);
				delay(30);
				SendCANmessage(0x208, 6, 0x00, 0x17, 0x00);
				delay(DELAY);
				
				// turn left 1 click
				Serial1.println("turn left 1 click");
				SendCANmessage(0x208, 6, 0x08, 0x16, 0xff); 
				delay(DELAY);
				// turn left 1 click
				Serial1.println("turn left 1 click");
				SendCANmessage(0x208, 6, 0x08, 0x16, 0xff); 
				delay(DELAY);
				// press
				Serial1.println("press");
				SendCANmessage(0x208, 6, 0x01, 0x17, 0x00);
				delay(30);
				SendCANmessage(0x208, 6, 0x00, 0x17, 0x00);
				delay(DELAY);
				
				
				Serial1.println("Done.");
				digitalWrite(PC13, PC13OFF);
			}
		} 
		else 
		{
			
		} // end if
	break;
	
	#ifdef DEBUGMODE	
	case 0x6C8: // 	climate info
	switch (r_msg->Data[0]) // mode? 
	{
		case 0x21: // normal mode, change flow direction
		// r_msg->Data[2] and [7] is direction, 52 - 59 
		switch (r_msg->Data[1])
		{
			case 0xE0:
			climate_direction = r_msg->Data[2];
			break;
		}		
		
		break;
		
		case 0x22: // normal mode, change flow speed or temperature
		switch (r_msg->Data[1])	
		{
			case 0x03: // temperature set
			climate_temperature = 10 * (r_msg->Data[3] - 0x30) + r_msg->Data[5] - 0x30; // make here only registering and move calculation to output zone
			break;
		}			
		break;
		
		case 0x24: // normal mode, auto flow? status
		// 4 is speed, 30 - 37
		climate_fanspeed = r_msg->Data[3] - 0x30;
		break;
		
		case 0x25: // normal mode, auto flow speed, status
		// 4 is E0 = full auto speed, 41 = manual flow direction
		break;
		
		case 0x26: // air distribution mode, 
		// [7] is flow direction , 52 - 59
		break;
	}
	break;
		
	default:
	break;
	#endif
	} // close switch

	canBus.free();

	///// here make all low priority processing. Later should be moved to timer interrupt?
	#ifdef DEBUGMODE	
	// output fan speed
	Serial1.print("fan speed = ");
	if (climate_fanspeed  < 8)
	{
		Serial1.println(climate_fanspeed);
	}
	else 
	{
		Serial1.println("AUTO");		
	}
	
	// 	output temperature
	Serial1.print("tempr setpoint = ");
	Serial1.println(climate_temperature);
	
	// output climate_direction
	Serial1.println("flow directon:");
	switch (climate_direction)
	{ 
		case 0x52: //all:
		Serial1.println("up | mid | down");
		break;
		case 0x53: //up:
		Serial1.println("up | ___ | ____");
		break;
		case 0x54: //up_middle:
		Serial1.println("up | mid | ____");
		break;
		case 0x55: //middle:
		Serial1.println("__ | mid | ____");
		break;
		case 0x56: //middle_down:
		Serial1.println("__ | mid | down");
		break;
		case 0x57: //down:
		Serial1.println("__ | ___ | down");
		break;
		case 0x58: //up_down:
		Serial1.println("up | ___ | down");
		break;
		case 0x59: //dir_auto:
		Serial1.println("AUTO");
		break;
	} 
	#endif

	
} // close while
  
} // close void loop
