#define STRING_VER 0.1

#include <HardwareCAN.h>
/*
 * This application receives several frames containing various data. It also produces data that are sent periodically using another several frames.
 */

// Define the values of the MS CAN identifiers
#define MS_MY_COMMUNICATION_ID 0x
#define MS_DATE_TIME_ID 0x180
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
	#define MS_BTN_AUX 0x82
	#define MS_BTN_LEFT_WHEEL_TURN 0x83
	#define MS_BTN_LEFT_WHEEL 0x84
	#define MS_BTN_NEXT 0x91
	#define MS_BTN_PREV 0x92
	
#define MS_CLIMA_CONTROLS_ID 0x208
	#define MS_BTN_CLIMA_CENTER_WHEEL 0x17
	#define MS_BTN_CLIMA_CENTER_WHEEL_TURN 0x16
	
#define MS_CLIMA_INFO_ID 0x6C8
#define MS_MEDIA_RDS_ID 0x6C1
#define MS_IGNITION_STATE_ID 0x450
	#define MS_IGNITION_NO_KEY 0x00
	#define MS_IGNITION_KEY_PRESENT 0x04
	#define MS_IGNITION_1 0x05
	#define MS_IGNITION_START 0x06
	
#define MS_SPEED_RPM_ID 0x4E8
	#define MS_BACKWARDS_BIT 0x04
	
#define MS_WAKEUP_ID 0x697
#define MS__ID 0x00
#define MS__ID 0x00
#define MS__ID 0x00
// MS bytes for different buttons
	// see under IDs
#define MS_BTN_ 0x00
#define MS_BTN_ 0x00
#define MS_BTN_ 0x00


// Define the values of the LS CAN identifiers
#define LS_MY_COMMUNICATION_ID 0x
#define LS_ODOMETER_ECN_ID 0x5E8
#define LS_SPEED_RPM_ARROWS_ID 0x108

#define LS__ID 0x00

// Define buttons state bytes
#define BTN_PRESSED 0x01
#define BTN_RELEASED 0x00
#define WHEEL_PRESSED 0x08
#define CLIMA_WHEEL_TURN 0x08
#define WHEEL_TURN_DOWN 0xFF
#define WHEEL_TURN_ 0x01



// Limit time to flag a CAN error
#define CAN_TIMEOUT 100           
#define CAN_DELAY 0 // 10        // ms between two processings of incoming messages
#define CAN_SEND_RATE 10 // 200   // ms between two successive sendings

// Message structures. Each message has its own identifier. As many such variables should be defined 
// as the number of different CAN frames the application has to send. 
CanMsg msg_LS_ODOMETER_ECN ;
CanMsg msg_LS_MY_COMMUNICATION ;
CanMsg msg_LS_SPEED_RPM_ARROWS ;
CanMsg msg_MS_CLIMA_CONTROLS ;
CanMsg msg_MS_MEDIA_BUTTONS ;
CanMsg msg_MS_MEDIA_RDS ;
CanMsg msg_MS_WAKEUP ;
// CanMsg msg ;
CanMsg msgMS_CIM_warning ;


// Traffic handling data
int CANquietTime ;          // Quiet time counter to detect no activity on CAN bus
bool CANError ;             // Indicates that incoming CAN traffic is missing
int CANsendDivider ;        // Used to send frames once every so many times loop() is called

///// Definitions and global vars /////
// #define DEBUGMODE
#define T_DELAY 0 //10

#define PC13ON 0
#define PC13OFF 1
#define DELAY 250
/* global variables */
/// flags:
volatile bool flag_blocked;
volatile bool flag_usbMode;
volatile bool flag_climChanged;
/* climate control */
volatile uint8 climate_temperature=0;
//volatile enum { } climate_temperature;
volatile uint8 climate_fanspeed=0;
//volatile enum { all=0x52, up,up_middle, middle, middle_down, down, up_down, dir_auto} climate_direction;
volatile uint8 climate_direction=0;
volatile uint8 clim1;
volatile uint8 clim2;
volatile uint8 clim3;
// number buttons listener
// byte buttonPressed[];
// byte buttonPressedIndex=0;
// Instanciation of CAN interface

HardwareCAN canBus(CAN1_BASE);
CanMsg msg ;

///// basic functions for CAN  /////
///
void CAN_a_33_Setup(void)
{	// start CAN_a_33_Setup
  Serial2.println("canBus Initialization: LS CAN");
  CAN_STATUS Stat ;
  afio_init(); // this will restart subsystem and make it work!
  canBus.map(CAN_GPIO_PA11_PA12);  
  Stat = canBus.begin(CAN_SPEED_33, CAN_MODE_NORMAL);
  canBus.filter(0, 0, 0);
  canBus.set_irq_mode();
  Stat = canBus.status();
  if (Stat != CAN_OK) 	{
						digitalWrite(PC13, LOW);
						Serial2.println("canBus Initialization failed for LS CAN");
						delay(1000);
						}
						else 
						{
						Serial2.println("LS CAN = OK");
						}
 }
	// end CAN_a_33_Setup
void LS_CAN(void) { CAN_a_33_Setup(); }	
	
void CAN_b_95_Setup(void)
{	// start CAN_b_95_Setup
  Serial2.println("canBus Initialization: MS CAN");
  CAN_STATUS Stat ;
  canBus.map(CAN_GPIO_PB8_PB9);
  Stat = canBus.begin(CAN_SPEED_95, CAN_MODE_NORMAL);
  canBus.filter(0, 0, 0);
  canBus.set_irq_mode();
  Stat = canBus.status();
  if (Stat != CAN_OK)  	{
						digitalWrite(PC13, LOW);
						Serial2.println("canBus Initialization failed for MS CAN");
						delay(1000);
						}
						else 
						{
						Serial2.println("MS CAN = OK");
						}
}
	// end CAN_b_95_Setup
void MS_CAN(void) { CAN_b_95_Setup(); }	
	
void CANmsgSetup(void)
{	// start CANmsgSetup
/*
  // Initialize the message structures
  // A CAN structure includes the following fields:
  msgGyroscope.IDE = CAN_ID_STD;          // Indicates a standard identifier ; CAN_ID_EXT would mean this frame uses an extended identifier
  msgGyroscope.RTR = CAN_RTR_DATA;        // Indicated this is a data frame, as opposed to a remote frame (would then be CAN_RTR_REMOTE)
  msgGyroscope.ID = GYRO_ID ;             // Identifier of the frame : 0-2047 (0-0x3ff) for standard idenfiers; 0-0x1fffffff for extended identifiers
  msgGyroscope.DLC = 3;                   // Number of data bytes to follow
  msgGyroscope.Data[0] = 0x0;             // Data bytes, there can be 0 to 8 bytes.
  msgGyroscope.Data[1] = 0x0;
  msgGyroscope.Data[2] = 0x0;
  
  msgMotorControl.IDE = CAN_ID_STD;
  msgMotorControl.RTR = CAN_RTR_DATA;
  msgMotorControl.ID = MOTOR_CONTROL_ID ;
  msgMotorControl.DLC = 2;
  msgMotorControl.Data[0] = 0x0;
  msgMotorControl.Data[1] = 0x0;
  */
}
	// end CANmsgSetup
	
// Send one frame. Parameter is a pointer to a frame structure (above), that has previously been updated with data.
// If no mailbox is available, wait until one becomes empty. There are 3 mailboxes.
CAN_TX_MBX CANsend(CanMsg *pmsg)
{	// start CAN_TX_MBX CANsend
  CAN_TX_MBX mbx;

  do 
  {
    mbx = canBus.send(pmsg) ;
#ifdef USE_MULTITASK
    vTaskDelay( 1 ) ;                 // Infinite loops are not multitasking-friendly
#endif
  }
  while(mbx == CAN_TX_NO_MBX) ;       // Waiting outbound frames will eventually be sent, unless there is a CAN bus failure.
   Serial2.println("CANsend OK");
  return mbx ;
}
	// end CAN_TX_MBX CANsend

void SendCANmessage( // general function to send any message
					long id=0x001, 
					byte dlength=8, 
					byte d0=0x00, byte d1=0x00, byte d2=0x00, byte d3=0x00, byte d4=0x00, byte d5=0x00, byte d6=0x00, byte d7=0x00
					)
{	// start SendCANmessage
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
	// print a copy of sent message
	Serial2.println("Sending msg:");
	Serial2.print(id, HEX);
	Serial2.print(" #");
	
	for (int i=0; i<dlength;++i)
	{
		Serial2.print(" ");
		Serial2.print(msg.Data[i], HEX);
	}
	Serial2.println();

	CANsend(&msg) ;      // Send this frame            
	digitalWrite(PC13, HIGH);   // turn the LED off 

}
	// end SendCANmessage
	
void SendMessages()
{
/// list here everything to be sent
Serial2.println("Sending all messages");
LS_CAN();
SendCANmessage(LS_ODOMETER_ECN_ID,8,0x81,0x00,0x00,0x00); 
if (flag_climChanged)
	{
	SendCANmessage(LS_ODOMETER_ECN_ID,8,0x81,climate_direction,climate_temperature,climate_fanspeed); 
	}
MS_CAN();

}	
	

	
// Process incoming messages
// Note : frames are not fully checked for correctness: DLC value is not checked, neither are the IDE and RTR fields. However, the data is guaranteed to be corrrect.
void ProcessMessages(void) // all main code is here!
{	// start ProcessMessages
Serial2.println("Start processing incoming messages");
Serial2.println("Waiting for CAN message...");

  int i ;
  
  CanMsg *r_msg;

  // Loop for every message in the fifo
	int cntr=100; // counter to avoid infinite loop
  while ( ( (r_msg = canBus.recv()) != NULL ) and (cntr>0) )
  {	//	start while ((r_msg = canBus.recv()) != NULL)
    cntr--;
	CANquietTime = 0 ;              // Reset at each received frame
    CANError = false ;              // Clear CAN silence error
	//delay(50);
	#ifdef DEBUGMODE	
		Serial2.println("reading and printing the message if DEBUGMODE is on");
		// make separate function printmsg();
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
	///// processing the incoming message /////

    switch ( r_msg->ID )
    {	//	start switch ( r_msg->ID )
		case 0x206: //MS_WHEEL_BUTTONS_ID: // steering wheel buttons
			Serial2.println("steering wheel button detected");
			switch (r_msg->Data[1])
			// setting the flag_blocked flag. Button *))
			{
				case 0x82: //MS_BTN_AUX: 	// "aux/hands free" button
					if (r_msg->Data[0]==BTN_PRESSED)
					{
						flag_blocked = true;
						Serial2.println("Blocking button is pressed");
					}
					else 
					{
						flag_blocked = false;
						Serial2.println("Blocking button is released");
					}
				break;
				
				//default:
				//break;
			}
		Serial2.println("end of steering wheel button prosessing");
		break;
	/**/		
		case MS_CLIMA_CONTROLS_ID: //climate controls
			Serial2.println("clima controls detected");
			switch (r_msg->Data[1])
			{
				case 0x17: //MS_BTN_CLIMA_CENTER_WHEEL:
					// check "flag_blocked" button pressed
					Serial2.println("MS_BTN_CLIMA_CENTER_WHEEL detected");
					if ( // check if the climate control menu is pressed together with blocking button
						(flag_blocked == false) and
						(r_msg->Data[0]==BTN_PRESSED) and 
						(r_msg->Data[2]==0x00) // do I need it?
					   )
					{   // Start AC triggering script
						Serial2.println("Blocking is NOT pressed");
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
						// end   AC triggering script
					 
					// end if (flag_blocked == false)
					else 
					{
						Serial2.println("Blocking IS pressed");
						Serial2.println("Skipping AC triggering script");
					} 
					// end else if (flag_blocked == false)
				break;
				
				//default:
				//break;
			}
		Serial2.println("end of clima controls prosessing");	
		break;
	/**/		
		case MS_CLIMA_INFO_ID: // 	climate info
			Serial2.println("climate info detected");
			switch (r_msg->Data[0]) // mode? 
			{
				case 0x21: // normal mode, change flow direction
					Serial2.println("normal mode, change flow direction");
					// r_msg->Data[2] and [7] is direction, 52 - 59 
					switch (r_msg->Data[1]) /// why ?
					{
						case 0xE0:
							if (climate_direction != r_msg->Data[2])
							{
								Serial2.println("readest new direction");	
								climate_direction = r_msg->Data[2];
								flag_climChanged = true;
							}
							// end if
						break;
						
						//default:
						//break;
					}		
					// end switch (r_msg->Data[1])
				break;
				
				case 0x22: // normal mode, change flow speed or temperature
					Serial2.println("normal mode, change flow speed or temperature");
					switch (r_msg->Data[1])	
					{
						case 0x03: // temperature set
							Serial2.println("temperature set");
							if (climate_temperature != r_msg->Data[3])
							{
								climate_temperature = r_msg->Data[3]; // temporary, only high digit
								Serial2.println(climate_temperature, HEX);
								//climate_temperature = 10 * (r_msg->Data[3] - 0x30) + r_msg->Data[5] - 0x30; // make here only registering and move calculation to output zone
								flag_climChanged = true;	
							}
						break;

						case 0x50: // fan set. data[3] = data[4] = ascii
							Serial2.println("fan set");
							if (climate_fanspeed != r_msg->Data[3])
							{
								climate_fanspeed = r_msg->Data[3]; 
								Serial2.println("climate_fanspeed");
								flag_climChanged = true;	
							}					
						break;
						
						//default:
						//break;
					}			
				break;
				
				case 0x24: // normal mode, auto flow? status
					Serial2.println("normal mode, auto flow? Speed:");
					// 4 is speed, 30 - 37
					climate_fanspeed = r_msg->Data[3]; // - 0x30;
					Serial2.println(climate_fanspeed, HEX);
				break;
				
				case 0x25: // normal mode, auto flow speed, status
					Serial2.println("normal mode, auto flow speed, status");
					// 4 is E0 = full auto speed, 41 = manual flow direction
					Serial2.println(r_msg->Data[3], HEX);
				break;
				
				case 0x26: // air distribution mode,
					Serial2.println("air distribution mode");
					// [7] is flow direction , 52 - 59
					climate_direction = r_msg->Data[7];
					Serial2.println(climate_direction, HEX);
				break;
					
				//default:
				//break;
			} 
			// end switch (r_msg->Data[0]) 
			Serial2.println("End of climate info processing");
		break; // case 0x6C8: // ID = climate info
			
		/*default :  // Any frame with a different identifier is ignored  /// hidden because of error!!!!!
		Serial2.println("reached default switch for ID");
		break ;*/
    }
		//	end   switch ( r_msg->ID )
	Serial2.println("end of message processing. Clear input buffer...");		
    canBus.free();                          // Remove processed message from buffer, whatever the identifier
#ifdef USE_MULTITASK
    vTaskDelay( 1 ) ;                       // Infinite loops are not multitasking-friendly
#endif
  }
	//	 end  while ((r_msg = canBus.recv()) != NULL)
}
	// end ProcessMessages

// Send messages
// Prepare and send 2 frames containing the value of process variables
// Sending all frames at once is a choice; they could be sent separately, at different times and rates.
/*
void SendCANmsg_XXX(void)	// dedicated function to send specific message
{	// start SendCANmsg_XXX
  // Prepare XXX frame 
  msg_XXX.Data[0] = 0x00 ;
  msg_XXX.Data[0] = 0x00 ;
  msg_XXX.Data[0] = 0x00 ;
  msg_XXX.Data[0] = 0x00 ;
  msg_XXX.Data[0] = 0x00 ;
  msg_XXX.Data[0] = 0x00 ;
  msg_XXX.Data[0] = 0x00 ;
  msg_XXX.Data[0] = 0x00 ;
  CANsend(&msg_XXX) ;      // Send this frame
  
}
	// end SendCANmsg_XXX
*/
	
void checkUSBmode()
{	//	start checkUSBmode()
	Serial2.println("pin B12 set to input mode");
	pinMode(28, INPUT); // B12 = 16+12 = 28
	flag_usbMode = digitalRead(28); // if B12 is HIGH, then switch to programming mode
	Serial2.print("pin B12 is ");
    Serial2.println(flag_usbMode);

	if (flag_usbMode)
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
}	
	//	end   checkUSBmode()
	
void blink(int times)
{// use to debug
  for (int i=0;i<times;++i)
  {
      digitalWrite(PC13, LOW);    // turn the onboard LED on
      delay(50);
      digitalWrite(PC13, HIGH);   // turn the LED off 
      delay(100);
  } 
}
// end blink(int times)

///////************************************************///////
///////************************************************///////
// The application program starts here
void setup() {
  // put your setup code here, to run once:
  CANmsgSetup() ;        // prepare the message structures.

  	Serial2.begin(115200); // USART2 on A2-A3 pins
	Serial2.println("Hello World!");
	Serial2.println("Starting \"AstraH MS Module\" v STRING_VER program");
	pinMode(PC13, OUTPUT); // LED
	digitalWrite(PC13, PC13ON);
	
	checkUSBmode(); 
	
	// small self-presentation in MS CAN
	Serial2.println("setting block flag to 0");
	flag_blocked=false;
	
	MS_CAN();
	
	Serial2.println("Sending something to CAN");
	SendCANmessage(0x208, 0); // dummy
	Serial2.println("Sending wakeupscreen to CAN");
	SendCANmessage(0x697, 8, 0x47, 0x00, 0x60, 0x00, 0x02, 0x00, 0x00, 0x80); // wake up screen
	Serial2.println("Press SETTINGS two times...");
	
	SendCANmessage(MS_MEDIA_BUTTONS_ID,3,BTN_PRESSED,MS_BTN_SETTINGS,0x00); // press settings      
	delay(50);
	SendCANmessage(MS_MEDIA_BUTTONS_ID,3,BTN_RELEASED,MS_BTN_SETTINGS,0x00); // release settings      
	delay(350);	
	SendCANmessage(MS_MEDIA_BUTTONS_ID,3,BTN_PRESSED,MS_BTN_SETTINGS,0x00); // press settings      
	delay(50);
	SendCANmessage(MS_MEDIA_BUTTONS_ID,3,BTN_RELEASED,MS_BTN_SETTINGS,0x00); // release settings      
	delay(50);
    Serial2.println(" .. finished");

	  // small self-presentation in LS CAN
	LS_CAN();
	Serial2.println("Countdoun on the ODO screen...");	
	SendCANmessage(LS_ODOMETER_ECN_ID,8,0x81,0x00,0x00,0x00);       
	delay(200);
	SendCANmessage(LS_ODOMETER_ECN_ID,8,0x81,0x11,0x11,0x11);       
	delay(200);
	SendCANmessage(LS_ODOMETER_ECN_ID,8,0x81,0x22,0x22,0x22);       
	delay(200);
	SendCANmessage(LS_ODOMETER_ECN_ID,8,0x81,0x33,0x33,0x33);       
	delay(200);
	SendCANmessage(LS_ODOMETER_ECN_ID,8,0x81,0x22,0x22,0x22);       
	delay(200);
	SendCANmessage(LS_ODOMETER_ECN_ID,8,0x81,0x11,0x11,0x11);       
	delay(200);
	SendCANmessage(LS_ODOMETER_ECN_ID,8,0x81,0x00,0x00,0x00);       
	canBus.free();
    Serial2.println(" .. finished");
	
	digitalWrite(PC13, PC13OFF);
	clim1 = clim2 = clim3 = 0x00;
	flag_climChanged = false;
	

    Serial2.println("Setup finished, going to loop()");
  }

void loop() {
Serial2.println("Start of loop()");

  // Process incoming messages periodically (should be often enough to avoid overflowing the fifo)
  Serial2.println("Starting ProcessMessages()...");
  MS_CAN();
  ProcessMessages() ;          // Process all incoming messages, update local variables accordingly
  Serial2.println("Finished ProcessMessages()");

  // This is an example of timeout management. Here it is global to all received frames; 
  // it could be on a frame by frame basis, with as many control variables as the number of frames.
  CANquietTime++ ; 
  if ( CANquietTime > CAN_TIMEOUT )
  {
    CANquietTime = CAN_TIMEOUT + 1 ;            // To prevent overflowing this variable if silence prolongs...
    CANError = true ;                           // Flag CAN silence error. Will be cleared at first frame received
	Serial2.println("Flag CAN silence error");
  }

  // Send messages containing variables to publish. Sent less frequently than the processing of incoming frames (here, every 200 ms)
  CANsendDivider-- ;
  if ( CANsendDivider < 0 )
  {
    CANsendDivider = CAN_SEND_RATE / CAN_DELAY ;
	Serial2.println("Starting SendMessages()...");
    SendMessages() ;  ////////////////////////////////////////// Check here
	Serial2.println("Finished SendMessages()");
	
  }
  delay(CAN_DELAY) ;    // The delay must not be greater than the time to overflow the incoming fifo (here about 15 ms)
}


