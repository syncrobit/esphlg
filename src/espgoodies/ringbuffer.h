
#ifndef _ESPGOODIES_RING_BUFFER_H
#define _ESPGOODIES_RING_BUFFER_H


#include <c_types.h>


typedef struct {

    uint8  *data;
    uint16  size;
    uint16  avail;
    uint16  head;
    uint16  tail;

} ring_buffer_t;


ring_buffer_t ICACHE_FLASH_ATTR *ring_buffer_new(uint16 size);
void          ICACHE_FLASH_ATTR  ring_buffer_free(ring_buffer_t *ring_buffer);
uint16        ICACHE_FLASH_ATTR  ring_buffer_peek(ring_buffer_t *ring_buffer, uint8 *data, uint16 len);
uint16        ICACHE_FLASH_ATTR  ring_buffer_read(ring_buffer_t *ring_buffer, uint8 *data, uint16 len);
/* Intentionally not placed into flash so that it can be used from UART interrupt handler */
uint16                           ring_buffer_write(ring_buffer_t *ring_buffer, uint8 *data, uint16 len);


#endif /* _ESPGOODIES_RING_BUFFER_H */
