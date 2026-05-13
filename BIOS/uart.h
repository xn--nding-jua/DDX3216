#ifndef UART_H_
#define UART_H_

#include "bios.h"

void uart_init(uint16_t baudrate);
void uart_putc(char c);
void uart_print(const char* str);
void uart_interrupt_enable();

#endif