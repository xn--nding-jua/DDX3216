/*
	Linker-Script for AMD Elan SC300 BIOS
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
	
	ROM (after linking):           RAM (runtime):
	┌─────────────────┐ 0x0000     ┌─────────────────┐ 0x0000
	│   .text16       │            │   IVT           │
	│   (Code)        │            ├─────────────────┤ 0x0400
	│   CS=0xF000     │            │   BDA           │
	│   will be run   │            ├─────────────────┤ 0x0500
	│                 │            │   .bss          │
	├─────────────────┤ ~0x16BC    │   global Vars   │
	│   .data  (LMA)  │            |                 |
	│   .rodata(LMA)  │            │                 │
	│                 │            │   growing dir   │
	├─────────────────┤            │       /\        │
	│   (free)        │            │   StackPointer  │
	├─────────────────┤ 0xFFF0     ├─────────────────┤ 0x7C00
	│   .reset        │            │                 │
	└─────────────────┘ 0xFFFF     └─────────────────┘
*/

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(reset_vector)

MEMORY {
	/* Destination: EPROM-IC 27C512 with 64kB */
    ROM (rx)  : ORIGIN = 0xFFFF0000, LENGTH = 64K

    /* RAM Area in DS-Segment (0x9F80) starting at offset 0x0100 with ~1kB in size */
    BIOSRAM (wa)  : ORIGIN = 0x0100, LENGTH = 0x0300 /* leave 0x0100 bytes for some global variables */
}

SECTIONS {
	/* Bare-Metal Assembler and C-Code at the very beginning of the ROM */
    .text : {
        __text_start = .;
        KEEP(*(.text))
        *(.text.*)
        . = ALIGN(2);
        __text_end = .;
    } > ROM

    .rodata : {
        __rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(2);
        __rodata_end = .;
    } > ROM

    /* We are not allowing data in BIOS as it won't be accessible from */
    /* DataSegment. So we raise an error as soon as we are declaring a */
    /* global variable with preloaded values. */
	/* preloaded global variables, etc. */
    .data : {
        __data_start = .;
        *(.data)
        *(.data.*)
        . = ALIGN(2);
        __data_end = .;
    } > ROM

    /* global variables into RAM, not ROM */
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(2);
        __bss_end = .;
    } > BIOSRAM

	/* Reset-Vector at very end of the ROM-image */
    .reset 0xFFFFFFF0 : {
        KEEP(*(.reset))
    } > ROM
}

/* Sanity Checks */
ASSERT(SIZEOF(.reset) <= 16, "ERROR: Reset-Sector has more than 16 Bytes!")
ASSERT(SIZEOF(.data) == 0,   "ERROR: DataSection not zero - found preloaded variable!")
ASSERT(__bss_end <= 0x2000,  "ERROR: BIOS .bss exceeds data area 0x0000..0x1FFF!")
