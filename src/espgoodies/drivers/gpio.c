
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <ctype.h>
#include <limits.h>
#include <user_interface.h>
#include <espconn.h>

#include "espgoodies/common.h"
#include "espgoodies/drivers/gpio.h"


typedef struct {

    gpio_interrupt_handler_t  handler;
    void                     *arg;
    uint8                     gpio_no;
    uint8                     type;

} intr_details_t;


static uint32          gpio_configured_state = 0;
static uint32          gpio_output_state = 0;
static uint32          gpio_pull_state = 0;
static bool            interrupt_handler_attached = FALSE;
static intr_details_t *intr_details = NULL;
static uint8           intr_details_count = 0;


static void ICACHE_FLASH_ATTR update_interrupts(void);
static void                   gpio_interrupt_handler(void *arg);


int gpio_get_mux(int gpio_no) {
    switch (gpio_no) {
        case 0:
            return PERIPHS_IO_MUX_GPIO0_U;

        case 1:
            return PERIPHS_IO_MUX_U0TXD_U;

        case 2:
            return PERIPHS_IO_MUX_GPIO2_U;

        case 3:
            return PERIPHS_IO_MUX_U0RXD_U;

        case 4:
            return PERIPHS_IO_MUX_GPIO4_U;

        case 5:
            return PERIPHS_IO_MUX_GPIO5_U;

        case 9:
            return PERIPHS_IO_MUX_SD_DATA2_U;

        case 10:
            return PERIPHS_IO_MUX_SD_DATA3_U;

        case 12:
            return PERIPHS_IO_MUX_MTDI_U;

        case 13:
            return PERIPHS_IO_MUX_MTCK_U;

        case 14:
            return PERIPHS_IO_MUX_MTMS_U;

        case 15:
            return PERIPHS_IO_MUX_MTDO_U;

        default:
            return 0;
    }
}

int gpio_get_func(int gpio_no) {
    switch (gpio_no) {
        case 0:
            return FUNC_GPIO0;

        case 1:
            return FUNC_GPIO1;

        case 2:
            return FUNC_GPIO2;

        case 3:
            return FUNC_GPIO3;

        case 4:
            return FUNC_GPIO4;

        case 5:
            return FUNC_GPIO5;

        case 9:
            return FUNC_GPIO9;

        case 10:
            return FUNC_GPIO10;

        case 12:
            return FUNC_GPIO12;

        case 13:
            return FUNC_GPIO13;

        case 14:
            return FUNC_GPIO14;

        case 15:
            return FUNC_GPIO15;

        default:
            return 0;
    }
}

void gpio_select_func(int gpio_no) {
    /* Special treatment for GPIO16 */
    if (gpio_no == 16) {
        WRITE_PERI_REG(PAD_XPD_DCDC_CONF, (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xFFFFFFBC) | 0x1);
        WRITE_PERI_REG(RTC_GPIO_CONF, READ_PERI_REG(RTC_GPIO_CONF) & 0xFFFFFFFE);
        return;
    }

    int mux = gpio_get_mux(gpio_no);
    if (!mux) {
        return;
    }

    PIN_FUNC_SELECT(mux, gpio_get_func(gpio_no));

    /* GPIO{12..15} need to be configured together as GPIOs, or otherwise the output logical level of one pin may
     * vary when configuring another pin from this set. For example, GPIO15 becomes high if GPIO{12..14} is set as GPIO.
     */
    if (gpio_no >= 12 && gpio_no <= 15) {
        for (int i = 12; i <= 15; i++) {
            if (!(gpio_configured_state & BIT(i))) {
                PIN_FUNC_SELECT(gpio_get_mux(i), gpio_get_func(i));
            }
        }
    }
}

void gpio_set_pull(int gpio_no, bool pull) {
    if (gpio_no == 16) {
        uint32 val = READ_PERI_REG(RTC_GPIO_ENABLE);
        if (!pull) {
            val |= 0x8;
        }

        WRITE_PERI_REG(RTC_GPIO_ENABLE, val);
    }
    else {
        int mux = gpio_get_mux(gpio_no);
        if (!mux) {
            return;
        }

        if (pull) {
            PIN_PULLUP_EN(mux);
        }
        else {
            PIN_PULLUP_DIS(mux);
        }
    }
}

void gpio_configure_input(int gpio_no, bool pull) {
    gpio_select_func(gpio_no);

    if (gpio_no == 16) {
        WRITE_PERI_REG(RTC_GPIO_ENABLE, READ_PERI_REG(RTC_GPIO_ENABLE) & 0xFFFFFFFE);
    }
    else {
        GPIO_DIS_OUTPUT(gpio_no);
    }

    gpio_set_pull(gpio_no, pull);

    gpio_configured_state |= 1UL << gpio_no;
    gpio_output_state &= ~(1UL << gpio_no);
    if (pull) {
        gpio_pull_state |= 1UL << gpio_no;
    }
    else {
        gpio_pull_state &= ~(1UL << gpio_no);
    }
}

void gpio_configure_output(int gpio_no, bool initial) {
    gpio_select_func(gpio_no);

    if (gpio_no == 16) {
        WRITE_PERI_REG(RTC_GPIO_ENABLE, (READ_PERI_REG(RTC_GPIO_ENABLE) & 0xFFFFFFFE) | 0x1);

        uint32 val = READ_PERI_REG(RTC_GPIO_OUT);
        WRITE_PERI_REG(RTC_GPIO_OUT, initial ? (val | 0x1) : (val & ~0x1));
    }
    else {
        GPIO_OUTPUT_SET(gpio_no, initial);
    }

    gpio_configured_state |= 1UL << gpio_no;
    gpio_output_state |= 1UL << gpio_no;
}

bool gpio_is_configured(int gpio_no) {
    return !!(gpio_configured_state & (1UL << gpio_no));
}

bool gpio_is_output(int gpio_no) {
    return !!(gpio_output_state & (1UL << gpio_no));
}

bool gpio_get_pull(int gpio_no) {
    return !!(gpio_pull_state & (1UL << gpio_no));
}

void gpio_write_value(int gpio_no, bool value) {
    if (gpio_no == 16) {
        uint32 val = READ_PERI_REG(RTC_GPIO_OUT);
        WRITE_PERI_REG(RTC_GPIO_OUT, value ? (val | 0x1) : (val & ~0x1));
    }
    else {
        GPIO_OUTPUT_SET(gpio_no, !!value);
    }
}

bool gpio_read_value(int gpio_no) {
    if (gpio_no == 16) {
        return !!READ_PERI_REG(RTC_GPIO_IN_DATA);
    }
    else {
        return !!GPIO_INPUT_GET(gpio_no);
    }
}

void gpio_interrupt_handler_add(uint8 gpio_no, uint8 type, gpio_interrupt_handler_t handler, void *arg) {
    DEBUG_GPIO("adding interrupt handler on GPIO %d, type %d", gpio_no, type);

    intr_details = realloc(intr_details, sizeof(intr_details_t) * (intr_details_count + 1));
    intr_details_t *details = &intr_details[intr_details_count++];

    details->gpio_no = gpio_no;
    details->handler = handler;
    details->type = type;
    details->arg = arg;

    update_interrupts();
}

void gpio_interrupt_handler_remove(uint8 gpio_no, gpio_interrupt_handler_t handler) {
    DEBUG_GPIO("removing interrupt handler from GPIO %d", gpio_no);

    int8 pos = -1;
    for (uint8 i = 0; i < intr_details_count; i++) {
        intr_details_t *details = intr_details + i;
        if (details->handler == handler && details->gpio_no == gpio_no) {
            pos = i;
            break;
        }
    }

    if (pos < 0) { /* Not found */
        return;
    }

    for (uint8 i = pos; i < intr_details_count - 1; i++) {
        intr_details[i] = intr_details[i + 1];
    }

    if (--intr_details_count) {
        intr_details = realloc(intr_details, sizeof(intr_details_t) * intr_details_count);
    }
    else {
        free(intr_details);
        intr_details = NULL;
    }
}


void update_interrupts(void) {
    ETS_GPIO_INTR_DISABLE();

    if (!interrupt_handler_attached) {
        DEBUG_GPIO("attaching interrupt handler");

        ETS_GPIO_INTR_ATTACH(gpio_interrupt_handler, NULL);
        interrupt_handler_attached = TRUE;
    }

    DEBUG_GPIO("updating interrupts");

    uint32 rising_mask = 0;
    uint32 falling_mask = 0;

    for (int i = 0; i < intr_details_count; i++) {
        intr_details_t *details = intr_details + i;
        if (details->type == GPIO_INTERRUPT_RISING_EDGE || details->type == GPIO_INTERRUPT_ANY_EDGE) {
            rising_mask |= BIT(details->gpio_no);
        }
        else if (details->type == GPIO_INTERRUPT_FALLING_EDGE || details->type == GPIO_INTERRUPT_ANY_EDGE) {
            falling_mask |= BIT(details->gpio_no);
        }
    }

    for (int gpio_no = 0; gpio_no <= 16; gpio_no++) {
        if (rising_mask & BIT(gpio_no)) {
            if (falling_mask & BIT(gpio_no)) {
                DEBUG_GPIO("enabling rising-edge interrupt on GPIO %d", gpio_no);
                gpio_pin_intr_state_set(gpio_no, GPIO_PIN_INTR_POSEDGE);
            }
            else {
                DEBUG_GPIO("enabling any-edge interrupt on GPIO %d", gpio_no);
                gpio_pin_intr_state_set(gpio_no, GPIO_PIN_INTR_ANYEDGE);
            }
        }
        else {
            if (falling_mask & BIT(gpio_no)) {
                DEBUG_GPIO("enabling falling-edge interrupt on GPIO %d", gpio_no);
                gpio_pin_intr_state_set(gpio_no, GPIO_PIN_INTR_NEGEDGE);
            }
            else {
                gpio_pin_intr_state_set(gpio_no, GPIO_PIN_INTR_DISABLE);
            }
        }

        GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(gpio_no));
    }

    ETS_GPIO_INTR_ENABLE();
}

void gpio_interrupt_handler(void *arg) {
    /* Ignore other interrupts while handling this interrupt */
    ETS_GPIO_INTR_DISABLE();

    /* Determine which GPIOs have generated interrupt */
    uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

    /* Determine current GPIO values */
    uint32 gpio_values = gpio_input_get();

    /* Clear triggered interrupts */
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);

    for (int i = 0; i < intr_details_count; i++) {
        intr_details_t *details = intr_details + i;
        uint8 gpio_no = details->gpio_no;
        uint32 gpio_no_bit = BIT(gpio_no);
        if (!(gpio_status & gpio_no_bit)) {
            continue;
        }

        bool value = !!(gpio_values & BIT(gpio_no));
        if (details->type == GPIO_INTERRUPT_RISING_EDGE && !value) {
            continue;
        }
        if (details->type == GPIO_INTERRUPT_FALLING_EDGE && value) {
            continue;
        }

        details->handler(gpio_no, value, details->arg);
    }

    ETS_GPIO_INTR_ENABLE();
}
