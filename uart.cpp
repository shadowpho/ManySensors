#include "uart.h"

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "pico/binary_info.h"

#include "hardware/irq.h"

#include <atomic>


//32b every 200ms => 5x tx a second gives us 1s grace to handle it
#define RX_BUF_SIZE 160
volatile uint8_t rx_buf[RX_BUF_SIZE];
volatile std::atomic<uint16_t> rx_head = 0;
volatile std::atomic<uint16_t> rx_tail = 0;
volatile std::atomic<uint16_t> overflow_counter = 0;

void on_uart_rx() 
{
    while (uart_is_readable(UART_ID)) 
    {
        uint8_t ch = uart_getc(UART_ID);
        uint16_t next_head = (rx_head + 1) % RX_BUF_SIZE;
        
        if (next_head != rx_tail) 
        {
            rx_buf[rx_head] = ch;
            rx_head = next_head;
        }
        else
            overflow_counter++;// Drop byte if buffer full
    }
}

// Check if data available
bool uart_rx_available() 
{
    return rx_head != rx_tail;
}

// Get one byte from ring buffer
uint8_t uart_rx_get() 
{
    uint8_t ch = 0;

    if (rx_head != rx_tail) {
        ch = rx_buf[rx_tail];
        rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    }

    return ch;
}

void init_uart()
{
    bi_decl(bi_2pins_with_func(UART_TX_PIN, UART_RX_PIN, GPIO_FUNC_UART));
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(UART_ID, true);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    int uart_irq = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(uart_irq, on_uart_rx);
    irq_set_enabled(uart_irq, true);
    uart_set_irq_enables(UART_ID, true, false);
}