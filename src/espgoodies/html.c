
#include <mem.h>
#include <c_types.h>
#include <spi_flash.h>
#include <user_interface.h>

#include "espgoodies/common.h"
#include "espgoodies/html.h"


#define USER1_FLASH_HTML_ADDR 0x78000
#define USER2_FLASH_HTML_ADDR 0xF8000
#define MAX_HTML_LEN          16384


uint8 *html_load(uint32 *len) {
    uint32 flash_html_addr = system_upgrade_userbin_check() ? USER2_FLASH_HTML_ADDR : USER1_FLASH_HTML_ADDR;

    ETS_INTR_LOCK();
    if (spi_flash_read(flash_html_addr, len, 4) != SPI_FLASH_RESULT_OK) {
        ETS_INTR_UNLOCK();
        DEBUG_HTML("failed to read HTML length from flash at 0x%05X", flash_html_addr);
        return NULL;
    }
    ETS_INTR_UNLOCK();

    DEBUG_HTML("HTML length is %d bytes", *len);
    if (*len > MAX_HTML_LEN) {
        DEBUG_HTML("refusing to read HTML longer than %d bytes", MAX_HTML_LEN);
        return NULL;
    }

    uint32 free_mem = system_get_free_heap_size();
    if (free_mem < (*len) * 2) {
        DEBUG_HTML("not enough memory to load HTML");
        return NULL;
    }

    uint8 *data = malloc(*len);
    uint32 addr = flash_html_addr + 4;
    ETS_INTR_LOCK();
    if (spi_flash_read(addr, (uint32 *) data, *len) != SPI_FLASH_RESULT_OK) {
        ETS_INTR_UNLOCK();
        DEBUG_HTML("failed to read %d bytes from flash at 0x%05X", *len, addr);
        free(data);
        return NULL;
    }
    ETS_INTR_UNLOCK();

    DEBUG_HTML("successfully read %d bytes from flash at 0x%05X", *len, flash_html_addr);

    return data;
}
