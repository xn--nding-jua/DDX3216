/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef DISK_H_
#define DISK_H_

#include "bios.h"

// PCMCIA registers
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

// IDE Register-Adressen
#define IDE_DATA        0x1F0   // Data Register (16-Bit)
#define IDE_ERROR       0x1F1   // Error Register (read)
#define IDE_FEATURES    0x1F1   // Features Register (write)
#define IDE_SECT_COUNT  0x1F2   // Sector Count
#define IDE_LBA_LOW     0x1F3   // LBA Bits 7:0
#define IDE_LBA_MID     0x1F4   // LBA Bits 15:8
#define IDE_LBA_HIGH    0x1F5   // LBA Bits 23:16
#define IDE_DRIVE_HEAD  0x1F6   // Drive/Head + LBA Bits 27:24
#define IDE_STATUS      0x1F7   // Status Register (read)
#define IDE_COMMAND     0x1F7   // Command Register (write)
#define IDE_ALT_STATUS  0x3F6   // Alternate Status (read, kein IRQ-Clear)
#define IDE_DEV_CTRL    0x3F6   // Device Control (write)

// IDE Status-Bits
#define IDE_SR_BSY      0x80    // Busy
#define IDE_SR_DRDY     0x40    // Drive Ready
#define IDE_SR_DRQ      0x08    // Data Request
#define IDE_SR_ERR      0x01    // Error

// IDE Kommandos
#define IDE_CMD_READ    0x20    // Read Sectors with Retry
#define IDE_CMD_WRITE   0x30    // Write Sectors with Retry
#define IDE_CMD_IDENT   0xEC    // Identify Drive

// CF-Karte 512MB Geometrie
// CHS für BIOS-Kompatibilität (DOS-Limit: 1024/16/63)
#define CF_CYLINDERS    1024
#define CF_HEADS        16
#define CF_SECTORS      63
#define CF_TOTAL_SECTS  ((uint32_t)CF_CYLINDERS * CF_HEADS * CF_SECTORS)

// LBA-Berechnung aus CHS
// LBA = (C × H_max + H) × S_max + (S - 1)
#define CHS_TO_LBA(c, h, s) \
    (((uint32_t)(c) * CF_HEADS + (h)) * CF_SECTORS + ((s) - 1))

bool pcmcia_init();
bool ide_wait_ready();
bool ide_wait_drq(void);
bool ide_read_bootsector();

struct __attribute__((packed)) disk_param_table {
    uint16_t cylinders;
    uint8_t  heads;
    uint16_t reserved1;
    uint16_t reserved2;
    uint8_t  sectors_per_track;
    uint16_t reserved3;
    uint8_t  drive_type;
};

static const struct disk_param_table hd0_params = {
    .cylinders       = CF_CYLINDERS,
    .heads           = CF_HEADS,
    .sectors_per_track = CF_SECTORS,
    .drive_type      = 0x00,
};

#endif