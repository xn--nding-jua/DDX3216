/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef BIOS_H_
#define BIOS_H_

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include "registers.h"
#include "inline.h"
#include "uart.h"
#include "lcd.h"
#include "disk.h"
#include "timer.h"
#include "isr.h"
#include "helper.h"
#include "keyboard.h"

#define LCD_BPP                 1      // bits per pixel (1 for text mode, 4 for graphics mode)
#define LCD_WIDTH               240    // real LCD resolution of the DDX3216
#define LCD_HEIGHT              64     // real vertical resolution of the LCD panel
#define VRAM_SIZE               ((LCD_WIDTH * LCD_BPP / 8) * (LCD_HEIGHT / 8) * 2)

static const char hex_chars[] = "0123456789ABCDEF";

struct __attribute__((packed)) ivt_entry {
    uint16_t offset;
    uint16_t segment;
};

struct __attribute__((packed)) sCursorPos {
    uint8_t col;
    uint8_t row;
};

struct __attribute__((packed)) bios_data_area {
    uint16_t com_ports[4];        // 0x0400: COM1 to COM4 I/O Port-Address
    uint16_t lpt_ports[3];        // 0x0408: LPT1 to LPT3 I/O Port-Address
    uint16_t ebda_base_address;   // EBDA-Address (0x040E)
    uint16_t equipment_word;      // 0x0410: Equipment Word
    uint8_t  res1;                // 0x0412: reserved
    uint16_t base_memory_kb;      // 0x0413: base-memory-size in kB (Default: 640 kB)
    uint8_t  padding1[2];         // 0x0415: placeholder until keyboard status flags at 0x0417
    uint8_t  kbd_status_flags1;   // 0x0417: Keyboard-Status-Flags (e.g. Numlock/Capslock active, etc.)
    uint8_t  kbd_status_flags2;  
    uint8_t  padding2[23];        // 0x0419: placeholder until video modes at 0x0430
    uint8_t  video_mode;          // 0x0449: Current Videomode (e.g. 0x03)
    uint16_t video_columns;       // 0x044A: Number of ROWs (e.g. 30 or 80)
    uint16_t video_page_size;     // 0x044C: Size of the current Video-Page in Bytes
    uint16_t video_page_start;    // 0x044E: Start-Offset of VRAM for current page (in Bytes, relative to 0xB8000)
    struct sCursorPos cursor_position[8];// 0x0450: Cursor-Positions for alle 8 possible Video-Pages (Row, Col)
    uint16_t cursor_type;         // 0x0460: Cursor-Typ (Start-/End-Scanline)   
    uint8_t  active_video_page;   // 0x0462: Active Video-Page index
    uint16_t crtc_port_address;   // 0x0463: I/O Port of Videochip (z.B. 0x3D4 für CGA)
    uint8_t  padding3[31];        // 0x0465: Placeholder until extended video area at 0x0480
    uint8_t  video_rows;          // 0x0484: Number of screen rows MINUS 1 (Important for your 8-row LCD!)
    uint16_t character_points;    // 0x0485: Bytes per character (Font height, e.g. 8)
    uint8_t  padding4[19];        // 0x0487: Placeholder until extended memory information
    uint16_t ext_memory_kb;       // 0x049B: Extended memory size in KB above 1MB (FFor your INT15)
};

struct __attribute__((packed)) bios_parameter_block {
    uint8_t  jmp_boot[3];         // 0x00: Assembler-Jump-Command (z.B. 0xEB 0x3C 0x90)
    char     oem_name[8];         // 0x03: Name of the OEM (z.B. "MSDOS5.0")
    
    // bootsector-structure
    uint16_t bytes_per_sector;    // 0x0B: Default = 512
    uint8_t  sectors_per_cluster; // 0x0D: Sectors per cluster (z.B. 2, 4, 8)
    uint16_t reserved_sectors;    // 0x0E: Sectors before first FAT (1 for MBR/bootsector)
    uint8_t  num_fats;            // 0x10: Number of FATs (should be 2)
    uint16_t root_dir_entries;    // 0x11: Max. Entries in the root directory (z.B. 512)
    uint16_t total_sectors_short; // 0x13: Total sectors (when < 65535, otherwise 0)
    uint8_t  media_descriptor;    // 0x15: Media type (z.B. 0xF8 for hard disk/CF card)
    uint16_t sectors_per_fat;     // 0x16: Size of FAT in sectors
    uint16_t sectors_per_track;   // 0x18: Sectors per track (important for INT 13h geometry!)
    uint16_t num_heads;           // 0x1A: Number of read/write heads
    uint32_t hidden_sectors;      // 0x1C: Sectors before Partition (LBA-Offset)
    uint32_t total_sectors_large; // 0x20: Total sectors (when 'total_sectors_short' == 0)
    
    // enhanced BPB (for FAT16)
    uint8_t  drive_number;        // 0x24: 0x00 for Floppy, 0x80 for HDD
    uint8_t  reserved;            // 0x25: Reserved (should be 0)
    uint8_t  extended_signature;  // 0x26: Most 0x29
    uint32_t volume_id;           // 0x27: Serialnumber of the volume (z.B. 0x12345678)
    char     volume_label[11];    // 0x2B: Name of Device (e.g. "NO NAME    ")
    char     file_system_type[8]; // 0x36: "FAT12   " or "FAT16   "
};

struct __attribute__((packed)) boot_sector {
    struct bios_parameter_block bpb;             // head of sector
    uint8_t                     boot_code[448];  // The actual bootloader code (e.g. DOS bootloader)
    uint16_t                    signature;       // Must be 0xAA55
};

#endif