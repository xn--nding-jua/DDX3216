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

#define BIOS_DEBUG              0
#define BIOS_MANUAL_CHS         1
    #define CF_CYLINDERS            1014
    #define CF_HEADS                16
    #define CF_SECTORS              63

#define BIOS_RESERVED_KB        16 // 16kB are reserved for our BIOS (global variables and stack)
#define BIOS_CONVENTIONAL_KB    (640 - BIOS_RESERVED_KB)

#define BIOS_SEG                0x9C00
#define BIOS_STACK_TOP          0x4000

#define BIOS_DATA_START_OFF     0x0100 // start in RAM for global variables
#define BIOS_DATA_END_OFF       0x2000 // end of RAM
#define BIOS_STACK_LOW_OFF      0x2000 // end of Stack
#define BIOS_STACK_HIGH_OFF     0x4000 // begin of Stack

#define LCD_BPP                 1      // bits per pixel (1 for text mode, 4 for graphics mode)
#define LCD_WIDTH               240    // real LCD resolution of the DDX3216
#define LCD_HEIGHT              64     // real vertical resolution of the LCD panel
#define VRAM_SIZE               ((LCD_WIDTH * LCD_BPP / 8) * (LCD_HEIGHT / 8) * 2)


// ISRs from Assembler-Part
extern void launch_bootsector(void) __attribute__((cdecl));
extern void launch_basic(void) __attribute__((cdecl));

extern void isr_int08(void);
extern void isr_int09(void);
extern void isr_int0c(void);
extern void isr_int10(void);
extern void isr_int11(void);
extern void isr_int12(void);
extern void isr_int13(void);
extern void isr_int14(void);
extern void isr_int15(void);
extern void isr_int16(void);
extern void isr_int17(void);
extern void isr_int19(void);
extern void isr_int1a(void);
extern void isr_int1c(void);
extern void isr_int29(void);

extern void isr_spurious_irq7(void);
extern void isr_spurious_irq15(void);

extern void isr_int_error(void);
extern void isr_hw_int_dummy(void);
extern void isr_sw_int_dummy(void);

extern struct floppy_param_table fd0_params;
extern struct disk_param_table hd0_params;

// function prototypes
void cpu_reset();
void boot_dos();
bool a20_is_enabled();
void a20_enable();
void a20_disable();

static const char hex_chars[] = "0123456789ABCDEF";

struct ivt_entry {
    uint16_t offset;
    uint16_t segment;
} __attribute__((packed));

struct sCursorPos {
    uint8_t col;
    uint8_t row;
} __attribute__((packed));

struct bios_parameter_block {
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
} __attribute__((packed));

struct boot_sector {
    struct bios_parameter_block bpb;             // head of sector
    uint8_t                     boot_code[448];  // The actual bootloader code (e.g. DOS bootloader)
    uint16_t                    signature;       // Must be 0xAA55
} __attribute__((packed));

struct system_config {
    uint16_t size;
    uint8_t computer_type;
    uint8_t model;
    uint8_t bios_revision;
    uint8_t feature_information;
    uint8_t feature_2;
    uint8_t reserved_1;
    uint8_t reserved_2;
    uint8_t reserved_3;
} __attribute__((packed));
struct system_config sysconfig; // global variable

#endif