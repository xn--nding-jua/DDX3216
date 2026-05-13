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
	│                 │            │   .data (VMA)   │ ← DS=0x0000
	├─────────────────┤ ~0x16BC    │   globale Vars  │   reachable
	│   .data  (LMA)  │──────────> ├─────────────────┤
	│   .rodata(LMA)  │  kopiert   │   .rodata (VMA) │ ← DS=0x0000
	│                 │            │   Strings etc.  │   reachable
	├─────────────────┤            ├─────────────────┤
	│   (free)        │            │   .bss          │ ← zeroed
	├─────────────────┤ 0xFFF0     ├─────────────────┤ 0x7C00
	│   .reset        │            │   Stack \/      │ ← SS=0x0000
	└─────────────────┘ 0xFFFF     └─────────────────┘
*/

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(reset_vector)

MEMORY {
	/* Destination: EPROM-IC 27C512 with 64kB */
    ROM (rx)  : ORIGIN = 0x0000, LENGTH = 64K
	
	/* RAM Area starting at 0x0500 behind the BIOS Data Area */
    RAM (wa)  : ORIGIN = 0x0500, LENGTH = 30K
}

SECTIONS {
	/* Bare-Metal Assembler and C-Code at the very beginning of the ROM */
    .text16 : {
        __text_start = .;
        KEEP(*(.text16))
        *(.text16.*)
        *(.text)
        *(.text.*)
        . = ALIGN(4);
        __text_end = .;
    } > ROM

	/* Strings, Tables, Data, etc. */
    /* Virtual Memory Address (VMA) is at 0x0500 in RAM */
    /* Loader Memory Address (LMA) is at __text_end in ROM */
    .data : {
        __data_start = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        __data_end = .;
    } > RAM AT> ROM /* this produces an overlap-error in the linker, but we will disable this check */

    .rodata : {
        __rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
        __rodata_end = .;
    } > RAM AT> ROM /* this produces an overlap-error in the linker, but we will disable this check */

    __data_size = __rodata_end - __data_start;

    /* uninitialized data (zeroed at runtime, not in ROM) */
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM

	/* Reset-Vector */
    .reset 0xFFF0 : {
        KEEP(*(.reset))
    } > ROM
}

/* Sanity Checks */
ASSERT(SIZEOF(.reset) <= 16, "ERROR: Reset-Sector has more than 16 Bytes!")
