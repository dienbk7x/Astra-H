//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                                  ASTRA H DEFINITIONS                                //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
// Define the values of the MS CAN identifiers
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
    #define MS_BTN_LEFT 0x6D
    #define MS_BTN_CD_RADIO 0xE0
    #define MS_BTN_SETTINGS 0xFF
// Define buttons state bytes
    #define BTN_PRESSED 0x01
    #define BTN_RELEASED 0x00
    #define WHEEL_PRESSED 0x08
    #define WHEEL_TURN_DOWN 0xFF
    #define WHEEL_TURN_UP 0x01

#define MS_WHEEL_BUTTONS_ID 0x206
    #define MS_BTN_STATION 0x81
    #define MS_BTN_MODE 0x82
    #define MS_BTN_LEFT_WHEEL_TURN 0x83
    #define MS_BTN_LEFT_WHEEL 0x84
    #define MS_BTN_NEXT 0x91
    #define MS_BTN_PREV 0x92
    #define MS_BTN_VOL 0x93

#define MS_MEDIA_ID 0x6C1
#define MS_CLIMATE_INFO_ID 0x6C8

#define MS_CLIMATE_CONTROLS_ID 0x208
    #define BIG_ENCODER_PRESS 0x17
        #define BIG_ENCODER_PRESSED 0x01
        #define BIG_ENCODER_RELEASED 0x00
/*          (can_msg->Data[0] == 0x01) and
            (can_msg->Data[1] == 0x17) and
            (can_msg->Data[2] == 0x00) //?

              log("turn right 1 click");
              SendCANmessage(0x208, 6, 0x08, 0x16, 0x01);
              delay(DELAY);
              // press
              log("press");
              SendCANmessage(0x208, 6, 0x01, 0x17, 0x00);
              delay(70);
              SendCANmessage(0x208, 6, 0x00, 0x17, 0x00);
              delay(DELAY);

              // turn left 1 click
              log("turn left 1 click");
              SendCANmessage(0x208, 6, 0x08, 0x16, 0xff);   */
    #define BIG_ENCODER_TURN 0x16
        #define BIG_ENCODER_CLOCKWISE 0x01
        #define BIG_ENCODER_COUNTER_CLOCKWISE 0xff


#define MS_ECC_ID 0x548
    #define MS_BATTERY 0x07
    #define MS_ENGINE_TEMP 0x10
    #define MS_RPM_SPEED 0x10
    #define MS_IGNITION_STATE_ID 0x450
    #define MS_IGNITION_NO_KEY 0x00
    #define MS_IGNITION_KEY_PRESENT 0x04
    #define MS_IGNITION_1 0x05
    #define MS_IGNITION_START 0x06
    #define MS_SPEED_RPM_ID 0x4E8
    #define MS_RANGE_ID 0x4EE
    #define MS_BACKWARDS_BIT 0x04

#define MS_CLEAR_DISPLAY 0x0691 // todo ?

#define MS_WAKEUP_ID 0x697
#define MS_TIME_CLOCK_ID 0x180
#define MS_WINDOW_ID 0x2B0
#define MS_TEMP_OUT_DOOR_ID 0x683

