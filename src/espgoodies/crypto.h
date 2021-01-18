
/*
 * sha1 stuff copied from https://github.com/esp8266/Arduino
 * sha256 stuff copied from https://github.com/B-Con/crypto-algorithms
 * hmac_sha256 stuff copied from https://github.com/cantora/avr-crypto-lib
 * base64 stuff copied from https://github.com/littlstar/b64.c
 */

#ifndef _ESPGOODIES_CRYPTO_H
#define _ESPGOODIES_CRYPTO_H


#include <c_types.h>


#define SHA1_LEN       20
#define SHA1_HEX_LEN   40
#define SHA256_LEN     32
#define SHA256_HEX_LEN 64


/* Result must be freed after using the following functions */

uint8 ICACHE_FLASH_ATTR *sha1(uint8 *data, int len);
char  ICACHE_FLASH_ATTR *sha1_hex(char *s);
uint8 ICACHE_FLASH_ATTR *sha256(uint8 *data, int len);
char  ICACHE_FLASH_ATTR *sha256_hex(char *s);

uint8 ICACHE_FLASH_ATTR *hmac_sha256(uint8 *data, int data_len, uint8 *key, int key_len);
char  ICACHE_FLASH_ATTR *hmac_sha256_hex(char *msg, char *key);


/* URL-safe base64 */

char  ICACHE_FLASH_ATTR *b64_encode(uint8 *data, int len, bool padding);
uint8 ICACHE_FLASH_ATTR *b64_decode(char *s);

char  ICACHE_FLASH_ATTR *bin2hex(uint8 *bin, int len);
uint8 ICACHE_FLASH_ATTR *hex2bin(char *hex);


#endif /* _ESPGOODIES_CRYPTO_H */
