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
uint32_t disk_chs_to_lba(uint32_t c, uint32_t h, uint32_t s);

struct __attribute__((packed)) disk_param_table {
    uint16_t cylinders;
    uint8_t  heads;
    uint16_t reserved1;
    uint16_t reserved2;
    uint8_t  sectors_per_track;
    uint16_t reserved3;
    uint8_t  drive_type;
};

struct __attribute__((packed)) disk_address_packet {
    uint8_t  size;          // +0x00: Strukturgröße (0x10)
    uint8_t  reserved;      // +0x01: immer 0x00
    uint16_t sector_count;  // +0x02: Anzahl Sektoren
    uint16_t dest_offset;   // +0x04: Ziel-Offset
    uint16_t dest_segment;  // +0x06: Ziel-Segment
    uint32_t lba_low;       // +0x08: LBA Bits 31:0
    uint32_t lba_high;      // +0x0C: LBA Bits 63:32 (bei uns immer 0)
};

struct __attribute__((packed)) drive_params_ext {
    uint16_t size;            // +0x00: Strukturgröße (0x1A)
    uint16_t flags;           // +0x02: Info-Flags
    uint32_t cylinders;       // +0x04: Anzahl Zylinder
    uint32_t heads;           // +0x08: Anzahl Köpfe
    uint32_t sectors;         // +0x0C: Sektoren pro Track
    uint32_t total_low;       // +0x10: Gesamtsektoren (Low)
    uint32_t total_high;      // +0x14: Gesamtsektoren (High)
    uint16_t bytes_per_sect;  // +0x18: Bytes pro Sektor
};

#endif