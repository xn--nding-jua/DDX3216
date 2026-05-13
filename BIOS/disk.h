/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef DISK_H_
#define DISK_H_

#include "bios.h"

void pcmcia_init();
void ide_wait_ready();

#endif