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

extern void activate_unreal_mode(void);

#endif