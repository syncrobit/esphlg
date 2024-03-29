
#ifndef _ESPGOODIES_UTILS_H
#define _ESPGOODIES_UTILS_H


#include <os_type.h>
#include <ctype.h>


#define IS_HEX(c)             (isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
#define CONN_EQUAL(c1, c2)    (!memcmp(c1->proto.tcp->remote_ip, c2->proto.tcp->remote_ip, 4) && \
                               c1->proto.tcp->remote_port == c2->proto.tcp->remote_port)
#define MIN(a, b)             (a) > (b) ? (b) : (a)
#define MAX(a, b)             (a) > (b) ? (a) : (b)

#define FMT_UINT64_VAL(value) ((unsigned long) (value >> 32)), ((unsigned long) value)
#define FMT_UINT64_HEX        "%08lX%08lX"

#define htons(x)              (((x)<< 8 & 0xFF00) | ((x)>> 8 & 0x00FF))
#define ntohs(x)              htons(x)

#define htonl(x)              (((x)<<24 & 0xFF000000UL) | \
                               ((x)<< 8 & 0x00FF0000UL) | \
                               ((x)>> 8 & 0x0000FF00UL) | \
                               ((x)>>24 & 0x000000FFUL))

#define ntohl(x)              htonl(x)


typedef void (*call_later_callback_t)(void *arg);


void   ICACHE_FLASH_ATTR  append_max_len(char *s, char c, int max_len);
int    ICACHE_FLASH_ATTR  realloc_chunks(char **p, int current_size, int req_size);
double ICACHE_FLASH_ATTR  strtod(const char *s, char **endptr);
char   ICACHE_FLASH_ATTR *dtostr(double d, int8 decimals);
double ICACHE_FLASH_ATTR  decent_round(double d);
double ICACHE_FLASH_ATTR  round_to(double d, uint8 decimals);
int    ICACHE_FLASH_ATTR  compare_double(const void *a, const void *b);
bool   ICACHE_FLASH_ATTR  call_later(call_later_callback_t callback, void *arg, uint32 delay_ms);


#endif /* _ESPGOODIES_UTILS_H */
