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
    // control word has 8 bits
    // SC1:0 | RW1:0 | M2:0 | BCD
    // we select counter 0
    // we select read/write LSB first, than MSB
    // we select mode 3 = 
    // we set 16-bit binary counter
    outb(TIMER_CTRL, 0b00110110); // 8254 Mode 3 (Square Wave Generator), Binary, Counter 0
    
    // AT-compatible timer (8254) clocks at 1.19318 MHz
    // but the SC300 timer clocks at 1.1892 MHz
    // so we choose the divider to be 0xFF23 instead of 0xFFFF for ~18.207 Hz
    outb(TIMER0_DATA, 0x23); // write LSB first
    outb(TIMER0_DATA, 0xFF); // write MSB second
}

// **********************************************************
// RTC function
// **********************************************************

uint8_t read_rtc(uint8_t reg) {
	// wait if an update is running (bit 7 is set)
    while (read_sc300_rtc_cfg(0x0A) & 0x80); 
	return read_sc300_rtc_cfg(reg);
}

void delay_1s_hardware(void) {
    // BIOS hardwaretimer ticks every 54.93ms. For 1 second we have to wait at least 18 cycles = 988,74 ms
    uint16_t start_ticks = readFarWord(0x0000, 0x046C);
    
    while (1) {
        uint16_t current_ticks = readFarWord(0x0000, 0x046C);
        uint16_t elapsed = current_ticks - start_ticks;
        if (elapsed >= 18) {
            break; // after one second
        }
        
        __asm__ volatile("nop"); 
    }
}
