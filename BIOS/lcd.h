/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef LCD_H_
#define LCD_H_

#include "bios.h"
#include "fonts.h"

bool g_lcd_textmode;

// general lcd functions
void lcd_install_font();
void lcd_init(bool textmode);
void lcd_clear();

// functions for textmode
void lcd_scroll_up();
void lcd_putc(char c, uint8_t attribute);
void lcd_putc_pos(int row, int col, char c, uint8_t attribute);
void lcd_print_string(const char *str, uint8_t attribute);
void lcd_print_uint16(uint16_t value, bool asHex);
void lcd_print_string_pos(int row, int col, const char *str, uint8_t attribute);
void lcd_print_string_ram(const char *str, uint8_t attribute);
void lcd_print_string_ram_pos(int row, int col, const char *str, uint8_t attribute);
void lcd_draw_double_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

// functions for graphicmode
void lcd_draw_pixel(uint16_t x, uint16_t y, uint8_t color);
uint8_t lcd_read_pixel(uint16_t x, uint16_t y);
void lcd_draw_line(uint16_t startx, uint16_t starty, uint16_t endx, uint16_t endy, uint8_t color);
void lcd_draw_rect(uint16_t startx, uint16_t starty, uint16_t endx, uint16_t endy, uint8_t lineColor, uint8_t fillColor);
void lcd_graphic_putc(uint16_t x, uint16_t y, char c, uint8_t color, bool inverted);
void lcd_draw_circle(uint16_t centerx, uint16_t centery, uint16_t radius, uint8_t color);
void lcd_graphics_scroll_vertical(int8_t pixels);

// some high level functions
void lcd_graphics_demo();

#endif