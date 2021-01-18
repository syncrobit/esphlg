
#ifndef _CONFIG_H
#define _CONFIG_H


#include <c_types.h>

#include "espgoodies/json.h"


#ifdef _DEBUG_CONFIG
#define DEBUG_CONFIG(fmt, ...) DEBUG("[config        ] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_CONFIG(...)      {}
#endif

                                           /*   32 bytes - reserved */
#define CONFIG_OFFS_DEVICE_NAME     0x0020 /*   32 bytes */
#define CONFIG_OFFS_DEVICE_PASSWORD 0x0040 /*   32 bytes */
#define CONFIG_OFFS_DEVICE_FLAGS    0x0060 /*    4 bytes */
                                           /*   28 bytes - reserved */
#define CONFIG_OFFS_IP_ADDRESS      0x0080 /*    4 bytes */
#define CONFIG_OFFS_GATEWAY         0x0084 /*    4 bytes */
#define CONFIG_OFFS_DNS             0x0088 /*    4 bytes */
#define CONFIG_OFFS_NETMASK         0x008C /*    1 bytes */


void ICACHE_FLASH_ATTR config_init(void);
void ICACHE_FLASH_ATTR config_save(void);
void ICACHE_FLASH_ATTR config_factory_reset(void);


#endif /* _CONFIG_H */
