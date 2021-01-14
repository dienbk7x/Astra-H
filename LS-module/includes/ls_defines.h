#define LS_ID_KEY 0x170

#define LS_KEY_DATA_BYTE 0

// no key
#define KEY_OUT 0x30
// or 0x20

// key in , +0x160#02.00.xx.xx
#define KEY_LOCKED 0x70
// or 0x60
#define KEY_IN 0x70
// unlocked, wake up
#define KEY_IGN_OFF 0x72
#define KEY_UNLOCKED 0x72

#define KEY_IGN 0x04
// ignition just swithed on
#define KEY_IGN_ON 0x74
// ignition is on, 3 seconds passed? need to clarify!
#define KEY_STARTER_OFF 0x54

#define KEY_STARTER_ON 0x76
// 0x56 ??



/*
   0x305, 7, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x81, 0x00, 0x00)
   0x305, 7, null, null, LIGHTS_SWITCH, AAA, BBB, CCC, CENTRAL_LOCK_PRESSED, null)
                              default = 0x00 0x10 0x80
                              default = 0x80 0x10 0x80 жопогрейка?
                             SPORT_ON = 0x__ 0x3A 0x_1
                              ESP_OFF = 0x_8 0x3B 0x_1
                              default = 0x00 0x10 0x80
   enum LIGHTS_SWITCH {
     OFF = 0x00,
     FOG_FRONT = 0x10,
     FOG_FRONT = 0x20,
     GABARIT = 0x40,
     BLIZHIY = 0xC0
   }


   0x305, 7, 0x00, 0x00, 0x00, 0x10, 0x3A, 0x81, 0x00, 0x00)
*/