# x86 BIOS for AMD Elan SC300 for the Behringer DDX3216
## General Information

I'm a huge fan of retro-computer and audio-technology. As I learned that the Behringer DDX3216 was built based on an x86-processor some of my neurons fired in my head and I pondered if I could boot software on this device. The DDX3216 uses the following hardware-components:

* AMD Elan SC300 386 Processor (an 386SX SoC with UART, PCMCIA, etc.)
* 27C512 64x8bit ROM IC (for BIOS)
* 8x HYB5117400BJ60 4Mx4bit RAM for total of 16MB DRAM
* 1x UM61256 SRAM (as Video-RAM)
* 4x 29C040-120 Flash-ICs for the main-software
* 4-bit LCD on SC300-internal LCD-interface (with 3x Toshiba T6A39 Col- and 1x T6A40 Row-Controller)
* TLC16C552 external UART (2 Serial-ports and 1x parallel port)
* PCMCIA-Connector for external CF-card-connection (with adapter)
* unassembled Intel 82078 FDC (Floppy Disk Controller) connected to a spare 34-pin connector

On my search for a working BIOS I learned that there is no free BIOS available for the AMD Elan SC300. I even requested an official quote at Phoenix and other manufacturers, but it seems that the SC400 and SC500 are more common as the SC300 is more than 33 years old.

So I started reading x86-documentation and the Programmers Reference Manual of the SC300 and started a new repo. Within only two weeks I managed to get the main-skeleton of a DOS-compatible x86-BIOS, that initializes the basic registers of the SC300, implements the most important hardware- and software-interrupts for the hardware and DOS. It also implements functions for reading the ROM, the CF-card on the PCMCIA-slot and other components like the external UART.

This is a screenshot of the boot-process:

![alt text](Documentation/firstboot.jpg)

These things are already working:
[x] Assembler-part that initializes the CPU and the most important parts like DRAM and SRAM
[x] external UART to communicate with a PC via RS232-cable
[x] LCD-display via the 4-bit LCD-interface of the SC300
[x] CF-card-interface to load the bootsector
[x] Bootsector successully calls INT13, INT10 and INT15 and loads IO.SYS of DOS 6.22


With date of 31th of May IO.SYS is loaded successfully and shows "Starting MS-DOS..." but then the system stopps loading COMMAND.COM. Maybe the disk-parameters are not send to IO.SYS correctly or another problem with the interrupts are still present. I'm working on this.

## Bootsectors
For the first tests I've implemented a very basic Bootsector that implements a very simple and short assembler-part that calls a C-function. Within C the external UART is used to output some demo-text. Actually there are two versions: a very simple bootdisk that stays in real-mode and a more advanced one that enables the protected mode that allows use to use the flat-memory-model of the 386-CPU.
But as I'm close to boot DOS 6.22 on this machine I will not work on these bootdisks anymore.

## Download x86-GCC-Compiler and -assembler:
https://github.com/lordmilko/i686-elf-tools/releases/tag/15.2.0

For the first tests I used NASM:
Download:
* https://www.nasm.us https://www.nasm.us/pub/nasm/releasebuilds/3.01

Documentation:
* https://www.nasm.us/docs/3.01/nasm09.html#section-9.1
* https://www.nasm.us/doc/nasm08.html