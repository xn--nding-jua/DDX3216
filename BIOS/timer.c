/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#include "timer.h"

// **********************************************************
// timer functions
// **********************************************************
void timer_init() {
    outb(TIMER_CTRL, 0x36); // 8254 Mode 3 (Square Wave Generator), Binary, Counter 0
    
    // Teiler auf 0xFFFF setzen für ~18.2 Hz
    outb(TIMER0_DATA, 0xFF); // Low Byte
    outb(TIMER0_DATA, 0xFF); // High Byte
}

// **********************************************************
// RTC function
// **********************************************************

uint8_t read_rtc_raw(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

uint8_t read_rtc(uint8_t reg) {
	// wait if an update is running (bit 7 is set)
    while (read_rtc_raw(0x0A) & 0x80); 
    
    outb(0x70, reg);
    return inb(0x71);
}