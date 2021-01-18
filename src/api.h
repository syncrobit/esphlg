
#ifndef _API_H
#define _API_H


#include <ip_addr.h>
#include <espconn.h>

#include "espgoodies/json.h"


#ifdef _DEBUG_API
#define DEBUG_API(fmt, ...) DEBUG("[api           ] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_API(...)      {}
#endif


json_t ICACHE_FLASH_ATTR *api_call_handle(int method, char* path, json_t *query_json, json_t *request_json, int *code);

void   ICACHE_FLASH_ATTR  api_conn_set(struct espconn *conn);
bool   ICACHE_FLASH_ATTR  api_conn_busy(void);
bool   ICACHE_FLASH_ATTR  api_conn_equal(struct espconn *conn);
void   ICACHE_FLASH_ATTR  api_conn_reset(void);

json_t ICACHE_FLASH_ATTR *api_get_device(json_t *query_json, int *code);
json_t ICACHE_FLASH_ATTR *api_patch_device(json_t *query_json, json_t *request_json, int *code);

json_t ICACHE_FLASH_ATTR *api_post_reset(json_t *query_json, json_t *request_json, int *code);
json_t ICACHE_FLASH_ATTR *api_get_firmware(json_t *query_json, int *code);
json_t ICACHE_FLASH_ATTR *api_patch_firmware(json_t *query_json, json_t *request_json, int *code);

json_t ICACHE_FLASH_ATTR *api_get_wifi(json_t *query_json, int *code);


#endif /* _API_H */
