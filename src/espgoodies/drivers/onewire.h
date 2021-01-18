
/*
 * Inspired from https://github.com/PaulStoffregen/OneWire/
 */

#ifndef _ESPGOODIES_ONE_WIRE_H
#define _ESPGOODIES_ONE_WIRE_H


#include <c_types.h>


#define ONE_WIRE_CMD_SEARCH_ROM      0xF0
#define ONE_WIRE_CMD_SKIP_ROM        0xCC
#define ONE_WIRE_CMD_CONVERT_T       0x44
#define ONE_WIRE_CMD_READ_SCRATCHPAD 0xBE


typedef struct {

    uint8 pin_no;
    uint8 rom[8];
    uint8 last_discrepancy;
    uint8 last_family_discrepancy;
    bool  last_device_flag;

} one_wire_t;


void  ICACHE_FLASH_ATTR one_wire_setup(one_wire_t *one_wire);
bool  ICACHE_FLASH_ATTR one_wire_reset(one_wire_t *one_wire);

void  ICACHE_FLASH_ATTR one_wire_search_reset(one_wire_t *one_wire);
bool  ICACHE_FLASH_ATTR one_wire_search(one_wire_t *one_wire, uint8 *addr);

uint8 ICACHE_FLASH_ATTR one_wire_read(one_wire_t *one_wire);
void  ICACHE_FLASH_ATTR one_wire_write(one_wire_t *one_wire, uint8 value, bool parasitic);
void  ICACHE_FLASH_ATTR one_wire_write_bytes(one_wire_t *one_wire, uint8 *buf, uint16 len, bool parasitic);

void  ICACHE_FLASH_ATTR one_wire_parasitic_power_off(one_wire_t *one_wire);

uint8 ICACHE_FLASH_ATTR one_wire_crc8(uint8 *buf, int len);


#endif /* _ESPGOODIES_ONE_WIRE_H */
