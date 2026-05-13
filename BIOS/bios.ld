/*
	Linker-Script for AMD Elan SC300 BIOS
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
	
	ROM-Layout (64kB):
	0x0000: Begin of ROM (mapped via ROMCS# to 0xF0000)
	....
	0xFFF0: Reset-Vector (mapped via ROMCS# to 0xFFFF0)
	0xFFFF: End of ROM (mapped via ROMCS# to 0xFFFFF)
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
	/* Placed at the very beginning of the ROM. Bare-Metal Assembler-Code */
    .text16 0x0000 : {
        KEEP(*(.text16))
        *(.text16.*)
    } > ROM

	/* C-Code */
    .text : {
        *(.text)
        *(.text.*)
    } > ROM

	/* Read-only data (Strings, Tables, etc.) */
    .rodata : { *(.rodata*) } > ROM
	/*.data : { *(.data*) } > ROM*/

    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    } > RAM

	/* Reset-Vector */
    .reset 0xFFF0 : {
        KEEP(*(.reset))
    } > ROM
}

ASSERT(SIZEOF(.reset) <= 16, "ERROR: Reset-Sector has more than 16 Bytes!")