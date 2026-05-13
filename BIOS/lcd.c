/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#include "lcd.h"

// **********************************************************
// LCD functions
// **********************************************************

// initialize the LCD with a 240x64 pixel resolution
void lcd_init() {
	write_sc300_cfg(0x1B, 0x00);
	write_sc300_cfg(0x1C, 0x00);
	write_sc300_cfg(0x19, 30 - 1);
	write_sc300_cfg(0x1A, 64 - 1);
	write_sc300_cfg(0x1E, 0x0D);
	write_sc300_cfg(0x18, 0x80); 
}

void lcd_clear() {
	// DS was set in assembler-part to 0x0000, so this pointer directs
	// to the begin of the DRAM, which is used as VRAM as well
    uint8_t *vram = (uint8_t *)0x0000;
    for(int i = 0; i < (30 * 64); i++) {
        vram[i] = 0x00; // Alles löschen
    }
}

// test the LCD and fill a simple pattern
void lcd_clear_test() {
    uint8_t* vram = (uint8_t*)0x0000;
    for (int i = 0; i < (30 * 64); i++) {
        vram[i] = (i % 2) ? 0xAA : 0x55;
    }
}