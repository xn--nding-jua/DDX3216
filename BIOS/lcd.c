/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#include "lcd.h"

// **********************************************************
// LCD functions
// **********************************************************

// initialize the LCD
// the DDX3216 has a resolution of 240x64 but we are using a virtual
// resolution of 320x200 (CGA)
void lcd_init() {
	// initialize LCD-controller in text-mode according to AMD white paper 20749A

	// enable enhanced registers
	outb(LCD_CGA_IDX_ADDR, LCD_ENH_IDX_SOFTW_SWITCH_EN);
	inb(LCD_CGA_IDX_DATA); // read without I/O-cycle in between

	// select LCD type
	// bit 7..6: panel-selection
	// bit 5   : reserved
	// bit 4   : display-type
	// bit 3   : CGA font
	// bit 2   : reserved
	// bit 1   : auto-blink
	// bit 0   : display-mode
	write_sc300_lcd_cfg(LCD_ENH_IDX_SCREEN_CTRL_RESTORE,  0b01010000); // 4-bit single-screen small LCD / font-area 1 / CGA-mode

	// setup LCD standard registers
	// ===========================================
	write_sc300_lcd_cfg(LCD_VID_IDX_HOR_TOTAL,         (LCD_WIDTH * LCD_BPP) / 8);   // chars per row (virtual CGA resolution)
	write_sc300_lcd_cfg(LCD_VID_IDX_HOR_DISP,          (LCD_WIDTH * LCD_BPP) / 8);   // chars per row (virtual CGA resolution)
	write_sc300_lcd_cfg(LCD_VID_IDX_HOR_SYNC_POS,      0x00); // according to page 3-26 of reference manual
	write_sc300_lcd_cfg(LCD_VID_IDX_HOR_SYNC_WIDTH,    0x00); // according to page 3-26 of reference manual

	write_sc300_lcd_cfg(LCD_VID_IDX_VER_TOTAL,         LCD_HEIGHT / 8);    // number of rows (virtual CGA resolution)
	write_sc300_lcd_cfg(LCD_VID_IDX_VER_TOTAL_ADJ,     0x00); // according to page 3-27 of reference manual
	write_sc300_lcd_cfg(LCD_VID_IDX_VER_DISP,          LCD_HEIGHT / 8);    // number of rows (virtual CGA resolution)
	write_sc300_lcd_cfg(LCD_VID_IDX_VER_SYNC_POS,      LCD_HEIGHT / 8);    // number of rows (virtual CGA resolution)
	
	write_sc300_lcd_cfg(LCD_VID_IDX_INTERLACED_MODE,   0);
	write_sc300_lcd_cfg(LCD_VID_IDX_MAX_SCANLINE,      8-1);  // number of pixels per character -1
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_START,      6-1); // last two pixel-rows of a font as cursor
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_END,        7-1); // last two pixel-rows of a font as cursor
	write_sc300_lcd_cfg(LCD_VID_IDX_START_ADDR_UPPER,  0); // video-memory starts directly after boot-sector at 0xB8000
	write_sc300_lcd_cfg(LCD_VID_IDX_START_ADDR_LOWER,  0); // video-memory starts directly after boot-sector at 0xB8000
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_UPPER, 0); // display cursor in the upper left corner
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_LOWER, 0); // display cursor in the upper left corner

	write_sc300_lcd_cfg(LCD_ENH_IDX_SCREEN_CONTROL_2,  0b00000000); // enable display-controller (default)
	write_sc300_lcd_cfg(LCD_ENH_IDX_SCREEN_CONTROL_1,  0b00100000); // dot doubling disabled

	write_sc300_cfg(LCD_CGA_MODE_CTRL, 0b00111001); // blinking enabled / 1-bpp graphics / video enabled / colorburst enabled / textmode / normal-width char (8 pixels)
	write_sc300_cfg(LCD_CGA_CLR_SEL, 0x00); // default

	// disable enhanced registers
	outb(LCD_CGA_IDX_ADDR, LCD_ENH_IDX_SOFTW_SWITCH_DIS);
	inb(LCD_CGA_IDX_DATA); // read without I/O-cycle in between
}

void lcd_clear() {
	// delete video-memory
	for (int i = 0; i < VRAM_SIZE; i++) {
		writeFarByte(VRAM_SEG, i, 0x00);
	}
}

// test the LCD and fill a simple pattern
void lcd_clear_test() {
	for (int i = 0; i < VRAM_SIZE; i++) {
		writeFarByte(VRAM_SEG, i, (i % 2) ? 0xAA : 0x55);
	}
}

// scroll up by one row and clear the last row with spaces
void lcd_scroll_up() {
    // copy rows 1-8 by one row (30 chars * 2 Bytes = 60 Bytes) one up
    for (int r = 1; r < (LCD_HEIGHT / 8); r++) {
        for (int c = 0; c < (LCD_WIDTH / 8); c++) {
            uint16_t src_offset = (r * (LCD_WIDTH / 8) * 2) + (c * 2);
            uint16_t dst_offset = ((r - 1) * (LCD_WIDTH / 8) * 2) + (c * 2);
            
            writeFarWord(VRAM_SEG, dst_offset, readFarByte(VRAM_SEG, src_offset)); // character-byte (8-bit ASCII-chars)
            writeFarWord(VRAM_SEG, dst_offset + 1, readFarByte(VRAM_SEG, src_offset + 1)); // attribute-byte
        }
    }

    // clear last row with spaces
    for (int c = 0; c < (LCD_WIDTH / 8); c++) {
        uint16_t offset = ((LCD_HEIGHT / 8 - 1) * (LCD_WIDTH / 8) * 2) + (c * 2);
        writeFarByte(VRAM_SEG, offset, ' ');
        writeFarByte(VRAM_SEG, offset + 1, 0x07); // light gray on black
    }

	// update internal cursor position
	bda->cursor_position[0].row = (LCD_HEIGHT / 8) - 1;
	bda->cursor_position[0].col = 0;

	// update hardware cursor position
	uint16_t offset = ((bda->cursor_position[0].row * (LCD_WIDTH / 8)) + bda->cursor_position[0].col) * 2;
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_UPPER, (offset >> 9) & 0xFF); // upper 7 bits of offset (divide by 512)
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_LOWER, (offset >> 1) & 0xFF); // lower 8 bits of offset (divide by 2)
}

// output single char on LCD at specific position
void lcd_putc_pos(int row, int col, char c, uint8_t attribute) {
	// 0xB0000 is the upper left character
	// in text-mode the LCD uses 2-bytes to display a single character: the first byte is the ASCII code of the character, the second byte is the attribute (color, blinking, etc.)
	// the offset within video-memory can be calculated as follows:
	uint16_t offset = ((row * (LCD_WIDTH / 8)) + col) * 2;
    
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

void lcd_putc(char c, uint8_t attribute) {
	uint16_t offset;

	switch (c) {
		case '\n': // newline
			bda->cursor_position[0].col = 0;
			bda->cursor_position[0].row++;

			if (bda->cursor_position[0].row == (LCD_HEIGHT / 8)) {
				// reset to top of screen when we reach the end of the screen
				lcd_scroll_up();
			}
			break;
		case '\r': // carriage return
			bda->cursor_position[0].col = 0;
			break;
		case '\b': // backspace
			// delete char at current position
			if (bda->cursor_position[0].col > 0) {
				bda->cursor_position[0].col--;
			}
			offset = ((bda->cursor_position[0].row * (LCD_WIDTH / 8)) + bda->cursor_position[0].col) * 2;
			writeFarByte(VRAM_SEG, offset, ' '); // character-byte (8-bit ASCII-chars)
			writeFarByte(VRAM_SEG, offset + 1, attribute); // attribute-byte
			break;
		default:
			// output character to LCD

			// increment internal cursor position for new character
			if (bda->cursor_position[0].col < (LCD_WIDTH / 8) - 1) {
				bda->cursor_position[0].col++;
			}else{
				bda->cursor_position[0].col = 0;
				bda->cursor_position[0].row++;
			}
			if (bda->cursor_position[0].row == (LCD_HEIGHT / 8)) {
				lcd_scroll_up();
			}
			offset = ((bda->cursor_position[0].row * (LCD_WIDTH / 8)) + bda->cursor_position[0].col) * 2;
			writeFarByte(VRAM_SEG, offset, c); // character-byte (8-bit ASCII-chars)
			writeFarByte(VRAM_SEG, offset + 1, attribute); // attribute-byte
			break;
	}

	// update hardware cursor position
	offset = ((bda->cursor_position[0].row * (LCD_WIDTH / 8)) + bda->cursor_position[0].col) * 2;
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_UPPER, (offset >> 9) & 0xFF); // upper 7 bits of offset (divide by 512)
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_LOWER, (offset >> 1) & 0xFF); // lower 8 bits of offset (divide by 2)
}

// output whole string on LCD
void lcd_print_string(int row, int col, const char *str, uint8_t attribute) {
	int current_col = col;
    
    while (*str) {
        lcd_putc_pos(row, current_col, *str, attribute);
        
        str++;
        current_col++;
        
        if (current_col >= (LCD_WIDTH / 8)) {
            current_col = 0;
            row++;
        }
    }
}
