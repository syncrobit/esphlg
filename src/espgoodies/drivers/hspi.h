
/*
 * Inspired from https://github.com/esp8266/Arduino/blob/master/libraries/SPI/SPI.h
 */

#ifndef _ESPGOODIES_HSPI_H
#define _ESPGOODIES_HSPI_H


#include <c_types.h>


#define HSPI_BIT_ORDER_MSB_FIRST 0
#define HSPI_BIT_ORDER_LSB_FIRST 1

#ifdef _DEBUG_HSPI
#define DEBUG_HSPI(fmt, ...) DEBUG("[hspi          ] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_HSPI(...)      {}
#endif


void  ICACHE_FLASH_ATTR hspi_setup(uint8 bit_order, bool cpol, bool cpha, uint32 freq);
bool  ICACHE_FLASH_ATTR hspi_get_current_setup(uint8 *bit_order, bool *cpol, bool *cpha, uint32 *freq);
void  ICACHE_FLASH_ATTR hspi_transfer(uint8 *out_buff, uint8 *in_buff, uint32 len);
uint8 ICACHE_FLASH_ATTR hspi_transfer_byte(uint8 byte);


#endif /* _ESPGOODIES_HSPI_H */
