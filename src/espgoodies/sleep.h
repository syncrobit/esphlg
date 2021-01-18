
#ifndef _ESPGOODIES_SLEEP_H
#define _ESPGOODIES_SLEEP_H


#include <c_types.h>


#ifdef _DEBUG_SLEEP
#define DEBUG_SLEEP(fmt, ...) DEBUG("[sleep         ] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_SLEEP(...)      {}
#endif

#define SLEEP_WAKE_INTERVAL_MIN    0           /* 0 - sleep disabled */
#define SLEEP_WAKE_INTERVAL_MAX    (7 * 1440)  /* Minutes */
#define SLEEP_WAKE_DURATION_MIN    0           /* Seconds */
#define SLEEP_WAKE_DURATION_MAX    3600        /* Seconds */
#define SLEEP_WAKE_CONNECT_TIMEOUT 20          /* Seconds */

typedef void (*sleep_callback_t)(uint32 interval);


void   ICACHE_FLASH_ATTR sleep_init(sleep_callback_t callback);
void   ICACHE_FLASH_ATTR sleep_reset(void);
bool   ICACHE_FLASH_ATTR sleep_pending(void);
bool   ICACHE_FLASH_ATTR sleep_is_short_wake(void);

uint16 ICACHE_FLASH_ATTR sleep_get_wake_interval(void);
void   ICACHE_FLASH_ATTR sleep_set_wake_interval(uint16 interval);

uint16 ICACHE_FLASH_ATTR sleep_get_wake_duration(void);
void   ICACHE_FLASH_ATTR sleep_set_wake_duration(uint16 duration);


#endif /* _ESPGOODIES_SLEEP_H */
