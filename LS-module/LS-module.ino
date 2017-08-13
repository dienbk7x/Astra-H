#include <HardwareCAN.h>
/* a copy of MS-module !!!
 * 
 */
#define DEBUGMODE
#define T_DELAY 10
// Instanciation of CAN interface
HardwareCAN canBus(CAN1_BASE);
CanMsg msg ;
CanMsg *r_msg;

///// basic functions for CAN  /////
///
void CAN_a_33_Setup(void)
{

  CAN_STATUS Stat ;
  afio_init(); // this will restart subsystem and make it work!
  canBus.map(CAN_GPIO_PA11_PA12);  
  Stat = canBus.begin(CAN_SPEED_33, CAN_MODE_NORMAL);
  canBus.filter(0, 0, 0);
  canBus.set_irq_mode();
  Stat = canBus.status();
  if (Stat != CAN_OK)
  {digitalWrite(PC13, LOW);
   }
// /* Your own error processing here */  ;  // Initialization failed
 }

void CAN_b_95_Setup(void)
{
  CAN_STATUS Stat ;
  canBus.map(CAN_GPIO_PB8_PB9);
  Stat = canBus.begin(CAN_SPEED_95, CAN_MODE_NORMAL);
  canBus.filter(0, 0, 0);
  canBus.set_irq_mode();
  Stat = canBus.status();
  if (Stat != CAN_OK)
  {digitalWrite(PC13, LOW);
   }
//     /* Your own error processing here */ ;   // Initialization failed
//  delay(T_DELAY);
}


CAN_TX_MBX CANsend(CanMsg *pmsg) // Should be moved to the library?!
{
  CAN_TX_MBX mbx;

//  do 
//  {
    mbx = canBus.send(pmsg) ;
#ifdef USE_MULTITASK
    vTaskDelay( 1 ) ;                 // Infinite loops are not multitasking-friendly
#endif
//  }
//  while(mbx == CAN_TX_NO_MBX) ;       // Waiting outbound frames will eventually be sent, unless there is a CAN bus failure.
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

  digitalWrite(PC13, LOW);    // turn the onboard LED on
  CANsend(&msg) ;      // Send this frame            
  digitalWrite(PC13, HIGH);   // turn the LED off 
  delay(T_DELAY);  
}

#define PC13ON 0
#define PC13OFF 1
#define DELAY 250
/* global variables */
volatile bool blocked;
volatile bool usbMode;
volatile bool climChanged;
/* climate control */
volatile uint8 climate_temperature=0;
//volatile enum { } climate_temperature;
volatile uint8 climate_fanspeed=0;
//volatile enum { all=0x52, up,up_middle, middle, middle_down, down, up_down, dir_auto} climate_direction;
volatile uint8 climate_direction=0;
volatile uint8 clim1;
volatile uint8 clim2;
volatile uint8 clim3;


// The application program starts here
void setup() {        // Initialize the CAN module and prepare the message structures.
	Serial2.begin(115200); // USART2 on A2-A3 pins
	Serial2.println("Hello World!");
	Serial2.println("Starting \"AstraH_climate\" v0.01 program");
	pinMode(PC13, OUTPUT); // LED
	digitalWrite(PC13, PC13ON);
	pinMode(28, INPUT); // B12 = 16+12 = 28
	usbMode = digitalRead(28);
	if (usbMode)
	{
		Serial2.println("Entering USB mode");
		Serial.begin(115200);
		Serial.println("Entering USB mode");
		while (1)
		{
			Serial.print(".");
			Serial2.print(".");
			delay(1000);
		}
	}
	else
	{
		// put your setup code here, to run once:
		blocked=false;
		CAN_b_95_Setup();
		SendCANmessage(0x201,3,0x01,0xFF,0x00); // press settings      
		delay(50);
		SendCANmessage(0x201,3,0x00,0xFF,0x00); // release settings      
		delay(50);
		
		
		CAN_a_33_Setup();
		delay(50);
		SendCANmessage(0x5E8,8,0x81,0x00,0x00,0x00);       
		delay(200);
		SendCANmessage(0x5E8,8,0x81,0x11,0x11,0x11);       
		delay(200);
		SendCANmessage(0x5E8,8,0x81,0x22,0x22,0x22);       
		delay(200);
		SendCANmessage(0x5E8,8,0x81,0x33,0x33,0x33);       
		delay(200);
		SendCANmessage(0x5E8,8,0x81,0x22,0x22,0x22);       
		delay(200);
		SendCANmessage(0x5E8,8,0x81,0x11,0x11,0x11);       
		delay(200);
		SendCANmessage(0x5E8,8,0x81,0x00,0x00,0x00);       
		canBus.free();
		digitalWrite(PC13, PC13OFF);
		clim1 = clim2 = clim3 = 0x00;
		climChanged = false;
	}
}

void loop() {
/**/
	CAN_b_95_Setup();
//	Serial2.println("started CAN 95");
	// read MS CAN and record data into variables
//	for (byte i=0;i<0xBB;i++)
//	{
		/* start read MS CAN */
	  CanMsg *r_msg;
		r_msg = canBus.recv();
//		while ((r_msg = canBus.recv()) != NULL ) 
		{
		Serial2.println("started while cycle");
		delay(50);
		#ifdef DEBUGMODE	
		
		// printing data to serial. may be switched off for faster work
				digitalWrite(PC13, PC13ON); // LED shows that recieved data is being printed out
				Serial2.print(r_msg->ID, HEX);
				Serial2.print(" # ");
				Serial2.print(r_msg->Data[0], HEX);
				Serial2.print(" ");
				Serial2.print(r_msg->Data[1], HEX);
				Serial2.print(" ");
				Serial2.print(r_msg->Data[2], HEX);
				Serial2.print(" ");
				Serial2.print(r_msg->Data[3], HEX);
				Serial2.print(" ");
				Serial2.print(r_msg->Data[4], HEX);
				Serial2.print(" ");
				Serial2.print(r_msg->Data[5], HEX);
				Serial2.print(" ");
				Serial2.print(r_msg->Data[6], HEX);
				Serial2.print(" ");
				Serial2.println(r_msg->Data[7], HEX);
				digitalWrite(PC13, PC13OFF);
		#endif
		///// processing the incoming message
		switch(r_msg->ID) 
		{
		case 0x206: // steering wheel buttons
			
			// setting the blocked flag. Button *))
			if (r_msg->Data[1]==0x82) 
			{
				if (r_msg->Data[0]==0x01)	
				{
					blocked = true;
					Serial2.println("Blocking button is pressed");
				}
				else 
				{
					blocked = false;
					Serial2.println("Blocking button is released");
				}
			}
		break;
/**/		
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
					Serial2.println("OK is NOT pressed");
					Serial2.println("Running AC triggering script");
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
					
					
					Serial2.println("Done.");
					digitalWrite(PC13, PC13OFF);
				}
			} 
			else 
			{
				
			} // end if
		break;
/**/		
		case 0x6C8: // 	climate info
		switch (r_msg->Data[0]) // mode? 
		{
			case 0x21: // normal mode, change flow direction
			// r_msg->Data[2] and [7] is direction, 52 - 59 
			switch (r_msg->Data[1])
			{
				case 0xE0:
				if (climate_direction != r_msg->Data[2])
				{
				climate_direction = r_msg->Data[2];
				climChanged = true;
				}
				break;
			}		
			
			break;
			
			case 0x22: // normal mode, change flow speed or temperature
			switch (r_msg->Data[1])	
			{
				case 0x03: // temperature set
				
				if (climate_temperature != r_msg->Data[3])
				{
				climate_temperature = r_msg->Data[3]; 
				//climate_temperature = 10 * (r_msg->Data[3] - 0x30) + r_msg->Data[5] - 0x30; // make here only registering and move calculation to output zone
				climChanged = true;	
				}
				break;

				case 0x50: // fan set. data[3] = data[4] = ascii
				if (climate_fanspeed != r_msg->Data[3])
				{
				climate_fanspeed = r_msg->Data[3]; 
				climChanged = true;	
				}					
				break;
			}			
			break;
			
			case 0x24: // normal mode, auto flow? status
			// 4 is speed, 30 - 37
			// climate_fanspeed = r_msg->Data[3] - 0x30;
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
		} // close switch
//		}
		/* stop read MS CAN */
		canBus.free();
		if (climChanged) 
		{
			Serial2.println("Change of climate control detected, sending new data to odometer");
			digitalWrite(PC13,PC13ON);
			delay(50);
			digitalWrite(PC13,PC13OFF);
			delay(50);
			digitalWrite(PC13,PC13ON);
			delay(50);
			digitalWrite(PC13,PC13OFF);
			
			climShowOnOdometer();
			climChanged= false;
		}
	}
//	canBus.free();
}



void climShowOnOdometer()
{
	
	/**/
	//// here make all low priority processing. Later should be moved to timer interrupt?
	// output fan speed
	// prepare for output
	// clim1 = clim2 = clim3 = 0x00;
	if (climate_temperature > 19)
	{
		clim1=0x20;
		clim1 += climate_temperature - 20;
	}
	else
	{
		clim1=0x10;
		clim1+= climate_temperature - 10;
	}
	Serial2.print("clim1=");
	Serial2.println(clim1,HEX);

	clim3 = climate_fanspeed;
	Serial2.print("clim3=");
	Serial2.println(clim3,HEX);
	

	Serial2.print("fan speed = ");
	if (climate_fanspeed  < 8)
	{
		Serial2.println(climate_fanspeed);
	}
	else 
	{
		Serial2.println("AUTO");		
	}
	
	// 	output temperature
	Serial2.print("tempr setpoint = ");
	Serial2.println(climate_temperature);
	
	// output climate_direction
	Serial2.println("flow directon:");
	switch (climate_direction)
	{ 
		case 0x52: //all:
		Serial2.println("up | mid | down");
		clim2 = 0x77;
		clim3+= 0x70;
		break;
		case 0x53: //up:
		Serial2.println("up | ___ | ____");
		clim2 = 0x70;
		clim3 += 0;
		break;
		case 0x54: //up_middle:
		Serial2.println("up | mid | ____");
		clim2 = 0x77;
		clim3 += 0;
		break;
		case 0x55: //middle:
		Serial2.println("__ | mid | ____");
		clim2 = 0x07;
		clim3 += 0;
		break;
		case 0x56: //middle_down:
		Serial2.println("__ | mid | down");
		clim2 = 0x07;
		clim3 += 0x70;
		break;
		case 0x57: //down:
		Serial2.println("__ | ___ | down");
		clim2 = 0;
		clim3 += 0x70;
		break;
		case 0x58: //up_down:
		Serial2.println("up | ___ | down");
		clim2 = 0x70;
		clim3 += 0x70;
		break;
		case 0x59: //dir_auto:
		Serial2.println("AUTO");
		clim2 = 0x88;
		clim3 += 0x80;
		break;
	}
/**/
	CAN_a_33_Setup();
	delay(50);
	// send result to odometer
	SendCANmessage(0x5E8,8,0x81,climate_temperature,climate_direction,climate_fanspeed);       // send raw data 
	//SendCANmessage(0x5E8,8,0x81,clim1,clim2,clim3);       
	//delay(500);
	//SendCANmessage(0x5E8,8,0x81,clim1,clim2,clim3);       
	canBus.free();
	CAN_b_95_Setup();
/**/
}