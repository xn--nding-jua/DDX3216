#ifndef UART_H_
#define UART_H_

#include "bios.h"

void uart_init(uint16_t baudrate);
void uart_putc(char c);
void uart_print_string(const char* str);
void uart_print_uint16(uint16_t value, bool asHex);
void uart_print_string_ram(const char* str);
void uart_interrupt_enable();
void uart_interrupt_disable();

#endif