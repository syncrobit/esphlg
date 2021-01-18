
#ifndef _ESPGOODIES_HTML_H
#define _ESPGOODIES_HTML_H

#include <c_types.h>


#ifdef _DEBUG_HTML
#define DEBUG_HTML(fmt, ...) DEBUG("[html          ] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_HTML(...)      {}
#endif


/* Result must be freed */
uint8 ICACHE_FLASH_ATTR *html_load(uint32 *len);


#endif /* _ESPGOODIES_HTML_H */
