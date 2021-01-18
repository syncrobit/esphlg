
#ifndef _ESPGOODIES_DEBUG_H
#define _ESPGOODIES_DEBUG_H


#include <c_types.h>


#define DEBUG_BAUD         115200
#define DEBUG_UART_DISABLE -1


void ICACHE_FLASH_ATTR debug_uart_setup(int8 uart_no);
int8 ICACHE_FLASH_ATTR debug_uart_get_no(void);


#endif /* _ESPGOODIES_DEBUG_H */
