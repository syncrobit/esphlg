
#include <stdlib.h>
#include <ctype.h>

#include <mem.h>
#include <user_interface.h>

#include "espgoodies/common.h"
#include "espgoodies/crypto.h"
#include "espgoodies/httpserver.h"
#include "espgoodies/json.h"
#include "espgoodies/ota.h"
#include "espgoodies/utils.h"
#include "espgoodies/wifi.h"

#include "apiserver.h"
#include "common.h"
#include "config.h"
#include "device.h"
#include "api.h"


#define API_ERROR(response_json, c, error) ({     \
    *code = c;                                    \
    _api_error(response_json, error, NULL, NULL); \
})

#define MISSING_FIELD(response_json, field) ({                  \
    *code = 400;                                                \
    _api_error(response_json, "missing-field", "field", field); \
})

#define INVALID_FIELD(response_json, field) ({                  \
    *code = 400;                                                \
    _api_error(response_json, "invalid-field", "field", field); \
})

#define NO_SUCH_ATTR(response_json, attr) ({                           \
    *code = 400;                                                       \
    _api_error(response_json, "no-such-attribute", "attribute", attr); \
})

#define RESPOND_NO_SUCH_FUNCTION(response_json) {                      \
    response_json = API_ERROR(response_json, 404, "no-such-function"); \
    goto response;                                                     \
}

#define WIFI_RSSI_EXCELLENT -55
#define WIFI_RSSI_GOOD      -65
#define WIFI_RSSI_FAIR      -75


static struct espconn *api_conn;


static char *ota_states_str[] = {"idle", "checking", "downloading", "restarting"};

static json_t ICACHE_FLASH_ATTR *_api_error(json_t *response_json, char *error, char *field_name, char *field_value);

static void   ICACHE_FLASH_ATTR  on_ota_latest(char *version, char *date, char *url);
static void   ICACHE_FLASH_ATTR  on_ota_perform(int code);

static void   ICACHE_FLASH_ATTR  on_wifi_scan(wifi_scan_result_t *results, int len);

static bool   ICACHE_FLASH_ATTR  validate_ip_address(char *ip, uint8 *a);
static bool   ICACHE_FLASH_ATTR  validate_wifi_ssid(char *ssid);
static bool   ICACHE_FLASH_ATTR  validate_wifi_key(char *key);
static bool   ICACHE_FLASH_ATTR  validate_wifi_bssid(char *bssid_str, uint8 *bssid);


json_t *api_call_handle(int method, char* path, json_t *query_json, json_t *request_json, int *code) {
    char *part1 = NULL, *part2 = NULL, *part3 = NULL;
    char *token;
    *code = 200;

    json_t *response_json = NULL;

    if (method == HTTP_METHOD_OTHER) {
        RESPOND_NO_SUCH_FUNCTION(response_json);
    }

    /* Remove the leading & trailing slashes */

    while (path[0] == '/') {
        path++;
    }

    int path_len = strlen(path);
    while (path[path_len - 1] == '/') {
        path[path_len - 1] = 0;
        path_len--;
    }

    /* Split path */

    token = strtok(path, "/");
    if (!token) {
        RESPOND_NO_SUCH_FUNCTION(response_json);
    }
    part1 = strdup(token);

    token = strtok(NULL, "/");
    if (token) {
        part2 = strdup(token);
    }

    token = strtok(NULL, "/");
    if (token) {
        part3 = strdup(token);
    }

    token = strtok(NULL, "/");
    if (token) {
        RESPOND_NO_SUCH_FUNCTION(response_json);
    }

    /* Determine the API endpoint and method */

    if (!strcmp(part1, "device")) {
        if (part2) {
            RESPOND_NO_SUCH_FUNCTION(response_json);
        }
        else {
            if (method == HTTP_METHOD_GET) {
                response_json = api_get_device(query_json, code);
            }
            else if (method == HTTP_METHOD_PATCH) {
                response_json = api_patch_device(query_json, request_json, code);
            }
            else {
                RESPOND_NO_SUCH_FUNCTION(response_json);
            }
        }
    }
    else if (!strcmp(part1, "reset") && method == HTTP_METHOD_POST && !part2) {
        response_json = api_post_reset(query_json, request_json, code);
    }
    else if (!strcmp(part1, "firmware") && !part2) {
        if (method == HTTP_METHOD_GET) {
            response_json = api_get_firmware(query_json, code);
        }
        else if (method == HTTP_METHOD_PATCH) {
            response_json = api_patch_firmware(query_json, request_json, code);
        }
        else {
            RESPOND_NO_SUCH_FUNCTION(response_json);
        }
    }
    else if (!strcmp(part1, "wifi")) {
        if (part2) {
            RESPOND_NO_SUCH_FUNCTION(response_json);
        }

        if (method == HTTP_METHOD_GET) {
            response_json = api_get_wifi(query_json, code);
        }
        else {
            RESPOND_NO_SUCH_FUNCTION(response_json);
        }
    }
    else {
        RESPOND_NO_SUCH_FUNCTION(response_json);
    }

    response:

    free(part1);
    free(part2);
    free(part3);

    return response_json;
}

void api_conn_set(struct espconn *conn) {
    if (api_conn) {
        DEBUG_API("overwriting existing API connection");
    }

    api_conn = conn;
}

bool api_conn_busy(void) {
    return !!api_conn;
}

bool api_conn_equal(struct espconn *conn) {
    if (!api_conn) {
        return FALSE;
    }

    return CONN_EQUAL(conn, api_conn);
}

void api_conn_reset(void) {
    api_conn = NULL;
}

json_t *api_get_device(json_t *query_json, int *code) {
    DEBUG_API("returning device attributes");

    char value[256];
    json_t *response_json = json_obj_new();

    /* General device properties */
    json_obj_append(response_json, "name", json_str_new(device_name));
    json_obj_append(response_json, "version", json_str_new(FW_VERSION));
    json_obj_append(response_json, "uptime", json_int_new(system_uptime()));

    /* IP configuration */
    ip_addr_t ip = wifi_get_ip_address();
    if (ip.addr) {
        snprintf(value, 256, WIFI_IP_FMT, IP2STR(&ip));
        json_obj_append(response_json, "ip_address", json_str_new(value));
    }
    else {
        json_obj_append(response_json, "ip_address", json_str_new(""));
    }

    json_obj_append(response_json, "ip_netmask", json_int_new(wifi_get_netmask()));

    ip = wifi_get_gateway();
    if (ip.addr) {
        snprintf(value, 256, WIFI_IP_FMT, IP2STR(&ip));
        json_obj_append(response_json, "ip_gateway", json_str_new(value));
    }
    else {
        json_obj_append(response_json, "ip_gateway", json_str_new(""));
    }

    ip = wifi_get_dns();
    if (ip.addr) {
        snprintf(value, 256, WIFI_IP_FMT, IP2STR(&ip));
        json_obj_append(response_json, "ip_dns", json_str_new(value));
    }
    else {
        json_obj_append(response_json, "ip_dns", json_str_new(""));
    }

    /* Current IP info */
    ip = wifi_get_ip_address_current();
    if (ip.addr) {
        snprintf(value, 256, WIFI_IP_FMT, IP2STR(&ip));
        json_obj_append(response_json, "ip_address_current", json_str_new(value));
    }
    else {
        json_obj_append(response_json, "ip_address_current", json_str_new(""));
    }

    json_obj_append(response_json, "ip_netmask_current", json_int_new(wifi_get_netmask_current()));

    ip = wifi_get_gateway_current();
    if (ip.addr) {
        snprintf(value, 256, WIFI_IP_FMT, IP2STR(&ip));
        json_obj_append(response_json, "ip_gateway_current", json_str_new(value));
    }
    else {
        json_obj_append(response_json, "ip_gateway_current", json_str_new(""));
    }

    ip = wifi_get_dns_current();
    if (ip.addr) {
        snprintf(value, 256, WIFI_IP_FMT, IP2STR(&ip));
        json_obj_append(response_json, "ip_dns_current", json_str_new(value));
    }
    else {
        json_obj_append(response_json, "ip_dns_current", json_str_new(""));
    }

    /* Wi-Fi configuration */
    char *ssid = wifi_get_ssid();
    char *psk = wifi_get_psk();
    uint8 *bssid = wifi_get_bssid();

    if (ssid) {
        json_obj_append(response_json, "wifi_ssid", json_str_new(ssid));
    }
    else {
        json_obj_append(response_json, "wifi_ssid", json_str_new(""));
    }

    if (psk) {
        json_obj_append(response_json, "wifi_key", json_str_new(psk));
    }
    else {
        json_obj_append(response_json, "wifi_key", json_str_new(""));
    }

    if (bssid) {
        snprintf(value, 256, "%02X:%02X:%02X:%02X:%02X:%02X", WIFI_BSSID2STR(bssid));
        json_obj_append(response_json, "wifi_bssid", json_str_new(value));
    }
    else {
        json_obj_append(response_json, "wifi_bssid", json_str_new(""));
    }

    /* Current Wi-Fi info */
    int rssi = wifi_station_get_rssi();
    if (rssi < -100) {
        rssi = -100;
    }
    if (rssi > -30) {
        rssi = -30;
    }

    char current_bssid_str[18] = {0};
    if (wifi_station_is_connected()) {
        snprintf(
            current_bssid_str,
            sizeof(current_bssid_str),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            WIFI_BSSID2STR(wifi_get_bssid_current())
        );
    }
    json_obj_append(response_json, "wifi_bssid_current", json_str_new(current_bssid_str));

    int8  wifi_signal_strength = -1;
    if (wifi_station_is_connected()) {
        if (rssi >= WIFI_RSSI_EXCELLENT) {
            wifi_signal_strength = 3;
        }
        else if (rssi >= WIFI_RSSI_GOOD) {
            wifi_signal_strength = 2;
        }
        else if (rssi >= WIFI_RSSI_FAIR) {
            wifi_signal_strength = 1;
        }
        else {
            wifi_signal_strength = 0;
        }
    }
    json_obj_append(response_json, "wifi_signal_strength", json_int_new(wifi_signal_strength));

    /* Memory information */
    json_obj_append(response_json, "mem_usage", json_int_new(100 - 100 * system_get_free_heap_size() / MAX_AVAILABLE_RAM));
    json_obj_append(response_json, "flash_size", json_int_new(system_get_flash_size() / 1024));

    char id[10];
    snprintf(id, 10, "%08x", spi_flash_get_id());
    json_obj_append(response_json, "flash_id", json_str_new(id));

    snprintf(id, 10, "%08x", system_get_chip_id());
    json_obj_append(response_json, "chip_id", json_str_new(id));

    json_obj_append(response_json, "firmware_auto_update", json_bool_new(device_flags & DEVICE_FLAG_OTA_AUTO_UPDATE));
    json_obj_append(response_json, "firmware_beta_enabled", json_bool_new(device_flags & DEVICE_FLAG_OTA_BETA_ENABLED));

#ifdef _DEBUG
    json_obj_append(response_json, "debug", json_bool_new(TRUE));
#else
    json_obj_append(response_json, "debug", json_bool_new(FALSE));
#endif

    json_obj_append(response_json, "setup_mode", json_bool_new(system_setup_mode_active()));

    *code = 200;
    
    return response_json;
}

json_t *api_patch_device(json_t *query_json, json_t *request_json, int *code) {
    DEBUG_API("updating device attributes");

    json_t *response_json = json_obj_new();

    if (json_get_type(request_json) != JSON_TYPE_OBJ) {
        return API_ERROR(response_json, 400, "invalid-request");
    }

    bool needs_reset = FALSE;

    int i;
    char *key;
    json_t *child;
    for (i = 0; i < json_obj_get_len(request_json); i++) {
        key = json_obj_key_at(request_json, i);
        child = json_obj_value_at(request_json, i);

        if (!strcmp(key, "name")) {
            if (json_get_type(child) != JSON_TYPE_STR) {
                return INVALID_FIELD(response_json, key);
            }

            char *value = json_str_get(child);
            if (strlen(value) >= DEVICE_NAME_LEN) {
                return INVALID_FIELD(response_json, key);
            }

            free(device_name);
            device_name = strdup(value);

            DEBUG_DEVICE("name set to \"%s\"", device_name);

            httpserver_set_name(device_name);
        }
        else if (!strcmp(key, "password")) {
            if (json_get_type(child) != JSON_TYPE_STR) {
                return INVALID_FIELD(response_json, key);
            }

            char *password = json_str_get(child);
            if (strlen(password) >= DEVICE_PASSWORD_LEN) {
                return INVALID_FIELD(response_json, key);
            }

            free(device_password);
            device_password = strdup(password);
            DEBUG_DEVICE("password set");
        }
        else if (!strcmp(key, "ip_address")) {
            if (json_get_type(child) != JSON_TYPE_STR) {
                return INVALID_FIELD(response_json, key);
            }

            char *ip_address_str = json_str_get(child);
            ip_addr_t ip_address = {0};
            if (ip_address_str[0]) { /* Manual */
                uint8 bytes[4];
                if (!validate_ip_address(ip_address_str, bytes)) {
                    return INVALID_FIELD(response_json, key);
                }

                IP4_ADDR(&ip_address, bytes[0], bytes[1], bytes[2], bytes[3]);
            }

            wifi_set_ip_address(ip_address);
            needs_reset = TRUE;
        }
        else if (!strcmp(key, "ip_netmask")) {
            if (json_get_type(child) != JSON_TYPE_INT) {
                return INVALID_FIELD(response_json, key);
            }

            int netmask = json_int_get(child);
            if (netmask < 0 || netmask > 31) {
                return INVALID_FIELD(response_json, key);
            }

            wifi_set_netmask(netmask);
            needs_reset = TRUE;
        }
        else if (!strcmp(key, "ip_gateway")) {
            if (json_get_type(child) != JSON_TYPE_STR) {
                return INVALID_FIELD(response_json, key);
            }

            char *gateway_str = json_str_get(child);
            ip_addr_t gateway = {0};
            if (gateway_str[0]) { /* Manual */
                uint8 bytes[4];
                if (!validate_ip_address(gateway_str, bytes)) {
                    return INVALID_FIELD(response_json, key);
                }

                IP4_ADDR(&gateway, bytes[0], bytes[1], bytes[2], bytes[3]);
            }

            wifi_set_gateway(gateway);
            needs_reset = TRUE;
        }
        else if (!strcmp(key, "ip_dns")) {
            if (json_get_type(child) != JSON_TYPE_STR) {
                return INVALID_FIELD(response_json, key);
            }

            char *dns_str = json_str_get(child);
            ip_addr_t dns = {0};
            if (dns_str[0]) { /* Manual */
                uint8 bytes[4];
                if (!validate_ip_address(dns_str, bytes)) {
                    return INVALID_FIELD(response_json, key);
                }

                IP4_ADDR(&dns, bytes[0], bytes[1], bytes[2], bytes[3]);
            }

            wifi_set_dns(dns);
            needs_reset = TRUE;
        }
        else if (!strcmp(key, "wifi_ssid")) {
            if (json_get_type(child) != JSON_TYPE_STR) {
                return INVALID_FIELD(response_json, key);
            }

            char *ssid = json_str_get(child);
            if (!validate_wifi_ssid(ssid)) {
                return INVALID_FIELD(response_json, key);
            }

            wifi_set_ssid(ssid);
            needs_reset = TRUE;
        }
        else if (!strcmp(key, "wifi_key")) {
            if (json_get_type(child) != JSON_TYPE_STR) {
                return INVALID_FIELD(response_json, key);
            }

            char *psk = json_str_get(child);
            if (!validate_wifi_key(psk)) {
                return INVALID_FIELD(response_json, key);
            }

            wifi_set_psk(psk);
            needs_reset = TRUE;
        }
        else if (!strcmp(key, "wifi_bssid")) {
            if (json_get_type(child) != JSON_TYPE_STR) {
                return INVALID_FIELD(response_json, key);
            }

            char *bssid_str = json_str_get(child);
            if (bssid_str[0]) {
                uint8 bssid[WIFI_BSSID_LEN];
                if (!validate_wifi_bssid(bssid_str, bssid)) {
                    return INVALID_FIELD(response_json, key);
                }

                wifi_set_bssid(bssid);
            }
            else {
                wifi_set_bssid(NULL);
            }
            needs_reset = TRUE;
        }
        else if (!strcmp(key, "firmware_auto_update")) {
            if (json_get_type(child) != JSON_TYPE_BOOL) {
                return INVALID_FIELD(response_json, key);
            }

            if (json_bool_get(child)) {
                device_flags |= DEVICE_FLAG_OTA_AUTO_UPDATE;
                DEBUG_OTA("firmware auto update enabled");
            }
            else {
                device_flags &= ~DEVICE_FLAG_OTA_AUTO_UPDATE;
                DEBUG_OTA("firmware auto update disabled");
            }
        }
        else if (!strcmp(key, "firmware_beta_enabled")) {
            if (json_get_type(child) != JSON_TYPE_BOOL) {
                return INVALID_FIELD(response_json, key);
            }

            if (json_bool_get(child)) {
                device_flags |= DEVICE_FLAG_OTA_BETA_ENABLED;
                DEBUG_OTA("firmware beta enabled");
            }
            else {
                device_flags &= ~DEVICE_FLAG_OTA_BETA_ENABLED;
                DEBUG_OTA("firmware beta disabled");
            }
        }
        else {
            return NO_SUCH_ATTR(response_json, key);
        }
    }

    config_save();

    if (needs_reset) {
        DEBUG_API("reset needed");

        system_reset(/* delayed = */ TRUE);
    }

    *code = 204;

    return response_json;
}

json_t *api_post_reset(json_t *query_json, json_t *request_json, int *code) {
    json_t *response_json = json_obj_new();

    if (json_get_type(request_json) != JSON_TYPE_OBJ) {
        return API_ERROR(response_json, 400, "invalid-request");
    }

    json_t *factory_json = json_obj_lookup_key(request_json, "factory");
    bool factory = FALSE;
    if (factory_json) {
        if (json_get_type(factory_json) != JSON_TYPE_BOOL) {
            return INVALID_FIELD(response_json, "factory");
        }
        factory = json_bool_get(factory_json);
    }

    if (factory) {
        config_factory_reset();
        wifi_reset();
    }

    *code = 204;

    system_reset(/* delayed = */ TRUE);
    
    return response_json;
}

json_t *api_get_firmware(json_t *query_json, int *code) {
    json_t *response_json = NULL;

    int ota_state = ota_current_state();

    if (ota_state == OTA_STATE_IDLE) {
        bool beta = device_flags & DEVICE_FLAG_OTA_BETA_ENABLED;
        json_t *beta_json = json_obj_lookup_key(query_json, "beta");
        if (beta_json) {
            beta = !strcmp(json_str_get(beta_json), "true");
        }

        if (!ota_get_latest(beta, on_ota_latest)) {
            response_json = json_obj_new();
            json_obj_append(response_json, "error", json_str_new("busy"));
            *code = 503;
        }
    }
    else {  /* OTA busy */
        char *ota_state_str = ota_states_str[ota_state];

        response_json = json_obj_new();
        json_obj_append(response_json, "version", json_str_new(FW_VERSION));
        json_obj_append(response_json, "status", json_str_new(ota_state_str));
    }

    return response_json;
}

json_t *api_patch_firmware(json_t *query_json, json_t *request_json, int *code) {
    json_t *response_json = NULL;

    if (json_get_type(request_json) != JSON_TYPE_OBJ) {
        return API_ERROR(response_json, 400, "invalid-request");
    }
    
    json_t *version_json = json_obj_lookup_key(request_json, "version");
    json_t *url_json = json_obj_lookup_key(request_json, "url");
    if (!version_json && !url_json) {
        return MISSING_FIELD(response_json, "version");
    }

    if (url_json) {  /* URL given */
        if (json_get_type(url_json) != JSON_TYPE_STR) {
            return INVALID_FIELD(response_json, "url");
        }

        ota_perform_url(json_str_get(url_json), on_ota_perform);
    }
    else { /* Assuming version_json */
        if (json_get_type(version_json) != JSON_TYPE_STR) {
            return INVALID_FIELD(response_json, "version");
        }

        ota_perform_version(json_str_get(version_json), on_ota_perform);
    }

    return NULL;
}

json_t *api_get_wifi(json_t *query_json, int *code) {
    json_t *response_json = NULL;

    if (!wifi_scan(on_wifi_scan)) {
        response_json = json_obj_new();
        json_obj_append(response_json, "error", json_str_new("busy"));
        *code = 503;

        return response_json;
    }

    return NULL;
}

json_t *_api_error(json_t *response_json, char *error, char *field_name, char *field_value) {
    json_free(response_json);
    response_json = json_obj_new();
    json_obj_append(response_json, "error", json_str_new(error));
    if (field_name) {
        json_obj_append(response_json, field_name, json_str_new(field_value));
    }
    return response_json;
}

void on_ota_latest(char *version, char *date, char *url) {
    if (api_conn) {
        json_t *response_json = json_obj_new();
        json_obj_append(response_json, "version", json_str_new(FW_VERSION));
        json_obj_append(response_json, "status", json_str_new(ota_states_str[OTA_STATE_IDLE]));

        if (version) {
            json_obj_append(response_json, "latest_version", json_str_new(version));
            json_obj_append(response_json, "latest_date", json_str_new(date));
            json_obj_append(response_json, "latest_url", json_str_new(url));
        }

        respond_json(api_conn, 200, response_json);
        api_conn_reset();
    }

    free(version);
    free(date);
    free(url);
}

void on_ota_perform(int code) {
    if (!api_conn) {
        return;
    }

    if (code >= 200 && code < 300) {
        json_t *response_json = json_obj_new();
        respond_json(api_conn, 204, response_json);
    }
    else {  /* Error */
        json_t *response_json = json_obj_new();
        json_obj_append(response_json, "error", json_str_new("no-such-version"));
        respond_json(api_conn, 404, response_json);
    }

    api_conn_reset();
}

void on_wifi_scan(wifi_scan_result_t *results, int len) {
    DEBUG_API("got wifi scan results");

    if (!api_conn) {
        return;  /* Such is life */
    }

    json_t *response_json = json_list_new();
    json_t *result_json;
    int i;
    char bssid[18];
    char *auth;

    for (i = 0; i < len; i++) {
        result_json = json_obj_new();
        json_obj_append(result_json, "ssid", json_str_new(results[i].ssid));
        snprintf(
            bssid,
            sizeof(bssid),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            results[i].bssid[0],
            results[i].bssid[1],
            results[i].bssid[2],
            results[i].bssid[3],
            results[i].bssid[4],
            results[i].bssid[5]
        );
        json_obj_append(result_json, "bssid", json_str_new(bssid));
        json_obj_append(result_json, "channel", json_int_new(results[i].channel));
        json_obj_append(result_json, "rssi", json_int_new(results[i].rssi));

        switch (results[i].auth_mode) {
            case AUTH_OPEN:
                auth = "OPEN";
                break;
            case AUTH_WEP:
                auth = "WEP";
                break;
            case AUTH_WPA_PSK:
                auth = "WPA_PSK";
                break;
            case AUTH_WPA2_PSK:
                auth = "WPA2_PSK";
                break;
            case AUTH_WPA_WPA2_PSK:
                auth = "WPA_WPA2_PSK";
                break;
            default:
                auth = "unknown";
        }
        json_obj_append(result_json, "auth_mode", json_str_new(auth));

        json_list_append(response_json, result_json);
    }

    free(results);

    respond_json(api_conn, 200, response_json);
    api_conn_reset();
}

bool validate_ip_address(char *ip, uint8 *a) {
    a[0] = 0;
    char *s = ip;
    int c, i = 0;
    while ((c = *s++)) {
        if (isdigit(c)) {
            a[i] = a[i] * 10 + (c - '0');
        }
        else if ((c == '.') && (i < 3)) {
            i++;
            a[i] = 0;
        }
        else {
            return FALSE;
        }
    }

    if (i < 3) {
        return FALSE;
    }

    return TRUE;
}

bool validate_wifi_ssid(char *ssid) {
    int len = strlen(ssid);
    return len > 0 && len <= WIFI_SSID_MAX_LEN;
}

bool validate_wifi_key(char *key) {
    return strlen(key) <= WIFI_PSK_MAX_LEN;
}

bool validate_wifi_bssid(char *bssid_str, uint8 *bssid) {
    int i, len = 0;
    char *s = bssid_str;
    char t[3] = {0, 0, 0};

    while (TRUE) {
        if (len > WIFI_BSSID_LEN) {
            return FALSE;
        }

        if (!s[0]) {
            break;
        }

        if (!s[1]) {
            return FALSE;
        }

        t[0] = s[0]; t[1] = s[1];
        bssid[len++] = strtol(t, NULL, 16);

        for (i = 0; i < 3 && *s; i++) {
            s++;
        }
    }

    if (len != WIFI_BSSID_LEN) {
        return FALSE;
    }

    return TRUE;
}
