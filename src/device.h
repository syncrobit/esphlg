
#ifndef _DEVICE_H
#define _DEVICE_H


#ifdef _DEBUG_DEVICE
#define DEBUG_DEVICE(fmt, ...) DEBUG("[device        ] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_DEVICE(...)      {}
#endif


#define DEVICE_NAME_LEN     31
#define DEVICE_PASSWORD_LEN 31

#define DEVICE_FLAG_OTA_AUTO_UPDATE  0x00000001
#define DEVICE_FLAG_OTA_BETA_ENABLED 0x00000002


extern char  *device_name;
extern char  *device_password;
extern uint32 device_flags;


#endif /* _DEVICE_H */
