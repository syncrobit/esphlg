
#ifndef _ESPGOODIES_HTTPUTILS_H
#define _ESPGOODIES_HTTPUTILS_H


#include "espgoodies/json.h"


json_t ICACHE_FLASH_ATTR *http_parse_url_encoded(char *input);
char   ICACHE_FLASH_ATTR *http_build_auth_header(char *token, char *type);
char   ICACHE_FLASH_ATTR *http_parse_auth_header(char *header, char *type);
bool   ICACHE_FLASH_ATTR  http_decode_basic_auth(char *basic_auth, char **username, char **password);


#endif /* _ESPGOODIES_HTTPUTILS_H */
