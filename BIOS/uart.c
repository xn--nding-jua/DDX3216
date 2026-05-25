// **********************************************************
// UART functions
// **********************************************************

#include "uart.h"

void uart_init(uint16_t baudrate) {
    // enable AFDT# to output 14.336 MHz-clock 
    write_sc300_cfg(0xBA, 0b00001000);

	// set baudrate by setting divisor
    outb(UART_LCR, 0x80); //  enable access to divisor latches by setting DLAB-bit
	uint16_t divisor = (UART_CLK / (16 * baudrate));
    outb(UART_DLL, (uint8_t)(divisor & 0xFF));
    outb(UART_DLM, (uint8_t)((divisor >> 8) & 0xFF));
	
    outb(UART_LCR, 0x03); // reset DLAB-bit and set 8N1 mode
    outb(UART_FCR, 0x07); // enable FIFO and clear them
	outb(UART_MCR, 0x03); // set DTR and RTS within modem control register
}

void uart_putc(char c) {
    while (!(inb(UART_LSR) & 0x20));
    outb(UART_THR, (uint8_t)c);
}

void uart_print(const char* str) {
    while (*str) {
        if (*str == '\n') {
            uart_putc('\r');
        }
        uart_putc(*str++);
    }
}

void uart_interrupt_enable() {
	// enable receiver data available interrupt
	outb(UART_IER, 0x01);
	
	// set bit 2 in modem control register
	uint8_t mcr = inb(UART_MCR);
    outb(UART_MCR, mcr | 0x08);
	
	// update PIC mask and enable IRQ4
	uint8_t mask = inb(0x21);
    outb(0x21, mask & ~(1 << 4));
}