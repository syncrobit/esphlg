
#include <osapi.h>
#include <user_interface.h>

#include "espgoodies/drivers/uart.h"

#include "espgoodies/debug.h"


static int8 debug_uart_no;


static void ICACHE_FLASH_ATTR debug_putc_func(char c);


void debug_uart_setup(int8 uart_no) {
    debug_uart_no = uart_no;

    if (uart_no >= 0) {
        uart_setup(uart_no, DEBUG_BAUD, UART_PARITY_NONE, UART_STOP_BITS_1);
        os_install_putc1(debug_putc_func);
        system_set_os_print(1);
    }
    else {
        system_set_os_print(0);
    }
}

int8 debug_uart_get_no(void) {
    return debug_uart_no;
}

void debug_putc_func(char c) {
    uart_write_char(debug_uart_no, c);
}
