#pragma once

#include <cstdint>

#define UART_ID uart0
#define BAUD_RATE 9600
#define UART_TX_PIN 16
#define UART_RX_PIN 17


void init_uart();

bool uart_rx_available();
uint8_t uart_rx_get();