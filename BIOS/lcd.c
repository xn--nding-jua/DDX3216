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
	// an LCD with 480x320 displays 60x40 chars
	// our LCD has 240x64 so 0.5 x 0.2 resulting in 30x8 chars
	
	// initialize LCD-controller in text-mode
	write_sc300_cfg(LCD_CGA_MODE_CTRL, 0b00111001); // blinking enabled / 1-bpp graphics / video enabled / colorburst enabled / textmode / normal-width char (8 pixels)
	write_sc300_cfg(LCD_CGA_CLR_SEL, 0x00); // default

	// setup LCD standard registers
	// ===========================================
	write_sc300_lcd_cfg(LCD_VID_IDX_HOR_TOTAL,         30);   // chars per row
	write_sc300_lcd_cfg(LCD_VID_IDX_HOR_DISP,          30);   // chars per row
	write_sc300_lcd_cfg(LCD_VID_IDX_HOR_SYNC_POS,      0x00); // according to page 3-26 of reference manual
	write_sc300_lcd_cfg(LCD_VID_IDX_HOR_SYNC_WIDTH,    0x00); // according to page 3-26 of reference manual

	write_sc300_lcd_cfg(LCD_VID_IDX_VER_TOTAL,         8);    // number of rows
	write_sc300_lcd_cfg(LCD_VID_IDX_VER_TOTAL_ADJ,     0x00); // according to page 3-27 of reference manual
	write_sc300_lcd_cfg(LCD_VID_IDX_VER_DISP,          8);    // number of rows 
	write_sc300_lcd_cfg(LCD_VID_IDX_VER_SYNC_POS,      8);    // number of rows
	
	write_sc300_lcd_cfg(LCD_VID_IDX_MAX_SCANLINE,      8-1);  // number of pixels per character -1

	write_sc300_lcd_cfg(LCD_VID_IDX_START_ADDR_UPPER,  (uint8_t)((VRAM_SEG >> 16) & 0xFF));
	write_sc300_lcd_cfg(LCD_VID_IDX_START_ADDR_LOWER,  (uint8_t)((VRAM_SEG >> 8) & 0xFF));

	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_UPPER, (uint8_t)((VRAM_SEG >> 16) & 0xFF));
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_LOWER, (uint8_t)((VRAM_SEG >> 8) & 0xFF));
	
	// setup LCD enhanced registers
	// ===========================================
	// enable enhanced registers
	outb(LCD_CGA_IDX_ADDR, LCD_ENH_IDX_SOFTW_SWITCH_EN);
	inb(LCD_CGA_IDX_DATA);

	// select LCD type
	// bit 7..6: panel-selection
	// bit 5   : reserved
	// bit 4   : display-type
	// bit 3   : CGA font
	// bit 2   : reserved
	// bit 1   : auto-blink
	// bit 0   : display-mode
	write_sc300_lcd_cfg(LCD_ENH_IDX_SCREEN_CTRL_REST,  0b01010000); // 4-bit small LCD / font-area 1 / CGA-mode
	write_sc300_lcd_cfg(LCD_ENH_IDX_SCREEN_CONTROL_2,  0b00000000); // enable display-controller (default)
	write_sc300_lcd_cfg(LCD_ENH_IDX_SCREEN_CONTROL_1,  0b00100000); // dot doubling disabled

	// disable enhanced registers
	outb(LCD_CGA_IDX_ADDR, LCD_ENH_IDX_SOFTW_SWITCH_DIS);
	inb(LCD_CGA_IDX_DATA);


	
//	write_sc300_cfg(0x19, LCD_COLUMNS_BYTES - 1);
//	write_sc300_cfg(0x1A, LCD_ROWS - 1);
//	write_sc300_cfg(0x1E, 0x0D);
//	write_sc300_cfg(0x18, 0x80); 
}

void lcd_clear() {
	// delete video-memory
	for (int i = 0; i < VRAM_SIZE; i++) {
		writeFarByte(VRAM_SEG, i, 0x00);
		writeFarByte(VRAM_SEG, i + 1, 0x00);
	}
}

// test the LCD and fill a simple pattern
void lcd_clear_test() {
	for (int i = 0; i < VRAM_SIZE; i++) {
		writeFarByte(VRAM_SEG, i, (i % 2) ? 0xAA : 0x55);
	}
}

// output single char on LCD
void lcd_putchar(int row, int col, char c, uint8_t attribute) {
    uint16_t offset = (row * 80 * 2) + (col * 2);
    
    writeFarByte(VRAM_SEG, offset,     c); // character-byte (8-bit ASCII-chars)
    writeFarByte(VRAM_SEG, offset + 1, attribute); // attribute-byte
	
	/*
	Attribute Byte:
	bit 7     -> Blink
	bit 6..4  -> Background Color (RGB)
	bit 3     -> Intensity
	bit 2..0  -> Foreground Color (RGB)
	*/
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
