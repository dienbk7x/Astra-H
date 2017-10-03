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
  Stat = canBus.begin(CAN_SPEED_33, CAN_MODE_NORMAL);    

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
		// print a copy of sent message
//		Serial1.println("Sending msg:");
		Serial1.print(millis());
		Serial1.print("; ");
		Serial1.print(id, HEX);
		Serial1.print("; #; ");
		
		for (int i=0; i<dlength;++i)
		{
			Serial1.print("; ");
			Serial1.print(msg.Data[i], HEX);
		}
		Serial1.println();
  
//  delay(18);              
//  digitalWrite(PC13, HIGH);   // turn the LED off 
//  delay(10);  

}

#define PC13ON 0
#define PC13OFF 1
#define DELAY 250
/* global variables */

void setup() 
{
	pinMode(PC13, OUTPUT); // LED
	digitalWrite(PC13, PC13ON);
	Serial1.begin(115200);
	Serial1.println("Hello World!");
	Serial1.println("Starting gen program");
	Serial1.println("Time (ms) ; ID ; B0 ; B1 ; B2 ; B3 ; B4 ; B5 ; B6 ; B7 ");
	digitalWrite(PC13, PC13OFF);
	CANSetup();
}

void loop() 
{
	while (1) 
	{
		for (int k=1; k<=0x3ff;++k)
		{
      SendCANmessage(k);
		  delay(250);
    }
	} // close while
} // close void loop
