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
	write_sc300_cfg(0x1B,  (uint8_t)((VRAM_SEG >> 8) & 0xFF));
	write_sc300_cfg(0x1C, (uint8_t)((VRAM_SEG >> 16) & 0xFF));
	write_sc300_cfg(0x19, LCD_COLUMNS_BYTES - 1);
	write_sc300_cfg(0x1A, LCD_ROWS - 1);
	write_sc300_cfg(0x1E, 0x0D);
	write_sc300_cfg(0x18, 0x80); 
}

void lcd_clear() {
	// delete video-memory
	for (int i = 0; i < VRAM_SIZE; i++) {
		writeFarByte(VRAM_SEG, i, 0x00); // TODO: check if we need a 2-byte access like in CGA here
	}
}

// test the LCD and fill a simple pattern
void lcd_clear_test() {
	for (int i = 0; i < VRAM_SIZE; i++) {
		writeFarByte(VRAM_SEG, i, (i % 2) ? 0xAA : 0x55); // TODO: check if we need a 2-byte access like in CGA here
	}
}

// output single char on LCD
void lcd_putchar(int row, int col, char c, uint8_t attribute) {
    uint16_t offset = (row * 80 * 2) + (col * 2);
    
    writeFarByte(VRAM_SEG, offset,     c);
    writeFarByte(VRAM_SEG, offset + 1, attribute);
}

// output whole string on LCD
void lcd_print_string(int row, int col, const char *str, uint8_t attribute) {
	int current_col = col;
    
    while (*str) {
        lcd_putchar(row, current_col, *str, attribute);
        
        str++;
        current_col++;
        
        if (current_col >= 80) {
            current_col = 0;
            row++;
        }
    }
}
