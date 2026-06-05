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
CF-Karte acts like an IDE harddisk-drive
┌─────────────────────────────────────────────────────┐
│  CF-Card with 512MB BIOS-Geometry                   │
├─────────────────┬───────────────────────────────────┤
│  Cylinders      │  1024        (BIOS-Maximum)       │
│  Headers        │  16                               │
│  Sectors/Track  │  63                               │
│  Sector-Size    │  512 Bytes                        │
├─────────────────┼───────────────────────────────────┤
│  Total Sectors  │  1.032.192                        │
│  Usable capac.  │  ~504 MB (enoug for 512MB)        │
├─────────────────┼───────────────────────────────────┤
│  LBA-Adressing  │  Yes                              │
│  INT 13h Access │  CHS -> LBA Conversion            │
└─────────────────┴───────────────────────────────────┘
*/

void mms_page_redirect(uint8_t MMS, uint8_t page, uint32_t address) {
    uint8_t bit_23      = (address >> 23) & 0x01; // bit 23
    uint8_t bit_22_21   = (address >> 21) & 0x03; // bits 22..21
    uint8_t bit_20_14   = (address >> 14) & 0x7F; // bits 20..14
    uint8_t reg_val; // storage for current register-setting

    if (page > 7) {
        return;
    }

    if (MMS == 0) {
		// select MMSA
		write_sc300_cfg(MMSB_CONTROL, 0b00000010);
	}else{
		if (page > 3) {
			// MMSB supports only page 0..3
			return;
		}
		
		// select MMSB
		write_sc300_cfg(MMSB_CONTROL, 0b00000001);
	}

    // set bit 23
    // MSB: page 7, LSB: page 0
    reg_val = read_sc300_cfg(MMS_ADDRESS_EXTENSION1);
    reg_val &= ~(1 << page);               // delete old bit
    reg_val |= (bit_23 << page);           // set new bit
    write_sc300_cfg(MMS_ADDRESS_EXTENSION1, reg_val);

    // set bits 22..21 depending on selected page
    switch (page) {
        // pages 0 to 3 are stored in MMS_ADDRESS_EXTENSION2
        case 0: case 1: case 2: case 3:
            reg_val = read_sc300_cfg(MMS_ADDRESS_EXTENSION2);
            reg_val &= ~(0x03 << (page * 2));          // delete two old bits
            reg_val |= (bit_22_21 << (page * 2));      // set both new bits 22..21
            write_sc300_cfg(MMS_ADDRESS_EXTENSION2, reg_val);
            break;

        // pages 4 to 7 are stored in MMSA_ADDRESS_EXTENSION1
        case 4: case 5: case 6: case 7:
            reg_val = read_sc300_cfg(MMSA_ADDRESS_EXTENSION1);
            // (page - 4) sorgt dafür, dass Page 4 bei Bit 0 startet, Page 5 bei Bit 2, etc.
            reg_val &= ~(0x03 << ((page - 4) * 2));    
            reg_val |= (bit_22_21 << ((page - 4) * 2));
            write_sc300_cfg(MMSA_ADDRESS_EXTENSION1, reg_val);
            break;

        default:
            // invalid page
            return;
    }

    // set bits 20..14 and keep page enabled
    uint16_t io_port = 0x0208 + (page * 0x2000);
    outb(io_port, 0x80 | bit_20_14);
}

void mms_init() {
	// Configure MMSA Device pages 0...7 to be mapped at PCMCIA-Slot A
	// MMSA: 8x 16kB pages (mapped to 0xD0000 … 0xF0000) = 131072 bytes
	// MMSB: 4x 16kB pages (fixed at 0xA0000, right after the first 640kB)

	// select MMSA
	write_sc300_cfg(MMSB_CONTROL, 0b00000010);
	// set MMSA Base-Address to 0xD0000 and Page-IO-Address to 0x0208
	write_sc300_cfg(MMS_ADDRESS, 0b01000000);
	// no address extension-bit 23 for pages 0…7
	write_sc300_cfg(MMS_ADDRESS_EXTENSION1, 0b00000000);
	// no address-extension-bits 21/22 for pages 0…3
	write_sc300_cfg(MMS_ADDRESS_EXTENSION2, 0b00000000);
	// no address-extension-bits 21/22 for pages 4…7
	write_sc300_cfg(MMSA_ADDRESS_EXTENSION1, 0b00000000);

	// set translation-addresses and enable page 0...7
	// bit 7 = page enable, bits 6..0 = Translate Address A20...A14
	outb(0x0208, 0b10000000); // translate A20...A14 of MMSA page 0
	outb(0x2208, 0b10000001); // translate A20...A14 of MMSA page 1
	outb(0x4208, 0b10000010); // translate A20...A14 of MMSA page 2
	outb(0x6208, 0b10000011); // translate A20...A14 of MMSA page 3
	outb(0x8208, 0b10000100); // translate A20...A14 of MMSA page 4
	outb(0xA208, 0b10000101); // translate A20...A14 of MMSA page 5
	outb(0xC208, 0b10000110); // translate A20...A14 of MMSA page 6
	outb(0xE208, 0b10000111); // translate A20...A14 of MMSA page 7

	// set memory type for MMSA pages 0..3 to PCMCIA
	write_sc300_cfg(MMSA_DEVICE1, 0b10101010);
	// set memory type for MMSA pages 4..7 to PCMCIA
	write_sc300_cfg(MMSA_DEVICE2, 0b10101010);
	// select socket: MMSA pages 0...7 mapped to socket A
	write_sc300_cfg(MMSA_SOCKET, 0b00000000); // 
	// enable MMSA
	write_sc300_cfg(ROM_CONFIGURATION1, 0b01000000); 
	// initialize CA25 and CA24 for MMSA page 0...3
	write_sc300_cfg(CA24_CA25_CONTROL1, 0b00000000);
	// initialize CA25 and CA24 for MMSA page 4...7
	write_sc300_cfg(CA24_CA25_CONTROL2, 0b00000000);

	// from now on memory-addresses 0xD0000 to 0xEFFFF representing the first 128kB of PCMCIA-Card A
}

bool cfcard_init() {
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
	
	// OE# is pulled-up with a 10k-resistor, so the CF-Card will start in PCMCIA-MemoryMode (PC-Card Standard)
	// so we have to switch into TrueIDE mode using Configuration Option Register (COR) of the CF-Card
	
    // I/O-addresses 0x1F0-0x1F7 for IO-Read/Write in TrueIDE-Mode
	// I/O-addresses 0x3A4 for REGA#

    // enable PCMCIA reset
    uint8_t b4 = read_sc300_cfg(PCMCIA_CARD_RESET);
    write_sc300_cfg(PCMCIA_CARD_RESET, b4 | PCMCIA_CARESET); // set bit 2 = CARESET
    for (volatile uint32_t i = 0; i < 10000; i++); // wait ~1ms
    write_sc300_cfg(PCMCIA_CARD_RESET, b4 & ~PCMCIA_CARESET);   // reset bit 2

    // CF-card needs about 50ms to initialize after reset, so wait a bit
    for (volatile uint32_t i = 0; i < 50000; i++);

    uart_print("PCMCIA/CF: Configuring Cardslot\n");

    // enable 16-bit mode for PCMCIA using two 8-bit cycles (see page 5-18 in programming reference manual)
    // IOIS16# is tied to HIGH, so we are using 8-bit access
    // with two bytes per word: two 8-bit cycles are generated to the CARD using MCELA#
    // see page 5-18 in Progremming Reference Manual
    // so following two lines are both valid as both will translate 16-bit value to two 8-bit cycles on the CARD:
    //write_sc300_cfg(PCMCIA_DATA_WIDTH,          0b10101111); // pure 8-bit-access
    write_sc300_cfg(PCMCIA_DATA_WIDTH,          0x00); // pure 8-bit-access

    // configure PCMCIA window A1 to I/O window 0x1F0-0x1F7
    write_sc300_cfg(PCMCIA_WIN_A1_LOW_START,    0xF0);
    write_sc300_cfg(PCMCIA_WIN_A1_LOW_END,      0xF7);
    write_sc300_cfg(PCMCIA_WIN_A1_UPPER,        0x01); // for both upper and lower
    // configure PCMCIA Window A2 to I/O window 0x3F6-0x3F7 (IDE Control)
    write_sc300_cfg(PCMCIA_WIN_A2_LOW_START,    0xF6);
    write_sc300_cfg(PCMCIA_WIN_A2_LOW_END,      0xF7);
    write_sc300_cfg(PCMCIA_WIN_A2_UPPER,        0x03); // for both upper and lower

    // set I/O Card IRQ Redirection
    write_sc300_cfg(PCMCIA_CARD_IRQ_REDIRECT,   0b0000111); // enable REGA# and MCELA# on I/O access

    // configure wait-states
    write_sc300_cfg(MMS_MEMORY_WAIT_STATE_CTRL2, 0x00); // default, 1 SYSCLK or controlled by MMS_MEMORY_WAIT_STATE1
    write_sc300_cfg(MMS_MEMORY_WAIT_STATE1, 0b00000011); // 2 SYSCLK-Waitstates for 8-BIT IO Access

    // configure I/O wait-states
    write_sc300_cfg(IO_WAIT_STATE_CTRL, 0b01111100); // 2 SYSCLK WaitStates for HDD and IO, select HighSpeed mode

    // check if card is present (see page 5-68 in programming reference manual)
    //uart_print("Socket A-Status:");
    //uart_putc(read_sc300_cfg(PCMCIA_SOCKET_A_STATUS));

    // configure VPPA-pin (here only bits 9...2 are set)
	// VPPA is not used in CF-Cards
    //write_sc300_cfg(PCMCIA_VPPA_ADDRESS, 0b11101000); // map VPPA to 0x3A0
    //outb(VPPA_BASE, 0x00); // enable VPPA for CF-card. DataBit 0 will control the power

    // configure REGA#-pin (here only bits 9...2 are set)
    write_sc300_cfg(PCMCIA_REGA_ADDRESS, 0b11101001); // map REGA# to 0x3A4


	// see page 3-21 in CF Card Specification for the following modes and registers
	//
	// switch CF-Card from PCMCIA-MemoryMode to TrueIDE-mode
	// ==============================================
	// VCC    -> CE2# = 1
	// MCELA# -> CE1# = 0 or 1
	// MEMR#  -> OE#
	// MEMW#  -> WE#
	// REGA#  -> 0x00 = 1
	// 
	// IO Read Operation:
	// ==============================================
	// Common Memory Read
	// Common Memory Write
	// 
	// 
	// Configuration Registers Decoding
	// ==============================================
	// VCC    -> CE2# = 1
	// MCELA# -> CE1# = 0
	// MEMR#  -> OE#
	// MEMW#  -> WE#
	// REGA#  -> 0x01 = 0
	// Fixed Address Base: 0b01000000000 = 0x200
	// 
	// Memory Read Operation:
	// -------------------------------------------
	// 0x200: Configuration Option Register
	// 0x202: Card Status Register
	// 0x204: Pin Replacement Register
	// 0x206: Socket and Copy Register
	// 
	// Memory Write Operation:
	// -------------------------------------------
	// 0x200: Configuration Option Register
	// 0x202: Card Status Register
	// 0x204: Pin Replacement Register
	// 0x206: Socket and Copy Register

	// set REGA# to LOW to access Attribute Memory
    outb(REGA_BASE, 0x01);
	// read Card Information Structure (CIS) to get Configuration Option Register  (COR) address
	//uint8_t cis_byte = readFarByte(PCMCIA_BASE, 0x0000);
	// set COR to switch from memory-mapped-mode into I/O-mapped mode in Polling-Mode (0x1F0...0x1F7 / 0x3F6...0x3F7)
	writeFarByte(PCMCIA_BASE, 0x0200, 0b00000010); // b7=SoftReset, b6=Interrupt-Mode, b5=CardReset, b4..0=ConfigIndex
	// set REGA# to HIGH to access Common Memory
	delay_1us();
    outb(REGA_BASE, 0x00);
	

    uart_print("PCMCIA/CF: Card soft reset\n");
    // CF-Card software-reset via IDE-Register
    outb(IDE_DEV_CTRL, 0x06);   // SRST=1, nIEN=1
    for (volatile uint16_t i = 0; i < 10000; i++);
    outb(IDE_DEV_CTRL, 0x02);   // SRST=0, nIEN=1 (IRQ stays disabled)
    for (volatile uint16_t i = 0; i < 10000; i++);

    // select drive 0 (master)
    outb(IDE_DRIVE_HEAD, 0xA0);
    for (volatile uint32_t i = 0; i < 10000; i++);

    // wait for CF-card
    uart_print("PCMCIA/CF: Wait for card...");
    if (!ide_wait_ready()) {
        uart_print("Timeout!\n");
        return false;
    }

    uart_print("OK\n");
    return true;
}

bool ide_wait_ready() {
    // some dummy reads to wait for the drive to process the previous command and set BSY=0
    inb(IDE_ALT_STATUS);
    inb(IDE_ALT_STATUS);
    inb(IDE_ALT_STATUS);
    inb(IDE_ALT_STATUS);

    for (uint32_t timeout = 10000; timeout > 0; timeout--) {
        uint8_t status = inb(IDE_ALT_STATUS);

        if (status & IDE_SR_ERR) {
            uart_print("Error!\n");
            return false;
        }
        if (!(status & IDE_SR_BSY) && (status & IDE_SR_DRDY)) {
            return true; // ready
        }

        // small delay between polls
        for (volatile uint32_t d = 0; d < 10; d++);
    }

    uart_print("Timeout!\n");
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

uint8_t ide_read_bootsector() {
   uint32_t lba = 0;
   return ide_read_sector(lba, BASE_SEG, 0x7C00);
}

uint8_t ide_read_sector(uint32_t lba, uint16_t dest_seg, uint16_t offset) {
    // wait until drive is ready
    if (!ide_wait_ready()) {
        uart_print("ide_read_sector: Drive not ready\n");
        return 0xAA;
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
        return 0xBB;
    }

    // read 512 bytes and IDE_DATA (0x1F0) delivers at 8-Bit access always Low-Byte
    for (uint16_t i = 0; i < 512; i++) {
        uint8_t data = inb(IDE_DATA);
        writeFarByte(dest_seg, offset + i, data);
    }

    /*
    __asm__ __volatile__ (
        "pushw %%es\n\t"
        "movw  %0, %%es\n\t"        // ES = dest_seg (einmalig!)
        "movw  %1, %%di\n\t"        // DI = offset
        "movw  $512, %%cx\n\t"      // Zähler: 512 Bytes
        "movw  %2, %%dx\n\t"        // DX = IDE_DATA Port (0x1F0)
    "1:\n\t"
        "inb   %%dx, %%al\n\t"      // 1 Byte lesen
        "stosb\n\t"                 // ES:[DI]++ = AL
        "loop  1b\n\t"              // CX--; wenn != 0 -> Sprung
        "popw  %%es\n\t"
        :
        : "r"(dest_seg), "r"(offset), "i"(IDE_DATA)
        : "ax", "cx", "dx", "di", "memory"
    );
    */
    return 0x00; // no error
}
