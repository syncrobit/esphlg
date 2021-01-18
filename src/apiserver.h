
#ifndef _API_SERVER_H
#define _API_SERVER_H


#include "espgoodies/json.h"
#include "espgoodies/tcpserver.h"


#ifdef _DEBUG_APISERVER
#define DEBUG_APISERVER(fmt, ...)            DEBUG("[apiserver     ] " fmt, ##__VA_ARGS__)
#define DEBUG_APISERVER_CONN(conn, fmt, ...) DEBUG_CONN("apiserver", conn, fmt, ##__VA_ARGS__)
#else
#define DEBUG_APISERVER(...)                 {}
#define DEBUG_APISERVER_CONN(...)            {}
#endif


#define API_SERVER_TCP_PORT 80


void ICACHE_FLASH_ATTR api_server_init(void);

    /* respond_json() will free the json structure by itself, using json_free() ! */
void ICACHE_FLASH_ATTR respond_json(struct espconn *conn, int status, json_t *json);
void ICACHE_FLASH_ATTR respond_error(struct espconn *conn, int status, char *error);
void ICACHE_FLASH_ATTR respond_html(struct espconn *conn, int status, uint8 *html, int len);


#endif /* _API_SERVER_H */
