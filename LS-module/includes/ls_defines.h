#define LS_ID_KEY 0x170

#define LS_KEY_DATA_BYTE 0x00

// no key
#define KEY_OUT 0x30

// key in , +0x160#02.00.xx.xx
#define KEY_LOCKED 0x70
#define KEY_IN 0x70
// unlocked, wake up
#define KEY_IGN_OFF 0x72
#define KEY_UNLOCKED 0x72

// ignition just swithed on
#define KEY_IGN_ON 0x74
#define KEY_IGN 0x04
// ignition is on, 3 seconds passed? need to clarify!
#define KEY_STARTER_OFF 0x54

#define KEY_STARTER_ON 0x76
// 0x56 ??