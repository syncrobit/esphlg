
#ifndef _ESPGOODIES_OTA_H
#define _ESPGOODIES_OTA_H


#ifdef _DEBUG_OTA
#define DEBUG_OTA(fmt, ...) DEBUG("[ota           ] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_OTA(...)      {}
#endif

#define OTA_STATE_IDLE        0
#define OTA_STATE_CHECKING    1
#define OTA_STATE_DOWNLOADING 2
#define OTA_STATE_RESTARTING  3


/* Version, date and URL must be freed() by callback */
typedef void (*ota_latest_callback_t)(char *version, char *date, char *url);
typedef void (*ota_perform_callback_t)(int code);


void ICACHE_FLASH_ATTR ota_init(
                           char *current_version,
                           char *latest_url,
                           char *latest_stable_url,
                           char *latest_beta_url,
                           char *url_template
                       );

bool ICACHE_FLASH_ATTR ota_get_latest(bool beta, ota_latest_callback_t callback);
bool ICACHE_FLASH_ATTR ota_perform_url(char *url, ota_perform_callback_t callback);
bool ICACHE_FLASH_ATTR ota_perform_version(char *url, ota_perform_callback_t callback);
bool ICACHE_FLASH_ATTR ota_auto_update_check(bool beta, ota_perform_callback_t callback);
bool ICACHE_FLASH_ATTR ota_busy(void);
int  ICACHE_FLASH_ATTR ota_current_state(void);


#endif /* _ESPGOODIES_OTA_H */
