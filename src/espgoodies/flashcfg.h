
#ifndef _ESPGOODIES_FLASHCFG_H
#define _ESPGOODIES_FLASHCFG_H


#ifdef _DEBUG_FLASHCFG
#define DEBUG_FLASHCFG(fmt, ...) DEBUG("[flashcfg      ] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_FLASHCFG(...)      {}
#endif

#define FLASH_CONFIG_SLOT_DEFAULT 0 /* Slot for regular configuration */
#define FLASH_CONFIG_SLOT_SYSTEM  1 /* Slot for system configuration */

#define FLASH_CONFIG_SIZE_DEFAULT 0x2000 /*  8k bytes */
#define FLASH_CONFIG_SIZE_SYSTEM  0x1000 /*  4k bytes */

#define FLASH_CONFIG_OFFS_DEFAULT 0x0000
#define FLASH_CONFIG_OFFS_SYSTEM  0x2000


bool ICACHE_FLASH_ATTR flashcfg_load(uint8 slot, uint8 *data);
bool ICACHE_FLASH_ATTR flashcfg_save(uint8 slot, uint8 *data);
bool ICACHE_FLASH_ATTR flashcfg_reset(uint8 slot);


#endif /* _ESPGOODIES_FLASHCFG_H */
