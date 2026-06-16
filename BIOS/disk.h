/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef DISK_H_
#define DISK_H_

#include "bios.h"

// MMS registers
#define MMSB_CONTROL				0x74
#define MMS_ADDRESS					0x6D
#define MMS_ADDRESS_EXTENSION1		0x6C
#define MMS_ADDRESS_EXTENSION2		0x6E
#define MMSA_ADDRESS_EXTENSION1		0x67
#define MMSA_DEVICE1				0x71
#define MMSA_DEVICE2				0x72
#define MMSA_SOCKET					0xA8
#define ROM_CONFIGURATION1			0x65
#define CA24_CA25_CONTROL1			0xB5
#define CA24_CA25_CONTROL2			0xB6

// PCMCIA registers
#define PCMCIA_BASE					0xD000
#define PCMCIA_CARESET              (1 << 2)

#define PCMCIA_WIN_A1_LOW_START     0x00
#define PCMCIA_WIN_A1_LOW_END       0x01
#define PCMCIA_WIN_A1_UPPER         0x02
#define PCMCIA_WIN_A2_LOW_START     0x03
#define PCMCIA_WIN_A2_LOW_END       0x04
#define PCMCIA_WIN_A2_UPPER         0x05
#define PCMCIA_CARD_IRQ_REDIRECT    0x06
#define PCMCIA_VPPA_ADDRESS         0x07
#define PCMCIA_DATA_WIDTH           0x0A
#define PCMCIA_REGA_ADDRESS         0x8A
#define PCMCIA_CARD_RESET           0xB4
#define PCMCIA_SOCKET_A_STATUS      0xA2

#define MMS_MEMORY_WAIT_STATE_CTRL2 0x50
#define IO_WAIT_STATE_CTRL          0x61
#define MMS_MEMORY_WAIT_STATE1      0x62

// IDE Register-Adressen
#define IDE_DATA        0x1F0   // Data Register (16-Bit)
#define IDE_ERROR       0x1F1   // Error Register (read)
#define IDE_FEATURES    0x1F1   // Features Register (write)
#define IDE_SECT_COUNT  0x1F2   // Sector Count
#define IDE_LBA_LOW     0x1F3   // Sector        / LBA Bits 7:0
#define IDE_LBA_MID     0x1F4   // Cylinder Low  / LBA Bits 15:8
#define IDE_LBA_HIGH    0x1F5   // Cylinder High / LBA Bits 23:16
#define IDE_DRIVE_HEAD  0x1F6   // Drive/Head    / LBA Bits 27:24
#define IDE_STATUS      0x1F7   // Status Register (read)
#define IDE_COMMAND     0x1F7   // Command Register (write)
#define IDE_ALT_STATUS  0x3F6   // Alternate Status (read, kein IRQ-Clear)
#define IDE_DEV_CTRL    0x3F6   // Device Control (write)

#define VPPA_BASE		0x3A0
#define REGA_BASE		0x3A4

// IDE Status-Bits
#define IDE_SR_BSY      0x80    // Busy
#define IDE_SR_DRDY     0x40    // Drive Ready
#define IDE_SR_DRQ      0x08    // Data Request
#define IDE_SR_ERR      0x01    // Error

// IDE Kommandos
#define IDE_CMD_READ    0x20    // Read Sectors with Retry
#define IDE_CMD_WRITE   0x30    // Write Sectors with Retry
#define IDE_CMD_IDENT   0xEC    // Identify Drive


void mms_init();
bool cfcard_init();
bool ide_wait_ready();
bool ide_wait_drq(void);
uint8_t ide_read_bootsector();
uint8_t ide_read_sector(uint32_t lba, uint16_t dest_seg, uint16_t offset);
uint8_t ide_write_sector(uint32_t lba, uint16_t src_seg, uint16_t offset);
uint32_t disk_chs_to_lba(uint32_t c, uint32_t h, uint32_t s);

struct floppy_param_table {
	uint8_t steprate;               // Step rate 2ms, head unload time 240ms
	uint8_t head_loadtime;          // Head load time 4 ms, non-DMA mode 0
	uint8_t delay;                  // Byte delay until motor turned off
	uint8_t bytes_per_sector;       // 512 bytes per sector
	uint8_t sectors_per_track;      // 18 sectors per track (1.44MB)
	uint8_t gap_between_sectors;    // Gap between sectors for 3.5" floppy
	uint8_t data_length;            // Data length (ignored)
	uint8_t gap_length;             // Gap length when formatting
	uint8_t fomat_filler;           // Format filler byte
	uint8_t head_settle_time;       // Head settle time (1 ms)
	uint8_t motor_start;            // Motor start time in 1/8 seconds
} __attribute__((packed));

struct disk_param_table {
    uint16_t cylinders;         // number of cylinders - 1
    uint8_t  heads;             // number of heads - 1
    uint16_t reduced_write_cyl; // starting reduced-write current cylinder
    uint16_t write_precomp_cyl; // starting write precompensation cylinder
    uint8_t  max_ecc_burst;     // maximum ECC data burst length
    uint8_t  control_byte;      // disable retries (bit 7), disable ECC (bit 6)
    uint8_t  timeout_drive;     // standard timeout value
    uint8_t  timeout_format;    // timeout value for format drive
    uint8_t  timeout_check;     // timeout value for check drive
    uint16_t landing_zone;      // landing-zone (might be number of cylinders - 1 ???)
    uint8_t  sectors_per_track; // sectors per track
    uint8_t  reserved;          // reserved
} __attribute__((packed));

struct disk_address_packet {
    uint8_t  size;          // +0x00: Strukturgröße (0x10)
    uint8_t  reserved;      // +0x01: immer 0x00
    uint16_t sector_count;  // +0x02: Anzahl Sektoren
    uint16_t dest_offset;   // +0x04: Ziel-Offset
    uint16_t dest_segment;  // +0x06: Ziel-Segment
    uint32_t lba_low;       // +0x08: LBA Bits 31:0
    uint32_t lba_high;      // +0x0C: LBA Bits 63:32 (bei uns immer 0)
} __attribute__((packed));

struct drive_params_ext {
    uint16_t size;            // +0x00: Strukturgröße (0x1A)
    uint16_t flags;           // +0x02: Info-Flags
    uint32_t cylinders;       // +0x04: Anzahl Zylinder
    uint32_t heads;           // +0x08: Anzahl Köpfe
    uint32_t sectors;         // +0x0C: Sektoren pro Track
    uint64_t total;           // +0x10: Gesamtsektoren
    uint16_t bytes_per_sect;  // +0x18: Bytes pro Sektor
} __attribute__((packed));

#endif