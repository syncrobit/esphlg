
/*
 * Inspired from https://github.com/esp8266/Arduino
 */

#ifndef _ESPGOODIES_DNSSERVER_H
#define _ESPGOODIES_DNSSERVER_H


#include <c_types.h>


#ifdef _DEBUG_DNSSERVER
#define DEBUG_DNSSERVER(fmt, ...) DEBUG("[dnsserver     ] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_DNSSERVER(...)      {}
#endif


void ICACHE_FLASH_ATTR dnsserver_start_captive(void);
void ICACHE_FLASH_ATTR dnsserver_stop(void);


#endif /* _ESPGOODIES_DNSSERVER_H */
