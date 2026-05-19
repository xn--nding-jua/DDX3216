/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef BIOS_H_
#define BIOS_H_

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include "registers.h"
#include "inline.h"
#include "uart.h"
#include "lcd.h"
#include "disk.h"
#include "timer.h"
#include "isr.h"
#include "helper.h"

#define LCD_BPP                 1      // bits per pixel (1 for text mode, 4 for graphics mode)
#define LCD_WIDTH               240    // real LCD resolution of the DDX3216
#define LCD_HEIGHT              64     // real vertical resolution of the LCD panel
#define VRAM_SIZE               ((LCD_WIDTH * LCD_BPP / 8) * (LCD_HEIGHT / 8) * 2)

struct sCursorPosition {
    uint8_t row;
    uint8_t col;
};
extern struct sCursorPosition cursorPosition; // keep track of the current cursor position for lcd_putc()

extern void activate_unreal_mode(void);

#endif