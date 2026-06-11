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

// global variable for disk-parameters of the first harddisk (HD0)
struct floppy_param_table fd0_params;
struct disk_param_table hd0_params;

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

bool disk_read_identify_data() {
    outb(IDE_COMMAND, IDE_CMD_IDENT);
    // read 512 bytes of IDENTIFY-Data

    // wait until data is ready (DRQ set)
    if (!ide_wait_drq()) {
        uart_print_string("ide_read_sector: DRQ timeout\n");
        return false;
    }

    // read 512 bytes and IDE_DATA (0x1F0) delivers at 8-Bit access always Low-Byte
    for (uint16_t i = 0; i < 512; i++) {
        uint8_t data = inb(IDE_DATA);
        writeFarByte(BASE_SEG, 0x7C00 + i, data);
    }

    return true;
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

    // initialize the floppy-disk-parameter-table at this opportunity
	fd0_params.steprate = 0xDF;               // Step rate 2ms, head unload time 240ms
	fd0_params.head_loadtime = 0x02;          // Head load time 4 ms, non-DMA mode 0
	fd0_params.delay = 0x25;                  // Byte delay until motor turned off
	fd0_params.bytes_per_sector = 0x02;       // 512 bytes per sector
	fd0_params.sectors_per_track = 18;        // 18 sectors per track (1.44MB)
	fd0_params.gap_between_sectors = 0x1B;    // Gap between sectors for 3.5" floppy
	fd0_params.data_length = 0xFF;            // Data length (ignored)
	fd0_params.gap_length = 0x54;             // Gap length when formatting
	fd0_params.fomat_filler = 0xF6;           // Format filler byte
	fd0_params.head_settle_time = 0x0F;       // Head settle time (1 ms)
	fd0_params.motor_start = 0x08;            // Motor start time in 1/8 seconds


    // enable PCMCIA reset
    uint8_t b4 = read_sc300_cfg(PCMCIA_CARD_RESET);
    write_sc300_cfg(PCMCIA_CARD_RESET, b4 | PCMCIA_CARESET); // set bit 2 = CARESET
    for (volatile uint32_t i = 0; i < 10000; i++); // wait ~1ms
    write_sc300_cfg(PCMCIA_CARD_RESET, b4 & ~PCMCIA_CARESET);   // reset bit 2

    // CF-card needs about 50ms to initialize after reset, so wait a bit
    for (volatile uint32_t i = 0; i < 50000; i++);

    uart_print_string("PCMCIA/CF: Configuring Cardslot\n");

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
    //uart_print_string("Socket A-Status:");
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
	
    uart_print_string("PCMCIA/CF: Card soft reset\n");
    // CF-Card software-reset via IDE-Register
    outb(IDE_DEV_CTRL, 0x06);   // SRST=1, nIEN=1
    for (volatile uint16_t i = 0; i < 10000; i++);
    outb(IDE_DEV_CTRL, 0x02);   // SRST=0, nIEN=1 (IRQ stays disabled)
    for (volatile uint16_t i = 0; i < 10000; i++);

    // select drive 0 (master)
    outb(IDE_DRIVE_HEAD, 0xA0);
    for (volatile uint32_t i = 0; i < 10000; i++);

    // wait for CF-card
    uart_print_string("PCMCIA/CF: Wait for card...");
    if (!ide_wait_ready()) {
        lcd_print_string("Timeout!\n", 0x07);
        uart_print_string("Timeout!\n");
        return false;
    }

    // read drive geometry from CF-card using IDENTIFY-Command into bootloader-buffer at 0x7C00
    if (!disk_read_identify_data()) {
        // something went wrong during IDENTIFY-Command -> fallback with fixed geometry for 512MB CF-Card
        uart_print_string("Failed to read IDENTIFY-Data!\n");
        lcd_print_string("ERROR\n", 0x07);
        return false;

        /*
        // fixed CF-Card geometry with 512MB
        // CHS for BIOS-compatibility (DOS-Limit: 1024/16/63)
        hd0_params.cylinders         = 1014;
        hd0_params.heads             = 16;
        hd0_params.reduced_write_cyl = 0xFFFF;  // no Precompensation
        hd0_params.write_precomp_cyl = 0xFFFF;  // no Reduced Write
        hd0_params.max_ecc_burst     = 0x0B;    // standard-value that DOS expects
        hd0_params.control_byte      = 0x08;    // must be 8 when 16 heads
        hd0_params.timeout_drive     = 0x00;
        hd0_params.timeout_format    = 0x00;
        hd0_params.timeout_check     = 0x00;
        hd0_params.landing_zone      = 1013;
        hd0_params.sectors_per_track = 63;
        hd0_params.reserved          = 0x00;
        */
    }else{
        // copy relevant parameters from IDENTIFY-Data to global variable
        hd0_params.cylinders          = readFarWord(BASE_SEG, 0x7C00 + (1 * sizeof(uint16_t)));     // word 1: cylinders
        hd0_params.heads              = readFarByte(BASE_SEG, 0x7C00 + (3 * sizeof(uint16_t)));     // word 3: heads
        hd0_params.reduced_write_cyl  = 0x00;        // no Precompensation
        hd0_params.write_precomp_cyl  = 0x00;        // no Reduced Write
        hd0_params.max_ecc_burst      = 0x00;        // standard-value that DOS expects
        hd0_params.control_byte       = 0b11000000;  // disable retries (bit 7), disable ECC (bit 6)
        hd0_params.timeout_drive      = 0x00;
        hd0_params.timeout_format     = 0x00;
        hd0_params.timeout_check      = 0x00;
        hd0_params.landing_zone       = 0x00;
        hd0_params.sectors_per_track  = readFarByte(BASE_SEG, 0x7C00 + (6 * sizeof(uint16_t)));     // word 6: sectors per track
        hd0_params.reserved           = 0x00;

        // read LBA-sectors for max. 4GB disks
        uint32_t lba_sectors = readFarWord(BASE_SEG, 0x7C00 + (60 * sizeof(uint16_t))); // word 60..61: total number of sectors (LBA)

        // check if we have more than 1024 cylinders, 16 heads or 63 sectors per track and if so,
        // recalculate the geometry to fit into the CHS limits of the BIOS (1024/16/63) and set hd0_params accordingly
        if (hd0_params.cylinders > 1024) {
            // recalculate geometry to fit into CHS limits of the BIOS
            hd0_params.cylinders = 1024;
            hd0_params.heads = (lba_sectors / (hd0_params.cylinders * hd0_params.sectors_per_track)) + 1;
            if (hd0_params.heads > 16) {
                hd0_params.heads = 16;
                hd0_params.sectors_per_track = (lba_sectors / (hd0_params.cylinders * hd0_params.heads)) + 1;
                if (hd0_params.sectors_per_track > 63) {
                    hd0_params.sectors_per_track = 63;
                }
            }
        }

        
// ========================= DEBUG =========================
// data for debug purposes
hd0_params.cylinders          = 507;        // total numbers of cylinders
hd0_params.heads              = 32;         // total numbers of heads
hd0_params.reduced_write_cyl  = 0x00;       // starting reduced-write current cylinder
hd0_params.write_precomp_cyl  = 0x00;       // starting write precompensation cylinder
hd0_params.max_ecc_burst      = 0x00;       // maximum ECC data burst length
hd0_params.control_byte       = 0b11000000; // disable retries (bit 7), disable ECC (bit 6)
hd0_params.timeout_drive      = 0x00;       // standard timeout value
hd0_params.timeout_format     = 0x00;       // timeout value for format drive
hd0_params.timeout_check      = 0x00;       // timeout value for check drive
hd0_params.landing_zone       = 0x00;       // landing-zone (might be number of cylinders - 1 ???)
hd0_params.sectors_per_track  = 63;         // total numbers of sectors per track
hd0_params.reserved           = 0x00;       // reserved
// ========================= DEBUG END =========================


        // print geometry to LCD for debugging
        char textbuffer[6];
        uint16_to_dec(hd0_params.cylinders, textbuffer);
        lcd_print_string("C", 0x07);
        lcd_print_string_ram(textbuffer, 0x07);

        uint8_to_dec(hd0_params.heads, textbuffer);
        lcd_print_string("H", 0x07);
        lcd_print_string_ram(textbuffer, 0x07);

        uint8_to_dec(hd0_params.sectors_per_track, textbuffer);
        lcd_print_string("S", 0x07);
        lcd_print_string_ram(textbuffer, 0x07);
        lcd_putc('\n', 0x07);
    }

    //lcd_print_string("OK\n", 0x07);
    uart_print_string("OK\n");
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
            uart_print_string("Error!\n");
            return false;
        }
        if (!(status & IDE_SR_BSY) && (status & IDE_SR_DRDY)) {
            return true; // ready
        }

        // small delay between polls
        for (volatile uint32_t d = 0; d < 10; d++);
    }

    uart_print_string("Timeout!\n");
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
        uart_print_string("ide_read_sector: Drive not ready\n");
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
        uart_print_string("ide_read_sector: DRQ timeout\n");
        return 0xBB;
    }

    // read 512 bytes and IDE_DATA (0x1F0) delivers at 8-Bit access always Low-Byte
    for (uint16_t i = 0; i < 512; i++) {
        uint8_t data = inb(IDE_DATA);
        writeFarByte(dest_seg, offset + i, data);
    }

    return 0x00; // no error
}

// LBA-calculation based on CHS
// LBA = (C × H_max + H) × S_max + (S - 1)
uint32_t disk_chs_to_lba(uint32_t c, uint32_t h, uint32_t s) {
    return (c * (uint32_t)hd0_params.heads + h) * (uint32_t)hd0_params.sectors_per_track + s - 1;
}
