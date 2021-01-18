
#include <stdlib.h>
#include <string.h>

#include <mem.h>
#include <user_interface.h>
#include <version.h>
#include <spi_flash.h>

#include "espgoodies/common.h"
#include "espgoodies/debug.h"
#include "espgoodies/initdata.h"
#include "espgoodies/ota.h"
#include "espgoodies/rtc.h"
#include "espgoodies/system.h"
#include "espgoodies/wifi.h"

#include "apiserver.h"
#include "common.h"
#include "config.h"
#include "core.h"
#include "device.h"


#define CONNECT_TIMEOUT_SETUP_MODE 300 /* Seconds */
#define SETUP_MODE_IDLE_TIMEOUT    300 /* Seconds */

#define SETUP_BUTTON_PIN           0
#define SETUP_BUTTON_LEVEL         0
#define SETUP_BUTTON_HOLD          5   /* Seconds */
#define SETUP_BUTTON_RESET_HOLD    20  /* Seconds */

#define STATUS_LED_PIN             2
#define STATUS_LED_LEVEL           0

#define WIFI_AUTO_SCAN_INTERVAL    60  /* Seconds */
#define WIFI_MIN_RSSI_THRESHOLD    -80
#define WIFI_BETTER_RSSI_THRESHOLD 15
#define WIFI_BETTER_COUNT          2

#ifndef FW_BASE_URL
#define FW_BASE_URL           ""
#define FW_BASE_OTA_PATH      ""
#endif
#define FW_LATEST_FILE        "/latest"
#define FW_LATEST_STABLE_FILE "/latest_stable"
#define FW_LATEST_BETA_FILE   "/latest_beta"
#define FW_AUTO_MIN_INTERVAL  24 /* Hours */


static bool       wifi_first_time_connected = FALSE;
static os_timer_t wifi_connect_timeout_setup_mode_timer;

static os_timer_t ota_auto_timer;
static uint32     ota_auto_counter = 1;

static os_timer_t setup_mode_idle_timer;


static void ICACHE_FLASH_ATTR main_init(void);

void        ICACHE_FLASH_ATTR user_init(void);
void        ICACHE_FLASH_ATTR user_rf_pre_init(void);
int         ICACHE_FLASH_ATTR user_rf_cal_sector_set(void);

static void ICACHE_FLASH_ATTR on_system_ready(void);
static void ICACHE_FLASH_ATTR on_system_reset(void);

static void ICACHE_FLASH_ATTR on_setup_mode(bool active, bool external);
static void ICACHE_FLASH_ATTR on_setup_mode_idle_timeout(void *);

#ifdef _OTA
static void ICACHE_FLASH_ATTR on_ota_auto_timer(void *arg);
static void ICACHE_FLASH_ATTR on_ota_auto_perform(int code);
#endif

static void ICACHE_FLASH_ATTR on_wifi_connect(bool connected);
static void ICACHE_FLASH_ATTR on_wifi_connect_timeout_setup_mode(void *arg);


void main_init(void) {
    system_init_done_cb(on_system_ready);

    /* Setup button & status LED */
    system_setup_button_set_config(SETUP_BUTTON_PIN, SETUP_BUTTON_LEVEL, SETUP_BUTTON_HOLD, SETUP_BUTTON_RESET_HOLD);
    system_status_led_set_config(STATUS_LED_PIN, STATUS_LED_LEVEL);

    /* Setup mode Wi-Fi timeout */
    os_timer_disarm(&wifi_connect_timeout_setup_mode_timer);
    os_timer_setfn(&wifi_connect_timeout_setup_mode_timer, on_wifi_connect_timeout_setup_mode, NULL);
    os_timer_arm(&wifi_connect_timeout_setup_mode_timer, CONNECT_TIMEOUT_SETUP_MODE * 1000, /* repeat = */ FALSE);

    if (wifi_get_ssid()) {
        wifi_station_enable(
            device_name,
            on_wifi_connect,
            WIFI_AUTO_SCAN_INTERVAL,
            WIFI_MIN_RSSI_THRESHOLD,
            WIFI_BETTER_RSSI_THRESHOLD,
            WIFI_BETTER_COUNT
        );
    }
}

void on_system_ready(void) {
    DEBUG_SYSTEM("system initialization done");

    if (!wifi_get_ssid()) {
        /* If no SSID set (no Wi-Fi network configured), enter setup mode */
        DEBUG_SYSTEM("no SSID configured");
        if (!system_setup_mode_active()) {
            system_setup_mode_toggle(/* external = */ FALSE);
        }
    }
}

void on_system_reset(void) {
    DEBUG_SYSTEM("cleaning up before reset");

    core_stop();
    tcp_server_stop();
}

void on_setup_mode(bool active, bool external) {
    os_timer_disarm(&setup_mode_idle_timer);

    if (active) {
        /* Exit setup mode after an idle timeout */
        os_timer_setfn(&setup_mode_idle_timer, on_setup_mode_idle_timeout, NULL);
        os_timer_arm(&setup_mode_idle_timer, SETUP_MODE_IDLE_TIMEOUT * 1000, /* repeat = */ TRUE);
    }
}

void on_setup_mode_idle_timeout(void *arg) {
    if (!system_setup_mode_active()) {
        return;
    }

    if (wifi_station_is_connected()) {
        return; /* Don't reset if connected to AP */
    }

    if (system_setup_mode_has_ap_clients()) {
        return; /* Don't reset if AP has clients */
    }

    DEBUG_SYSTEM("setup mode idle timeout");
    system_setup_mode_toggle(/* external = */ FALSE);
}

void on_ota_auto_timer(void *arg) {
    if (!(device_flags & DEVICE_FLAG_OTA_AUTO_UPDATE)) {
        return;
    }

    if (system_uptime() > ota_auto_counter * FW_AUTO_MIN_INTERVAL * 3600) {
        ota_auto_counter++;
        ota_auto_update_check(/* beta = */ device_flags & DEVICE_FLAG_OTA_BETA_ENABLED, on_ota_auto_perform);
    }
}

void on_ota_auto_perform(int code) {
}

void on_wifi_connect(bool connected) {
    if (!connected) {
        /* Re-arm the setup mode Wi-Fi timer upon disconnection */
        os_timer_disarm(&wifi_connect_timeout_setup_mode_timer);
        os_timer_setfn(&wifi_connect_timeout_setup_mode_timer, on_wifi_connect_timeout_setup_mode, NULL);
        os_timer_arm(&wifi_connect_timeout_setup_mode_timer, CONNECT_TIMEOUT_SETUP_MODE * 1000, /* repeat = */ FALSE);
        return;
    }

    DEBUG_SYSTEM("we're connected!");

    os_timer_disarm(&wifi_connect_timeout_setup_mode_timer);

    if (!wifi_first_time_connected) {
        wifi_first_time_connected = TRUE;
        core_start();

        /* Start firmware auto update mechanism */
        if (device_flags & DEVICE_FLAG_OTA_AUTO_UPDATE) {
            ota_auto_update_check(/* beta = */ device_flags & DEVICE_FLAG_OTA_BETA_ENABLED, on_ota_auto_perform);
        }
    }
}

void on_wifi_connect_timeout_setup_mode(void *arg) {
    if (!system_setup_mode_active()) {
        DEBUG_SYSTEM("Wi-Fi connection timeout, switching to setup mode");
        system_setup_mode_toggle(/* external = */ FALSE);
    }
}


/* Main functions */

void user_rf_pre_init(void) {
    init_data_ensure();

    system_deep_sleep_set_option(2);  /* No RF calibration after waking from deep sleep */
    system_phy_set_rfoption(2);       /* No RF calibration after waking from deep sleep */
    system_phy_set_powerup_option(2); /* Calibration only for VDD33 and Tx power */
}

int user_rf_cal_sector_set(void) {
    /* Always consider a 1MB flash size; set RF_CAL to the 5th last sector */
    return 256 - 5;
}

void user_init(void) {
    system_timer_reinit();
#ifdef _DEBUG
    debug_uart_setup(_DEBUG_UART_NO);
    os_delay_us(10000);

    printf("\n\n");
    DEBUG_SYSTEM("espHLG      " FW_VERSION);
    DEBUG_SYSTEM("SDK Version " ESP_SDK_VERSION_STRING);
    DEBUG_SYSTEM("Chip ID     %08X", system_get_chip_id());
    DEBUG_SYSTEM("Flash ID    %08X", spi_flash_get_id());

#else /* !_DEBUG */
    debug_uart_setup(DEBUG_UART_DISABLE);
#endif /* _DEBUG */

    system_reset_set_callbacks(on_system_reset, config_factory_reset);
    system_setup_mode_set_callback(on_setup_mode);

    rtc_init();
    system_check_reboot_loop();
    system_config_load();
    config_init();
    system_init();
    ota_init(
        /* current_version = */   FW_VERSION,
        /* latest_url = */        FW_BASE_URL FW_BASE_OTA_PATH FW_LATEST_FILE,
        /* latest_stable_url = */ FW_BASE_URL FW_BASE_OTA_PATH FW_LATEST_STABLE_FILE,
        /* latest_beta_url = */   FW_BASE_URL FW_BASE_OTA_PATH FW_LATEST_BETA_FILE,
        /* url_template = */      FW_BASE_URL FW_BASE_OTA_PATH "/%s"
    );

    os_timer_disarm(&ota_auto_timer);
    os_timer_setfn(&ota_auto_timer, on_ota_auto_timer, NULL);
    os_timer_arm(&ota_auto_timer, 60 * 1000, /* repeat = */ TRUE);

    wifi_init();
    api_server_init();
    core_init();
    main_init();
}
