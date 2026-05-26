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
	// PCMCIA-card is connected via 10-bit address-bus and 8-bit data-bus
	// more signals connected to SC300:
	// MCELA# -> CE1#               Card Enable Even byte
	// RDYA#  <- RDY/BSY#           Ready/Busy Signal
	// WAIT#  <- WAIT#              Extend Bus Cycle Wait Signal
	// RSTA   -> RESET              Card Reset
	// REGA#  -> REG#               Attribute Memory Select
	// CDA#   <- CD1# and CD2#      Card Detect Signal
	// BVD1A  -> BVD1               Battery Voltage Detect 1
	// BVD2A  -> BVD2               Battery Voltage Detect 2
	//
	// for more see chapter 5.3 in programming reference manual
	
    // I/O-addresses 0x1F0-0x1F7 for data, error, sector count, sector number, cylinder low, cylinder high, drive/head and status/command

    // enable PCMCIA reset
    uint8_t b4 = read_sc300_cfg(PCMCIA_CARD_RESET);
    write_sc300_cfg(PCMCIA_CARD_RESET, b4 | PCMCIA_CARESET); // set bit 2 = CARESET
    for (volatile uint32_t i = 0; i < 10000; i++); // wait ~1ms
    write_sc300_cfg(PCMCIA_CARD_RESET, b4 & ~PCMCIA_CARESET);   // reset bit 2

    // CF-card needs about 50ms to initialize after reset, so wait a bit
    for (volatile uint32_t i = 0; i < 500000; i++);

    // enable 8-bit mode for PCMCIA
    write_sc300_cfg(PCMCIA_DATA_WIDTH,          0x00);    // Alles 8-Bit

    // configure PCMCIA window A1 to I/O window 0x1F0-0x1F7
    write_sc300_cfg(PCMCIA_WIN_A1_LOW_START,    0xF0);
    write_sc300_cfg(PCMCIA_WIN_A1_LOW_END,      0xF7);
    write_sc300_cfg(PCMCIA_WIN_A1_UPPER,        0x01); // for both upper and lower
    // configure PCMCIA Window A2 to I/O window 0x3F6-0x3F7 (IDE Control)
    write_sc300_cfg(PCMCIA_WIN_A2_LOW_START,    0xF6);
    write_sc300_cfg(PCMCIA_WIN_A2_LOW_END,      0xF7);
    write_sc300_cfg(PCMCIA_WIN_A2_UPPER,        0x03); // for both upper and lower

    // set I/O Card IRQ Redirection
    write_sc300_cfg(PCMCIA_CARD_IEQ_REDIRECT,   0x07); // ICAIOEN=1, ICAIOWIN2=1, ICAIOWIN1=1

    // configure wait-states
    uint8_t ws2 = read_sc300_cfg(MMS_MEMORY_WAIT_STATE_CTRL2);
    ws2 |= (1 << 5);    // CARDWSEN = 1
    ws2 |= (1 << 4);    // CARDWS1  = 1
    //ws2 |= (1 << 3);    // CARDWS0  = 1
    write_sc300_cfg(MMS_MEMORY_WAIT_STATE_CTRL2, ws2);    

    // configure I/O wait-states
    uint8_t iows = read_sc300_cfg(IO_WAIT_STATE_CTRL);
    iows |=  (1 << 3);  // HDWS1 = 1
    //iows |=  (1 << 2);  // HDWS0 = 1
    write_sc300_cfg(IO_WAIT_STATE_CTRL, iows);

    // CF-Card software-reset via IDE-Register
    outb(IDE_DEV_CTRL, 0x06);   // SRST=1, nIEN=1
    for (volatile uint16_t i = 0; i < 10000; i++);
    outb(IDE_DEV_CTRL, 0x02);   // SRST=0, nIEN=1 (IRQ stays disabled)
    for (volatile uint16_t i = 0; i < 10000; i++);

    // wait for CF-card
    if (!ide_wait_ready()) {
        uart_print("PCMCIA/CF: Timeout!\n");
        return;
    }

    uart_print("PCMCIA/CF: Init OK\n");
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

// set to maximum wait-states for slow CF-card access (e.g. for debugging with logic analyzer)
void pcmcia_set_waitstates_slow() {
    uint8_t iows = read_sc300_cfg(IO_WAIT_STATE_CTRL);
    iows &= ~(1 << 3);  // HDWS1 = 0
    iows &= ~(1 << 2);  // HDWS0 = 0  → 5 Wait States
    write_sc300_cfg(IO_WAIT_STATE_CTRL, iows);
}

bool ide_read_bootsector() {
    // wait until drive is ready
    if (!ide_wait_ready()) {
        uart_print("ide_read_bootsector: Drive not ready\n");
        return false;
    }

    // LBA-Address and send command
    uint32_t lba = 0; // LBA 0 = MBR
    outb(IDE_SECT_COUNT,  1);
    outb(IDE_LBA_LOW,     (uint8_t)( lba        & 0xFF));
    outb(IDE_LBA_MID,     (uint8_t)((lba >>  8) & 0xFF));
    outb(IDE_LBA_HIGH,    (uint8_t)((lba >> 16) & 0xFF));
    outb(IDE_DRIVE_HEAD,  0xE0 | (uint8_t)((lba >> 24) & 0x0F));
    outb(IDE_COMMAND,     IDE_CMD_READ);

    // wait until data is ready (DRQ set)
    if (!ide_wait_drq()) {
        uart_print("ide_read_bootsector: DRQ timeout\n");
        return false;
    }

    // copy 512 bytes from CF-card to 0x7C00 in RAM
    // IDE_DATA (0x1F0) delivers at 8-Bit access always Low-Byte
    uint8_t* bootsector = (uint8_t*)0x7C00;
    for (uint16_t i = 0; i < 512; i++) {
        bootsector[i] = inb(IDE_DATA);
    }

    return true;
}

bool ide_read_sector(uint32_t lba, uint8_t* buffer) {
    // wait until drive is ready
    if (!ide_wait_ready()) {
        uart_print("ide_read_sector: Drive not ready\n");
        return false;
    }

    // LBA-Address and send command
    outb(IDE_SECT_COUNT,  1);
    outb(IDE_LBA_LOW,     (uint8_t)( lba        & 0xFF));
    outb(IDE_LBA_MID,     (uint8_t)((lba >>  8) & 0xFF));
    outb(IDE_LBA_HIGH,    (uint8_t)((lba >> 16) & 0xFF));
    outb(IDE_DRIVE_HEAD,  0xE0 | (uint8_t)((lba >> 24) & 0x0F));
    outb(IDE_COMMAND,     IDE_CMD_READ);

    // wait until data is ready (DRQ set)
    if (!ide_wait_drq()) {
        uart_print("ide_read_sector: DRQ timeout\n");
        return false;
    }

    // 256 Words = read 512 bytes
    // we have 8-Bit Bus -> each Word as two Bytes read
    // IDE_DATA (0x1F0) delivers at 8-Bit access always Low-Byte
    for (uint16_t i = 0; i < 512; i++) {
        buffer[i] = inb(IDE_DATA);
    }

    return true;
}

// reads a single byte from the bootsector at the given offset (0-511)
uint8_t read_bootsector_byte(uint16_t offset) {
    // buffer in RAM for the whole sector
    // this is within the BSS-Segment at 0x0500+
    static uint8_t sector_buffer[512];

    if (offset >= 512) {
        return 0xFF;    // Invalid offset
    }

    if (!ide_read_sector(0, sector_buffer)) {
        return 0xFF;    // Read error
    }

    return sector_buffer[offset];
}