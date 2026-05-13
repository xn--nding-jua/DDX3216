/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#include "disk.h"

// **********************************************************
// PCMCIA / disk functions
// **********************************************************

void pcmcia_init() {
	// initializing PCMCIA and map I/O, so that it will react on 0x1F0
	
	// enable PCMCIA slot 0 and enable I/O mapping
	write_sc300_cfg(0x30, 0x81);
	
	// set I/O window 0 to start-address 0x1F0
	write_sc300_cfgw(0x32, 0x01F0);
	
	// set I/O window 0 to stop-address 0x1F7
	write_sc300_cfgw(0x34, 0x01F7);
}

void ide_wait_ready() {
    while ((inb(0x1F7) & (IDE_STATUS_BSY | IDE_STATUS_DRDY)) != IDE_STATUS_DRDY);
}
