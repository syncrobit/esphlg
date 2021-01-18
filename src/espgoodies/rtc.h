
#ifndef _ESPGOODIES_RTC_H
#define _ESPGOODIES_RTC_H

#include <c_types.h>


#ifdef _DEBUG_RTC
#define DEBUG_RTC(fmt, ...) DEBUG("[rtc           ] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_RTC(...)      {}
#endif

#define RTC_USER_ADDR 130 /* 130 * 4 bytes = 520 */


void   ICACHE_FLASH_ATTR rtc_init(void);
void   ICACHE_FLASH_ATTR rtc_reset(void);

bool   ICACHE_FLASH_ATTR rtc_is_full_boot(void);
uint32 ICACHE_FLASH_ATTR rtc_get_boot_count(void);

uint32 ICACHE_FLASH_ATTR rtc_get_value(uint8 addr);
bool   ICACHE_FLASH_ATTR rtc_set_value(uint8 addr, uint32 value);


#endif /* _ESPGOODIES_RTC_H */
