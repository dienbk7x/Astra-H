void lsSniffer(){
    UART.print("SNIFFER: type 'exit' to exit");
    delay(3500);
    lsCANSetup();
    canBus.filter(0, 0, 0);

    while(true) {
        if ( ( r_msg = canBus.recv() ) != NULL ) {
            printMsg();
            canBus.free();

        }
        // ======== Receive a message from UART =======================================================================
        if ((millis() - timeUart > 200)) {   //delay needed to fillup buffers
            messageUart = readUart();
            timeUart = millis();
        }
        if ((messageUart != "")) { // recognize and execute command
            UART.println(messageUart);
            if (messageUart=="exit") {
                break;
            }
        }
    }
    lsCANSetup();
}
