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
    
    // AT-compatible timer (8254) clocks at 1.19318 MHz
    // but the SC300 timer clocks at 1.1892 MHz
    // so we choose the divider to be 0xFF23 instead of 0xFFFF for ~18.207 Hz
    outb(TIMER0_DATA, 0x23); // Low Byte
    outb(TIMER0_DATA, 0xFF); // High Byte
}

// **********************************************************
// RTC function
// **********************************************************

uint8_t read_rtc(uint8_t reg) {
	// wait if an update is running (bit 7 is set)
    while (read_sc300_rtc_cfg(0x0A) & 0x80); 
	return read_sc300_rtc_cfg(reg);
}