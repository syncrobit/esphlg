
#include <c_types.h>
#include <spi_flash.h>
#include <osapi.h>

#include "espgoodies/common.h"
#include "espgoodies/system.h"
#include "espgoodies/initdata.h"


const uint32 esp_init_data_default[] = _ESP_INIT_DATA_DEFAULT_HEX;


void init_data_ensure(void) {
    uint32 rf_cal_sec = 256 - 5;
    uint32 addr = (rf_cal_sec * SPI_FLASH_SEC_SIZE) + 0x1000;
    uint32 rf_cal_data;

    if (sizeof(esp_init_data_default) > 0) {
        ETS_INTR_LOCK();
        spi_flash_read(addr, &rf_cal_data, 4);
        if (rf_cal_data != esp_init_data_default[0]) {
            DEBUG_SYSTEM("flashing esp_init_data_default");
            spi_flash_erase_sector(rf_cal_sec);
            spi_flash_write(addr, (uint32 *) esp_init_data_default, sizeof(esp_init_data_default));
        }
        else {
            DEBUG_SYSTEM("esp_init_data_default is up-to-date");
        }
        ETS_INTR_UNLOCK();
        os_delay_us(10000); /* Helps printing debug message */
    }
}
