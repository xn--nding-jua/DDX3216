/*
	Main-program for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com

	This C-Code is placed right after the bootsector during linking
	and the main-function is called by the bootloader-code
*/

#include <stdint.h>

#define COM1 0x3F8
#define COM_DLAB            0x80
#define COM_TXR             0       /*  Transmit register (WRITE) */
#define COM_RXR             0       /*  Receive register  (READ)  */
#define COM_IER             1       /*  Interrupt Enable          */
#define COM_IIR             2       /*  Interrupt ID              */
#define COM_FCR             2       /*  FIFO control              */
#define COM_LCR             3       /*  Line control              */
#define COM_MCR             4       /*  Modem control             */
#define COM_LSR             5       /*  Line Status               */
#define COM_MSR             6       /*  Modem Status              */
#define COM_DLH             0
#define COM_DLL             1

// write 8-bit data in 16-bit I/O-space
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// read 8-bit data in 16-bit I/O-space
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void print_char(char c) {
    asm volatile (
        "mov $0x0E, %%ah\n\t"  // select type-function in text-mode
        "mov %0, %%al\n\t"     // load char in AL
        "int $0x10\n\t"        // call BIOS-interrupt 10h
        :
        : "r"(c)
        : "eax"
    );
}

void print_string(const char* str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}

static void uart_init(int port, int baud)
{
	unsigned char c;
	unsigned divisor;

	outb(0x3, port + COM_LCR);	// set to 8N1
	outb(0, port + COM_IER);    // no interrupts
	outb(0, port + COM_FCR);    // no FIFO
	outb(0x3, port + COM_MCR);	// enable DTR and DTS

	divisor	= 115200 / baud;
	c = inb(port + COM_LCR);
	outb(c | COM_DLAB, port + COM_LCR);
	outb(divisor & 0xff, port + COM_DLL);
	outb((divisor >> 8) & 0xff, port + COM_DLH);
	outb(c & ~COM_DLAB, port + COM_LCR);

}

void uart_putchar(char c) {
    //while inb(COM1 + COM_FCR) & FR_TXFF);
    outb(COM1 + COM_TXR, c);
}

void uart_write(const char* data) {
    while (*data) {
        uart_putchar(*data++);
    }
}

__attribute__((noreturn)) void main() {
    print_string("Hello World from C-Code!");

	uart_init(COM1, 9600);
	uart_write("Hello World from C-Code via UART!\r\n");

    while(1) {
        asm volatile("hlt");
    }
}
