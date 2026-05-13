# x86 BIOS for AMD Elan SC300 for the Behringer DDX3216

## 

## General Information



I'm a huge fan of retro-computer and audio-technology. As I learned that the Behringer DDX3216 was built based on an x86-processor some of my neurons fired in my head and I pondered if I could boot software on this device. The DDX3216 uses the following hardware-components:



* AMD Elan SC300 386 Processor
* 27C512 64x8bit ROM IC
* HYB5117400BJ60 RAM (8x) for 2MB DRAM
* TLC16C552 external UART
* PCMCIA-Connector
* 4-bit LCD on SC300-internal LCD-interface
* 4x 29C040-120 Flash-ICs for the main-software



On my search for a working BIOS I learned that there is no free BIOS available for the SC300. I requested a quote at Phoenix and other manufacturers, but it seems that the SC400 and SC500 are more common as the SC300 is more than 33 years old.



So I started reading x86-documentation and the Programmers Reference Manual of the SC300 and started a new repo... More information to come...



## Download assembler and compiler

### x86-Assembler:



Download:
https://www.nasm.us
https://www.nasm.us/pub/nasm/releasebuilds/3.01



Documentation:
https://www.nasm.us/docs/3.01/nasm09.html#section-9.1
https://www.nasm.us/doc/nasm08.html

### 

### x86-GCC-Compiler:

https://github.com/lordmilko/i686-elf-tools/releases/tag/15.2.0

