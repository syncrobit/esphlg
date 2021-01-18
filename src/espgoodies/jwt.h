
#ifndef _ESPGOODIES_JWT_H
#define _ESPGOODIES_JWT_H


#include <c_types.h>

#include "espgoodies/json.h"


#define JWT_ALG_HS256 1


typedef struct {

    json_t *claims;
    uint8   alg;

} jwt_t;


jwt_t ICACHE_FLASH_ATTR *jwt_new(uint8 alg, json_t *claims);
void  ICACHE_FLASH_ATTR  jwt_free(jwt_t *jwt);

char  ICACHE_FLASH_ATTR *jwt_dump(jwt_t *jwt, char *secret);
jwt_t ICACHE_FLASH_ATTR *jwt_parse(char *jwt_str);
bool  ICACHE_FLASH_ATTR  jwt_validate(char *jwt_str, uint8 alg, char *secret);


#endif /* _ESPGOODIES_JWT_H */
