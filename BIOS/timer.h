/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef TIMER_H_
#define TIMER_H_

#include "bios.h"

void timer_init();
static uint8_t bcd_to_bin(uint8_t val);
uint8_t read_rtc(uint8_t reg);

#endif