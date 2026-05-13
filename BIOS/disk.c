/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#include "disk.h"

// **********************************************************
// PCMCIA / disk functions
// **********************************************************

/*
CF-Karte verhält sich wie eine IDE-Festplatte:
┌─────────────────────────────────────────────────────┐
│  CF-Karte 512MB - BIOS-Geometrie                    │
├─────────────────┬───────────────────────────────────┤
│  Zylinder       │  1024        (BIOS-Maximum)       │
│  Köpfe          │  16                               │
│  Sektoren/Track │  63                               │
│  Sektorgröße    │  512 Bytes                        │
├─────────────────┼───────────────────────────────────┤
│  Gesamt Sektoren│  1.032.192                        │
│  Nutzbare Kap.  │  ~504 MB (ausreichend für 512MB)  │
├─────────────────┼───────────────────────────────────┤
│  LBA-Adressier. │  Ja (empfohlen, intern genutzt)   │
│  INT 13h Zugriff│  CHS → LBA Konvertierung          │
└─────────────────┴───────────────────────────────────┘
*/

void pcmcia_init() {
	// initializing PCMCIA and map I/O, so that it will react on 0x1F0

	write_sc300_cfg(0x30, 0x81); 		// enable PCMCIA slot 0 and enable I/O mapping
	write_sc300_cfgw(0x32, 0x01F0);		// set I/O window 0 to start-address 0x1F0
	write_sc300_cfgw(0x34, 0x01F7);		// set I/O window 0 to stop-address 0x1F7
	write_sc300_cfg(0x36, 0x01); 		// enable window-control in 8-bit-mode
	write_sc300_cfgw(0x38, 0x03F6);     // I/O Window 1 Start (IDE Control)
	write_sc300_cfgw(0x3A, 0x03F7);     // I/O Window 1 Stop
	write_sc300_cfg(0x3C, 0x09);        // I/O Window 1 Control: enable + 16-Bit

	// wait around 50ms
	for (volatile uint32_t i = 0; i < 500000; i++);

	// clear reset in IDE device-control
	outb(IDE_DEV_CTRL, 0x02);

	// Hardware-Reset of CF-Card
    outb(IDE_DEV_CTRL, 0x06);  // SRST=1 (Bit2): Software Reset
    for (volatile uint16_t i = 0; i < 10000; i++);
    outb(IDE_DEV_CTRL, 0x02);  // SRST=0: clear Reset
    for (volatile uint16_t i = 0; i < 10000; i++);

    // wait until card is ready
    ide_wait_ready();
}

bool ide_wait_ready() {
    // Erst 400ns warten (4 dummy reads des Alt-Status)
    inb(IDE_ALT_STATUS);
    inb(IDE_ALT_STATUS);
    inb(IDE_ALT_STATUS);
    inb(IDE_ALT_STATUS);

    // Timeout ~1 Sekunde bei 33MHz
    for (uint32_t timeout = 1000000; timeout > 0; timeout--) {
        uint8_t status = inb(IDE_ALT_STATUS);

        if (status & IDE_SR_ERR) {
            uart_print("IDE Error!\n");
            return false;
        }
        if (!(status & IDE_SR_BSY) && (status & IDE_SR_DRDY)) {
            return true;    // Bereit
        }
    }

    uart_print("IDE Timeout!\n");
    return false;
}

bool ide_wait_drq(void) {
    // Warten bis DRQ (Data Request) gesetzt ist
    for (uint32_t timeout = 1000000; timeout > 0; timeout--) {
        uint8_t status = inb(IDE_ALT_STATUS);

        if (status & IDE_SR_ERR) {
            return false;
        }
        if (status & IDE_SR_DRQ) {
            return true;
        }
    }
    return false;
}