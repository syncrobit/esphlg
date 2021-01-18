
#ifndef _CORE_H
#define _CORE_H


#include <c_types.h>


#ifdef _DEBUG_CORE
#define DEBUG_CORE(fmt, ...) DEBUG("[core          ] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_CORE(...)      {}
#endif


void ICACHE_FLASH_ATTR core_init(void);

void ICACHE_FLASH_ATTR core_start(void);
void ICACHE_FLASH_ATTR core_stop(void);
void ICACHE_FLASH_ATTR core_update(void);


#endif /* _CORE_H */
