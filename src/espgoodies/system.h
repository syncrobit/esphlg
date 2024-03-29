
#ifndef _ESPGOODIES_SYSTEM_H
#define _ESPGOODIES_SYSTEM_H


#include <os_type.h>

#include "espgoodies/version.h"


#ifdef _DEBUG_SYSTEM
#define DEBUG_SYSTEM(fmt, ...) DEBUG("[system        ] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_SYSTEM(...)      {}
#endif

#define DEFAULT_HOSTNAME  "esp-%08x"

#define MAX_AVAILABLE_RAM (40 * 1024) /* 40k is an upper limit to the available RAM */


/* Following items go into system flash configuration */

#define SYSTEM_CONFIG_OFFS_SETUP_BUTTON_PIN   0x0000 /*    1 byte */
#define SYSTEM_CONFIG_OFFS_SETUP_BUTTON_LEVEL 0x0001 /*    1 byte */
#define SYSTEM_CONFIG_OFFS_SETUP_BUTTON_HOLD  0x0002 /*    1 byte */
#define SYSTEM_CONFIG_OFFS_SETUP_BUTTON_HOLDR 0x0003 /*    1 byte */

#define SYSTEM_CONFIG_OFFS_STATUS_LED_PIN     0x0004 /*    1 byte */
#define SYSTEM_CONFIG_OFFS_STATUS_LED_LEVEL   0x0005 /*    1 byte */

#define SYSTEM_CONFIG_OFFS_BATTERY_DIV_FACTOR 0x0006 /*    2 bytes */
#define SYSTEM_CONFIG_OFFS_BATTERY_VOLTAGES   0x0008 /*   12 bytes */
                                                     /* 0x0014 - 0x00FF: reserved */
#define SYSTEM_CONFIG_OFFS_FW_VERSION         0x0100 /*    4 bytes */


typedef void (*system_reset_callback_t)(void);
typedef void (*system_setup_mode_callback_t)(bool active, bool external);
typedef void (*system_task_handler_t)(uint32 task_id, void *param);


void   ICACHE_FLASH_ATTR system_config_load(void);
void   ICACHE_FLASH_ATTR system_config_save(void);

void   ICACHE_FLASH_ATTR system_init(void);

void   ICACHE_FLASH_ATTR system_task_set_handler(system_task_handler_t handler);
void   ICACHE_FLASH_ATTR system_task_schedule(uint32 task_id, void *param);

uint32 ICACHE_FLASH_ATTR system_uptime(void);
uint64 ICACHE_FLASH_ATTR system_uptime_ms(void);

/* Needs to be called at least once every hour; must not be in flash, as it's called from ISRs */
uint64                   system_uptime_us(void);

int    ICACHE_FLASH_ATTR system_get_flash_size(void);

void   ICACHE_FLASH_ATTR system_reset(bool delayed);
void   ICACHE_FLASH_ATTR system_reset_set_callbacks(
                             system_reset_callback_t callback,
                             system_reset_callback_t factory_callback
                         );
void   ICACHE_FLASH_ATTR system_reset_callback(bool factory);
void   ICACHE_FLASH_ATTR system_check_reboot_loop(void);

void   ICACHE_FLASH_ATTR system_get_fw_version(version_t *version);
void   ICACHE_FLASH_ATTR system_set_fw_version(version_t *version);

void   ICACHE_FLASH_ATTR system_setup_button_set_config(int8 pin, bool level, uint8 hold, uint8 reset_hold);
void   ICACHE_FLASH_ATTR system_setup_button_get_config(
                             int8 *pin,
                             bool *level,
                             uint8 *hold,
                             uint8 *reset_hold
                         );
void   ICACHE_FLASH_ATTR system_status_led_set_config(int8 pin, bool level);
void   ICACHE_FLASH_ATTR system_status_led_get_config(int8 *pin, bool *level);

void   ICACHE_FLASH_ATTR system_setup_mode_set_callback(system_setup_mode_callback_t callback);
bool   ICACHE_FLASH_ATTR system_setup_mode_active(void);
void   ICACHE_FLASH_ATTR system_setup_mode_toggle(bool external);
bool   ICACHE_FLASH_ATTR system_setup_mode_has_ap_clients(void);


#endif /* _ESPGOODIES_SYSTEM_H */
