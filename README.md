# Astra-H by megadrifter (https://github.com/megadrifter)
This project [will] contain several modules based on the STM32F103C8T6 "blue pill" hardware.
Each module has one main CAN interface constantly ON to read and send messages, and one secondary CAN interface that can be switched to for a short time instead of the main, only to send some message to other module.
For example, MS-module should always listen to MS CAN, then it can switch to LS CAN and send message to LS-module, and switch back immediately.
Files are basically for Arduimo IDE, so they are *.ino 

* MS-module should:
- switch compressor on/off at one click of central climate control wheel 
- read state of climate control
- send climate data to LS-module
- be able to go into secret screens

* LS-module should:
- do something with lights?
- close windows when activating alarm
	by emulatinh holding close button for N seconds
- receive climate data from MS-module and show on odometer (keep in memory)
- Make "arrows test" (just for fun)
* Astra H USART-GID:
- Display engine temperature and battery voltage on the display.
- Display of data transmitted in USART2
- High engine temperature alert

As for libraries, I use lib from stm32duino project with some modification of HardwareCAN library.

Maybe I'll manage to create a Class named Astra-H, but not sure, as I have no clue how to do this.

