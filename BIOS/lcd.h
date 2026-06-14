/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef LCD_H_
#define LCD_H_

#include "bios.h"
#include "fonts.h"

bool textmode;

void lcd_init();
void lcd_clear();
void lcd_clear_test();
void lcd_scroll_up();
void lcd_putc(char c, uint8_t attribute);
void lcd_putc_pos(int row, int col, char c, uint8_t attribute);
void lcd_print_string(const char *str, uint8_t attribute);
void lcd_print_uint16(uint16_t value, bool asHex);
void lcd_print_string_pos(int row, int col, const char *str, uint8_t attribute);
void lcd_print_string_ram(const char *str, uint8_t attribute);
void lcd_print_string_ram_pos(int row, int col, const char *str, uint8_t attribute);
void lcd_install_font();
void lcd_draw_double_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint8_t color);
uint8_t lcd_read_pixel(uint16_t x, uint16_t y);

#endif