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

bool textmode;

void lcd_init() {
	// initialize LCD-controller in text-mode according to AMD white paper 20749A or graphics-mode if desired

	// enable enhanced registers
	outb(LCD_CGA_IDX_ADDR, LCD_ENH_IDX_SOFTW_SWITCH_EN);
	inb(LCD_CGA_IDX_DATA); // read without I/O-cycle in between

	// select LCD type
	// bit 7..6: panel-selection
	// bit 5   : reserved
	// bit 4   : display-type
	// bit 3   : CGA font
	// bit 2   : reserved
	// bit 1   : auto-blanking
	// bit 0   : display-mode
	write_sc300_lcd_cfg(LCD_ENH_IDX_SCREEN_CTRL_RESTORE,  0b01010000); // 4-bit single-screen small LCD / font-area 1 / CGA-mode

	if (textmode) {
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

		write_sc300_lcd_cfg(LCD_VID_IDX_INTERLACED_MODE,   0); // not used in LCD mode
		write_sc300_lcd_cfg(LCD_VID_IDX_MAX_SCANLINE,      8-1); // number of pixels per character -1
		write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_START,      0b01000000 + 6-1); // enable 16*refreshrate / last two pixel-rows of a font as cursor
		write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_END,        7-1); // last two pixel-rows of a font as cursor
		write_sc300_lcd_cfg(LCD_VID_IDX_START_ADDR_UPPER,  0); // video-memory starts directly after boot-sector at 0xB8000
		write_sc300_lcd_cfg(LCD_VID_IDX_START_ADDR_LOWER,  0); // video-memory starts directly after boot-sector at 0xB8000
		write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_UPPER, 0); // display cursor in the upper left corner
		write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_LOWER, 0); // display cursor in the upper left corner

		write_sc300_lcd_cfg(LCD_ENH_IDX_SCREEN_CONTROL_2,  0b00000000); // enable display-controller (default)
		write_sc300_lcd_cfg(LCD_ENH_IDX_SCREEN_CONTROL_1,  0b00100000); // dot doubling disabled

		outb(LCD_CGA_MODE_CTRL, 0b00111101); // blinking enabled / 1-bpp graphics / video enabled / colorburst enabled / textmode / normal-width char (8 pixels)
		outb(LCD_CGA_CLR_SEL, 0x00); // default
	}else{
		// setup LCD standard registers
		// ===========================================
		write_sc300_lcd_cfg(LCD_VID_IDX_HOR_TOTAL,         (640 * LCD_BPP) / 16);   // pixels per row divided by 16 (see page 3-26 in PRM)
		write_sc300_lcd_cfg(LCD_VID_IDX_HOR_DISP,          (640 * LCD_BPP) / 16);   // same value as 0x00
		write_sc300_lcd_cfg(LCD_VID_IDX_HOR_SYNC_POS,      0x00); // according to page 3-26 of reference manual
		write_sc300_lcd_cfg(LCD_VID_IDX_HOR_SYNC_WIDTH,    0x00); // according to page 3-26 of reference manual

		write_sc300_lcd_cfg(LCD_VID_IDX_VER_TOTAL,         200 / 2);    // number of pixels per row divided by number of pages (see page 3-27 in PRM)
		write_sc300_lcd_cfg(LCD_VID_IDX_VER_TOTAL_ADJ,     0x00); // according to page 3-27 of reference manual
		write_sc300_lcd_cfg(LCD_VID_IDX_VER_DISP,          200 / 2);    // same value as 0x04
		write_sc300_lcd_cfg(LCD_VID_IDX_VER_SYNC_POS,      200 / 2);    // same value as 0x04

		write_sc300_lcd_cfg(LCD_VID_IDX_INTERLACED_MODE,   0);
		write_sc300_lcd_cfg(LCD_VID_IDX_MAX_SCANLINE,      2-1); // number of banks used for graphics-mode (here 2 pages of 8kB are used)
		write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_START,      0); // last two pixel-rows of a font as cursor
		write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_END,        0); // last two pixel-rows of a font as cursor
		write_sc300_lcd_cfg(LCD_VID_IDX_START_ADDR_UPPER,  0); // video-memory starts directly after boot-sector at 0xB8000
		write_sc300_lcd_cfg(LCD_VID_IDX_START_ADDR_LOWER,  0); // video-memory starts directly after boot-sector at 0xB8000
		write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_UPPER, 0); // display cursor in the upper left corner
		write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_LOWER, 0); // display cursor in the upper left corner

		write_sc300_lcd_cfg(LCD_ENH_IDX_SCREEN_CONTROL_2,  0b00000000); // enable display-controller (default)
		write_sc300_lcd_cfg(LCD_ENH_IDX_SCREEN_CONTROL_1,  0b00100000); // dot doubling disabled

		outb(LCD_CGA_MODE_CTRL, 0b00011010);
		outb(LCD_CGA_CLR_SEL, 0x0F); // default
	}

	// disable enhanced registers
	outb(LCD_CGA_IDX_ADDR, LCD_ENH_IDX_SOFTW_SWITCH_DIS);
	inb(LCD_CGA_IDX_DATA); // read without I/O-cycle in between
}

void lcd_clear() {
	// delete video-memory
	if (textmode) {
		for (int i = 0; i < VRAM_SIZE_TEXT; i++) {
			writeFarByte(VRAM_SEG, i, 0x00);
		}
	}else{
		/*
		for (int i = 0; i < VRAM_SIZE_GRAPHICS_PER_BANK; i++) {
			writeFarByte(VRAM_SEG, i, 0x00); // page 0 for even-numbered pixel-rows
			writeFarByte(VRAM_SEG, 0x0200 + i, 0x00); // page 1 for odd-numbered pixel-rows
		}
		*/
		uint8_t value = 0;
		for (uint16_t y = 0; y < 200/2; y++) {
			for (uint16_t x = 0; x < (640/8); x++) {
				writeFarByte(VRAM_SEG, (y * (uint16_t)0x50) + x, value); 		   					// page 0 for even-numbered pixel-rows
				writeFarByte(VRAM_SEG, (uint16_t)0x2000 + (y * (uint16_t)0x50) + x, 255 - value);	// page 1 for odd-numbered pixel-rows
				value++;
			}
		}
	}
}

// test the LCD and fill a simple pattern
void lcd_clear_test() {
	for (int i = 0; i < VRAM_SIZE_TEXT; i++) {
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
            
			writeFarWord(VRAM_SEG, dst_offset, readFarWord(VRAM_SEG, src_offset)); // character-byte + attribute-byte (16-bit)
        }
    }

    // clear last row with spaces
    for (int c = 0; c < (LCD_WIDTH / 8); c++) {
        uint16_t offset = ((LCD_HEIGHT / 8 - 1) * (LCD_WIDTH / 8) * 2) + (c * 2);
        writeFarByte(VRAM_SEG, offset, ' ');
        writeFarByte(VRAM_SEG, offset + 1, 0x07); // light gray on black
    }

	// update internal cursor position
	writeFarByte(BASE_SEG, BDA_CURSOR_POS_ROW, (LCD_HEIGHT / 8) - 1);
	writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, 0);

	// update hardware cursor position
	uint16_t offset = ((readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) * (LCD_WIDTH / 8)) + readFarByte(BASE_SEG, BDA_CURSOR_POS_COL)) * 2;
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_UPPER, (offset >> 9) & 0xFF); // upper 7 bits of offset (divide by 512)
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_LOWER, (offset >> 1) & 0xFF); // lower 8 bits of offset (divide by 2)
}

void lcd_putc(char c, uint8_t attribute) {
	uint16_t offset;

	switch (c) {
		case '\n': // newline
			writeFarByte(BASE_SEG, BDA_CURSOR_POS_ROW, readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) + 1); // increment row
			writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, 0); // reset column

			if (readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) == (LCD_HEIGHT / 8)) {
				// reset to top of screen when we reach the end of the screen
				lcd_scroll_up();
			}
			break;
		case '\r': // carriage return
			writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, 0);
			break;
		case '\b': // backspace
			// delete char at current position
			if (readFarByte(BASE_SEG, BDA_CURSOR_POS_COL) > 0) {
				writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, readFarByte(BASE_SEG, BDA_CURSOR_POS_COL) - 1);
			}
			offset = ((readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) * (LCD_WIDTH / 8)) + readFarByte(BASE_SEG, BDA_CURSOR_POS_COL)) * 2;
			writeFarByte(VRAM_SEG, offset, ' '); // character-byte (8-bit ASCII-chars)
			writeFarByte(VRAM_SEG, offset + 1, attribute); // attribute-byte
			break;
		default:
			// output character to LCD
			offset = ((readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) * (LCD_WIDTH / 8)) + readFarByte(BASE_SEG, BDA_CURSOR_POS_COL)) * 2;
			writeFarByte(VRAM_SEG, offset, c); // character-byte (8-bit ASCII-chars)
			writeFarByte(VRAM_SEG, offset + 1, attribute); // attribute-byte

			// increment internal cursor position for next character
			if (readFarByte(BASE_SEG, BDA_CURSOR_POS_COL) < (LCD_WIDTH / 8) - 1) {
				writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, readFarByte(BASE_SEG, BDA_CURSOR_POS_COL) + 1);
			}else{
				writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, 0);
				writeFarByte(BASE_SEG, BDA_CURSOR_POS_ROW, readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) + 1);
			}
			if (readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) == (LCD_HEIGHT / 8)) {
				lcd_scroll_up();
			}

			break;
	}

	// update hardware cursor position
	//uint16_t char_addr = (bda->cursor_position[0].row * (LCD_WIDTH / 8)) + bda->cursor_position[0].col;
	//write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_UPPER, (char_addr >> 8) & 0xFF);
	//write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_LOWER,  char_addr & 0xFF);

	offset = ((readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) * (LCD_WIDTH / 8)) + readFarByte(BASE_SEG, BDA_CURSOR_POS_COL)) * 2;
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_UPPER, (offset >> 9) & 0xFF); // upper 7 bits of offset (divide by 512)
	write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_LOWER, (offset >> 1) & 0xFF); // lower 8 bits of offset (divide by 2)
}

// output single char on LCD at specific position
void lcd_putc_pos(int row, int col, char c, uint8_t attribute) {
	// set cursor at desired position
	writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, col);
	writeFarByte(BASE_SEG, BDA_CURSOR_POS_ROW, row);

	// output char at current cursor position
	lcd_putc(c, attribute);
}

// output whole string on LCD at current position
void lcd_print_string(const char* str, uint8_t attribute) {
	// Strings are placed in ROM so we have to take care of different memory-segments
	// so we take the relative offset and use the readRomByte function
    uint16_t rom_offset = (uint16_t)(uintptr_t)str;

    while (1) {
        char c = (char)readRomByte(rom_offset++);
        
        if (c == '\0') {
            break;
        }

		lcd_putc(c, attribute);
    }
}

void lcd_print_uint16(uint16_t value, bool asHex) {
	char textbuffer[6];
	if (asHex) {
		uint16_to_hex(value, textbuffer); // needs only 5 chars
	} else {
		uint16_to_dec(value, textbuffer); // needs 6 chars
	}
	lcd_print_string_ram(textbuffer, 0x07);
}

// output whole string on LCD at specific position
void lcd_print_string_pos(int row, int col, const char* str, uint8_t attribute) {
	// set cursor at desired position
	writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, col);
	writeFarByte(BASE_SEG, BDA_CURSOR_POS_ROW, row);

	// output whole string on LCD at specific position
	lcd_print_string(str, attribute);
}

void lcd_print_string_ram(const char* str, uint8_t attribute) {
    while (*str) {
        lcd_putc(*str, attribute);
        str++;
	}
}

void lcd_print_string_ram_pos(int row, int col, const char* str, uint8_t attribute) {
	// set cursor at desired position
	writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, col);
	writeFarByte(BASE_SEG, BDA_CURSOR_POS_ROW, row);

	// output whole string on LCD at specific position
	lcd_print_string_ram(str, attribute);
}

void lcd_install_character(uint8_t c, uint8_t row, uint8_t value) {
	writeFarByte(VRAM_SEG, 0x7800 + (c * 8) + row, value);
}

void lcd_install_font() {
	uint16_t vram_offset_base = 0x7800;
	for (uint16_t c = 0; c < 256; c++) {
		uint16_t char_vram_offset = vram_offset_base + (c * 8);
		
		for (uint8_t row = 0; row < 8; row++) {
			uint8_t row_data = readRomByte((uint16_t)(uintptr_t)&bios_font_8x8[c][row]);
			writeFarByte(VRAM_SEG, char_vram_offset + row, row_data);
		}
	}
}

void lcd_draw_double_box(uint8_t row, uint8_t col, uint8_t w, uint8_t h) {
    // corners
    lcd_putc_pos(row,     col,     0xC9, 0x07); // ╔
    lcd_putc_pos(row,     col+w-1, 0xBB, 0x07); // ╗
    lcd_putc_pos(row+h-1, col,     0xC8, 0x07); // ╚
    lcd_putc_pos(row+h-1, col+w-1, 0xBC, 0x07); // ╝

    // horizontal lines
    for (uint8_t i = 1; i < w-1; i++) {
        lcd_putc_pos(row, col+i,     0xCD, 0x07); // ═
        lcd_putc_pos(row+h-1, col+i, 0xCD, 0x07); // ═
    }

    // vertical lines
    for (uint8_t i = 1; i < h-1; i++) {
        lcd_putc_pos(row+i, col,     0xBA, 0x07); // ║
        lcd_putc_pos(row+i, col+w-1, 0xBA, 0x07); // ║
    }
}

void lcd_draw_pixel(uint16_t x, uint16_t y, uint8_t color) {
	if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)) {
        return;
    }

	// put even rows to first bank, odd rows to second bank
	uint16_t bank_offset = ((y % 2) == 0) ? 0x0000 : 0x2000;

	// memory-offset within the specific bank
	uint16_t offset = bank_offset + ((y / 2) * VRAM_BYTES_PER_ROW) + (x / 8);

	// calculate bit-offset in specific byte
	uint8_t bitoffset = 7 - (x % 8);

	// read value from RAM, set specific pixel and write value back to RAM
	uint8_t value  = readFarByte(VRAM_SEG, offset);
	if (color > 0x0F) {
		// set pixel
		value |= (1 << bitoffset);
	}else{
		// clear pixel
		value &= ~(1 << bitoffset);
	}
	writeFarByte(VRAM_SEG, offset, value);
}

uint8_t lcd_read_pixel(uint16_t x, uint16_t y) {
	/*
	if ((x >= 240) || (y >= 64)) {
        return 0;
    }

	// put even rows to first bank, odd rows to second bank
	uint16_t bank_offset = ((y % 2) == 0) ? 0x0000 : 0x2000;

	// memory-offset within the specific bank
	uint16_t offset = bank_offset + ((y / 2) * VRAM_BYTES_PER_ROW) + (x / 8);

	// calculate bit-offset in specific byte
	uint8_t bitoffset = 7 - (x % 8);

	// read value from RAM, set specific pixel and write value back to RAM
	uint8_t value  = readFarByte(VRAM_SEG, offset);

	uint8_t pixel_state = (value >> bitoffset) & 1;

	return pixel_state;
	*/
}
