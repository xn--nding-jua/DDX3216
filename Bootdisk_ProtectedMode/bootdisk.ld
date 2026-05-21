OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(boot)

__bootsector_start  = 0x7C00; /* x86 BIOS copies the bootsector to this memory-address */
__bootsector_length = 0x200;  /* must be a single sector */
__diskimage_start   = __bootsector_start + __bootsector_length;
__diskimage_length  = 0x400;  /* 2 sectors */

MEMORY {
	BOOTSECTOR (rx)  : ORIGIN = __bootsector_start, LENGTH = __bootsector_length
	DISKIMAGE (rx)   : ORIGIN = __diskimage_start, LENGTH = __diskimage_length
}

SECTIONS
{
	/* put the assembler-part into the 512 byte bootsector and add magic-number 0xAA55 at the end */
    .text : {
        obj/boot.o(.text)
    } > BOOTSECTOR

    .magic __bootsector_start + __bootsector_length - 2 : {
        SHORT(0xAA55);
    } > BOOTSECTOR

	/* put C-code, constants, tables, strings, etc. in sector 2, 3, etc. */
	.text : {
		*(.text)
	} > DISKIMAGE

    .rodata : {
        *(.rodata)
    } > DISKIMAGE

    .data : {
        *(.data)
    } > DISKIMAGE

    .bss : {
        *(.bss)
    } > DISKIMAGE

	/* fill diskimage to the desired length */
	.eof __diskimage_start + __diskimage_length - 1 : {
		BYTE(0x00);
	} > DISKIMAGE
}
