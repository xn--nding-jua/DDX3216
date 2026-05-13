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
	write_sc300_cfg(0x1B,  (uint8_t)((VRAM_BASE >> 8) & 0xFF));
	write_sc300_cfg(0x1C, (uint8_t)((VRAM_BASE >> 16) & 0xFF));
	write_sc300_cfg(0x19, LCD_COLUMNS_BYTES - 1);
	write_sc300_cfg(0x1A, LCD_ROWS - 1);
	write_sc300_cfg(0x1E, 0x0D);
	write_sc300_cfg(0x18, 0x80); 
}

void lcd_clear() {
    uint8_t *vram = (uint8_t *)VRAM_BASE;
    for(int i = 0; i < VRAM_SIZE; i++) {
        vram[i] = 0x00; // Alles löschen
    }
}

// test the LCD and fill a simple pattern
void lcd_clear_test() {
    uint8_t* vram = (uint8_t*)VRAM_BASE;
    for (int i = 0; i < VRAM_SIZE; i++) {
        vram[i] = (i % 2) ? 0xAA : 0x55;
    }
}