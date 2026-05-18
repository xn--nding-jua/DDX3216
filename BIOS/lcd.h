/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef LCD_H_
#define LCD_H_

#include "bios.h"

void lcd_init();
void lcd_clear();
void lcd_clear_test();
void lcd_putchar(int row, int col, char c, uint8_t attribute);
void lcd_print_string(int row, int col, const char *str, uint8_t attribute);

#endif