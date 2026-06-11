// **********************************************************
// UART functions
// **********************************************************

#include "uart.h"

void uart_init(uint16_t baudrate) {
    // enable AFDT# to output 14.336 MHz-clock 
    write_sc300_cfg(0xBA, 0b00001000);
    delay_1ms();
    
	// set baudrate by setting divisor
    outb(UART_LCR, 0x80); //  enable access to divisor latches by setting DLAB-bit
	uint16_t divisor = (UART_CLK / (16 * baudrate));
    outb(UART_DLL, (uint8_t)(divisor & 0xFF));
    outb(UART_DLM, (uint8_t)((divisor >> 8) & 0xFF));
	
    outb(UART_LCR, 0x03); // reset DLAB-bit and set 8N1 mode
    outb(UART_IER, 0x00); // disable all interrupts
    //outb(UART_FCR, 0x07); // enable FIFO and clear them
    outb(UART_FCR, 0x00); // disable FIFO
	outb(UART_MCR, 0x03); // set DTR and RTS within modem control register
	
	// enable RS232 in/out by asserting SLIN#
	outb(LPT_WRITE_CONTROL, DDX3216_RS232_EN);
}

void uart_putc(char c) {
    while (!(inb(UART_LSR) & 0x20));
    outb(UART_THR, (uint8_t)c);
}

void uart_print_string(const char* str) {
	// Strings are placed in ROM so we have to take care of different memory-segments
	// so we take the relative offset and use the readRomByte function
    uint16_t rom_offset = (uint16_t)(uintptr_t)str;

    while (1) {
        char c = (char)readRomByte(rom_offset);
        
        if (c == '\0') {
            break;
        }

        uart_putc(c);

        // get next character
        rom_offset++;
    }
}

void uart_print_uint16(uint16_t value, bool asHex) {
	char textbuffer[6];
	if (asHex) {
		uint16_to_hex(value, textbuffer); // needs only 5 chars
	} else {
		uint16_to_dec(value, textbuffer); // needs 6 chars
	}
	uart_print_string_ram(textbuffer);
}

void uart_print_string_ram(const char* str) {
    while (*str) {
        uart_putc(*str);
        str++;
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
