OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(boot)

MEMORY {
	BOOTSECTOR (rx)  : ORIGIN = 0x7C00, LENGTH = 512
}

SECTIONS
{
    .text : {
        obj/boot.o(.text)
        *(.text)
    } > BOOTSECTOR

    .rodata : {
        *(.rodata)
    } > BOOTSECTOR

    .data : {
        *(.data)
    } > BOOTSECTOR

    .bss : {
        *(.bss)
    } > BOOTSECTOR

    .magic 0x7DFE : {
        SHORT(0xAA55);
    } > BOOTSECTOR
}
