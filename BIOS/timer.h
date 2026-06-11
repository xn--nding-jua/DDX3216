/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef TIMER_H_
#define TIMER_H_

#include "bios.h"

void timer_init();
uint8_t read_rtc(uint8_t reg);
void delay_1s_hardware(void);

#endif