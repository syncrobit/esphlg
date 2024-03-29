
#include <mem.h>
#include <c_types.h>
#include <spi_flash.h>

#include "espgoodies/common.h"
#include "espgoodies/flashcfg.h"


bool flashcfg_load(uint8 slot, uint8 *data) {
    uint32 size = 0;
    uint32 offs = 0;
    switch (slot) {
        case FLASH_CONFIG_SLOT_DEFAULT:
            size = FLASH_CONFIG_SIZE_DEFAULT;
            offs = FLASH_CONFIG_OFFS_DEFAULT;
            break;

        case FLASH_CONFIG_SLOT_SYSTEM:
            size = FLASH_CONFIG_SIZE_SYSTEM;
            offs = FLASH_CONFIG_OFFS_SYSTEM;
            break;
    }

    SpiFlashOpResult result = spi_flash_read(FLASH_CONFIG_ADDR + offs, (uint32 *) data, size);
    if (result != SPI_FLASH_RESULT_OK) {
        DEBUG_FLASHCFG("failed to read %d bytes from flash config at 0x%05X", size, FLASH_CONFIG_ADDR + offs);
        return FALSE;
    }
    else {
        DEBUG_FLASHCFG("successfully read %d bytes from flash config at 0x%05X", size, FLASH_CONFIG_ADDR + offs);
    }

    return TRUE;
}

bool flashcfg_save(uint8 slot, uint8 *data) {
    uint32 size = 0;
    uint32 offs = 0;
    switch (slot) {
        case FLASH_CONFIG_SLOT_DEFAULT:
            size = FLASH_CONFIG_SIZE_DEFAULT;
            offs = FLASH_CONFIG_OFFS_DEFAULT;
            break;

        case FLASH_CONFIG_SLOT_SYSTEM:
            size = FLASH_CONFIG_SIZE_SYSTEM;
            offs = FLASH_CONFIG_OFFS_SYSTEM;
            break;
    }

    int i, sector;
    SpiFlashOpResult result;
    for (i = 0; i < size / SPI_FLASH_SEC_SIZE; i++) {
        sector = (FLASH_CONFIG_ADDR + offs + SPI_FLASH_SEC_SIZE * i) / SPI_FLASH_SEC_SIZE;
        result = spi_flash_erase_sector(sector);
        if (result != SPI_FLASH_RESULT_OK) {
            DEBUG_FLASHCFG(
                "failed to erase flash sector at 0x%05X",
                FLASH_CONFIG_ADDR + offs + SPI_FLASH_SEC_SIZE * i
            );
            return FALSE;
        }
        else {
            DEBUG_FLASHCFG(
                "flash sector at 0x%05X erased",
                FLASH_CONFIG_ADDR + offs + SPI_FLASH_SEC_SIZE * i
            );
        }
    }

    result = spi_flash_write(FLASH_CONFIG_ADDR + offs, (uint32 *) data, size);
    if (result != SPI_FLASH_RESULT_OK) {
        DEBUG_FLASHCFG("failed to write %d bytes of flash config at 0x%05X", size, FLASH_CONFIG_ADDR + offs);
        return FALSE;
    }
    else {
        DEBUG_FLASHCFG("successfully written %d bytes of flash config at 0x%05X", size, FLASH_CONFIG_ADDR + offs);
    }

    return TRUE;
}

bool flashcfg_reset(uint8 slot) {
    uint32 size = 0;
    switch (slot) {
        case FLASH_CONFIG_SLOT_DEFAULT:
            size = FLASH_CONFIG_SIZE_DEFAULT;
            break;

        case FLASH_CONFIG_SLOT_SYSTEM:
            size = FLASH_CONFIG_SIZE_SYSTEM;
            break;
    }

    uint8 *config_data = zalloc(size);
    bool result = flashcfg_save(slot, config_data);
    free(config_data);

    return result;
}
