#include "uart.h"

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "pico/binary_info.h"


void init_uart()
{
    bi_decl(bi_2pins_with_func(UART_TX_PIN, UART_RX_PIN, GPIO_FUNC_UART));
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    //uart_puts(UART_ID, " Hello, UART!\n");
}