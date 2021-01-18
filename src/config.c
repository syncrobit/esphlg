
#include <mem.h>

#include "espgoodies/common.h"
#include "espgoodies/flashcfg.h"
#include "espgoodies/wifi.h"

#include "common.h"
#include "device.h"
#include "config.h"


void config_init(void) {
    uint8 *config_data = zalloc(FLASH_CONFIG_SIZE_DEFAULT);
    flashcfg_load(FLASH_CONFIG_SLOT_DEFAULT, config_data);

    /* If config data is full of 0xFF, that usually indicates erased flash. Fill it with 0s in that case, which is what
     * we use for default config. */
    uint16 i;
    bool erased = TRUE;
    for (i = 0; i < 32; i++) {
        if (config_data[i] != 0xFF) {
            erased = FALSE;
            break;
        }
    }

    if (erased) {
        DEBUG_CONFIG("detected erased flash config");
        memset(config_data, 0, FLASH_CONFIG_SIZE_DEFAULT);
        flashcfg_save(FLASH_CONFIG_SLOT_DEFAULT, config_data);
    }

    /* Device name */
    free(device_name);
    device_name = strndup((char *) config_data + CONFIG_OFFS_DEVICE_NAME, DEVICE_NAME_LEN + 1);
    if (!device_name[0]) {
        /* Use default device name */
        char hostname[DEVICE_NAME_LEN];
        snprintf(hostname, sizeof(hostname), DEFAULT_HOSTNAME, system_get_chip_id());
        free(device_name);
        device_name = strdup(hostname);
    }

    /* Password */
    free(device_password);
    device_password = strndup((char *) config_data + CONFIG_OFFS_DEVICE_PASSWORD, DEVICE_PASSWORD_LEN + 1);

    /* Device flags */
    memcpy(&device_flags, config_data + CONFIG_OFFS_DEVICE_FLAGS, 4);

    /* IP configuration */
    ip_addr_t wifi_ip, wifi_gw, wifi_dns;
    uint8 wifi_netmask;

    memcpy(&wifi_ip.addr, config_data + CONFIG_OFFS_IP_ADDRESS, 4);
    memcpy(&wifi_gw.addr, config_data + CONFIG_OFFS_GATEWAY, 4);
    memcpy(&wifi_dns.addr, config_data + CONFIG_OFFS_DNS, 4);
    memcpy(&wifi_netmask, config_data + CONFIG_OFFS_NETMASK, 1);

    wifi_set_ip_address(wifi_ip);
    wifi_set_netmask(wifi_netmask);
    wifi_set_gateway(wifi_gw);
    wifi_set_dns(wifi_dns);

    /* At this point we no longer need the config data */
    free(config_data);
}

void config_save(void) {
    uint8 *config_data = zalloc(FLASH_CONFIG_SIZE_DEFAULT);

    flashcfg_load(FLASH_CONFIG_SLOT_DEFAULT, config_data);

    /* Device name */
    memcpy(config_data + CONFIG_OFFS_DEVICE_NAME, device_name, DEVICE_NAME_LEN + 1);

    /* Device password */
    memcpy(config_data + CONFIG_OFFS_DEVICE_PASSWORD, device_password, DEVICE_PASSWORD_LEN + 1);

    /* Device flags */
    memcpy(config_data + CONFIG_OFFS_DEVICE_FLAGS, &device_flags, 4);

    /* IP configuration */
    ip_addr_t wifi_ip_address, wifi_gateway, wifi_dns;
    uint8 wifi_netmask;

    wifi_ip_address = wifi_get_ip_address();
    wifi_netmask = wifi_get_netmask();
    wifi_gateway = wifi_get_gateway();
    wifi_dns = wifi_get_dns();

    memcpy(config_data + CONFIG_OFFS_IP_ADDRESS, &wifi_ip_address, 4);
    memcpy(config_data + CONFIG_OFFS_GATEWAY, &wifi_gateway, 4);
    memcpy(config_data + CONFIG_OFFS_DNS, &wifi_dns, 4);
    memcpy(config_data + CONFIG_OFFS_NETMASK, &wifi_netmask, 1);

    flashcfg_save(FLASH_CONFIG_SLOT_DEFAULT, config_data);
    free(config_data);
}

void config_factory_reset(void) {
    uint8 *config_data = zalloc(FLASH_CONFIG_SIZE_DEFAULT);

    flashcfg_save(FLASH_CONFIG_SLOT_DEFAULT, config_data);
    free(config_data);
}
