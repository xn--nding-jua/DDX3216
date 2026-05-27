// BIOS for the AMD SC300 within the Behringer DDX3216
// (c) 2026 Chris Noeding, christian@noeding-online.de
// https://chrisdevblog.com
//
// This code initializes the AMD Elan SC300
// creates a stack and initializes the C-code
// the final product is a 64KB Image for the 27C512
//
// x86-Instructions: https://www.felixcloutier.com/x86/
//
// general memory-map of SC300
// start     end     | size       | description
// == conventional memory ===================================================
// 0x00000 - 0x003FF | 1 kB       | Real Mode IVT (Interrupt-Vector Table)  \
// 0x00400 - 0x004FF | 256 bytes  | BDA (BIOS Data Area)                    |
// 0x00500 - 0x07BFF | 29.7kB     | Conventional Memory (free RAM)          |--> 0x00000 ... 0x0FFFF = first 64kB Segment where we can use regular pointers as DS is at 0x0000
// 0x07C00 - 0x07DFF | 512 bytes  | OS Boot-Sector                          |
// 0x07E00 - 0x7FFFF | 32kB       | Conventional memory (free RAM)          /
// --------------------------------------------------------------------------
// 0x80000 - 0x9FFFF | 128 kB     | EBDA (Extended BIOS Data Area)
// -- high memory -----------------------------------------------------------
// 0xA0000 - 0xAFFFF | 64 kB      | optional BIOS
// 0xB0000 - 0xB7FFF | 32 kB      | External Video-SRAM in HGA-Mode
// 0xB8000 - 0xBFFFF | 32 kB      | External Video-SRAM in CGA-Mode (default)
// 0xC0000 - 0xC7FFF | 32 kB      | Video BIOS
// 0xC8000 - 0xEFFFF | 160 kB     | Expansion-ROMs
// 0xF0000 - 0xFFFFF | 64 kB      | Motherboard BIOS (we)
// -- extended memory -------------------------------------------------------
// 0x100000 - 0xFFFFFF | 16 MB | Extended Memory Area (see ref-manual on page 5-85 for /DOSCS-mapping)
// 0x100000 - 0x1FFFFF | 1 MB  | External DRAM Memory (RAM)
// 0xE00000 - 0xFEFFFF | 2 MB  | External Flash-Memory (RDOSSIZ[3..0] = 0010)
// 
// 
// memory-map of external Video-SRAM in CGA Text Mode
// start     end     | size   | description
// =============================================================================
// 0xB8000 - 0xBBFFF | 16 kB  | Display data
// 0xBC000 - 0xBCFFF | 4 kB   | Free
// 0xBD000 - 0xBDFFF | 4 kB   | Free
// 0xBE000 - 0xBEFFF | 4 kB   | Free
// 0xBF000 - 0xBF7FF | 2 kB   | Character fonts 2 (8x8)
// 0xBF800 - 0xBFFFF | 2 kB   | Character fonts 1 (8x8)
// 
// 
// 1. Reset-Vector is called at end of 4GB Address-range at 0xFFFFFFF0 (ROM is displayed here during "Un"real mode)
// 2. Far-Jump to address 0x000F0000 in ROM_SEG at 0xF000 (DS is set to ROM_SEG = 0xF000)
// 3. CPU is now in "real" 16-bit RealMode
// 4. After Startup, data (const, text, tables, etc.) is copied from ROM_SEG (0xF000) to STACK_SEG (0x0000) at Offset (0x0500)
// 5. DataSegment DS is set to 0x0000, CodeSegment CS is kept at 0xF000
// 6. C-Code and ISRs are still called from ROM_SEG but data is in 0x0500
// 
//
// Within the C-Code the "Un"real-Mode is used to access the RAM via 32-bit accesses
// we are using only FS/GS for the flat-access so that DS/SS is staying in 64kB-segment-mode
// as DOS-programs seems to use Segment-Wrap-Around-techniques
//

.intel_syntax noprefix
.code16

.extern __bss_start
.extern __bss_end
.extern bios_main            
.extern c_int08_handler
.extern c_int09_handler
.extern c_int10_handler
.extern c_int11_handler
.extern c_int12_handler
.extern c_int13_handler
.extern c_int14_handler
.extern c_int15_handler
.extern c_int16_handler
.extern c_int19_handler
.extern c_int0c_handler
.extern c_int1a_handler
.extern c_int1c_handler
.extern c_int_dummy_handler

// segment-addresses
.equ ROM_SEG,     0xF000
.equ STACK_SEG,   0x0000
.equ STACK_TOP,   0x7C00      
.equ CFG_ADDR,    0x22
.equ CFG_DATA,    0x23

// ========================================================
// Reset-Vector (must be at offset 0xFFF0 in ROM)
// ========================================================
// we are in the "Unreal" Mode / Reset-Mode
// CS (CodeSegment) is loaded to 0xFFFF:0000 (last 64kB segment of 32-bit address-mode)
// IP (InstructionPointer) is set to 0xFFF0
// So the very first assembler-command has to be at physical address 0xFFFF:FFF0
.section .reset, "ax"
.global reset_vector
reset_vector:
    nop                         // no-operation
    cli                         // disable interrupts
    jmp start                   // jump to beginning of the current segment (ROM_SEG)
	//jmp ROM_SEG:start          // far-jump to the beginning of the ROM-segment
    // Padding to the end and add date
    .zero (0x10 - (. - reset_vector) - 8)
    .ascii "05/24/26"

// ========================================================
// Startup-Function
// ========================================================
.section .text, "ax"
.global start
start:
    // initialize all segment-registers
    mov ax, 0xFF00
    mov ds, ax
    
    mov ax, 0x00F0
    mov ss, ax
    mov sp, 0x0100
    
    // ---------------------------------------------------------
    // setup internal registers of the SC300 using
    // port 22h for index and port 23h for data
    // ---------------------------------------------------------

    // Elan SC300 mandatory configuration
    // must be called after reset directly
sc300_init:
    // Index 0x0F: disables internal write-protection-mechanism and allows control of config-registers
    mov     al, 0x0F
    out     CFG_ADDR, al
    mov     al, 0xFF
    out     CFG_DATA, al

    // Index 0x44 (Misc4): Bit7=1, Bit6=0, Bit4=0, Bits2:0=0
    // mandatory: 1 0 x x 0 0 0 x -> 0x80
    mov     al, 0x44
    out     CFG_ADDR, al
    mov     al, 0x90            // Bit7=1, Bit4=1 (DISDEN suggested)
    out     CFG_DATA, al

    // Index 0x51 (ROM CFG2): Bits 3:2 = 00
    mov     al, 0x51
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x60 (Command Delay): Bit5=0, Bit2=0
    // 0 x 0 x 0 x x x -> both Bits 5 und 2 = 0
    mov     al, 0x60
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x62 (MMS Wait State 1): Bit7=0
    mov     al, 0x62
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x63 (Wait State Control): Bit1=0
    // for 33 MHz: Bits 6:5 setzen, Bit 1=0
    // 33 MHz: x 1 1 x x x 0 x -> 0x60 | recommended 0x0C
    // Standard (no specific frequency): Bit1=0
    mov     al, 0x63
    out     CFG_ADDR, al
    mov     al, 0x0C            // suggested: Bits 3:2 = 11 (INTIOWAIT, 16IOWAIT)
    out     CFG_DATA, al

    // Index 0x64 (Version): 1 0 0 x 1 1 x x -> 0x8C
    // Bit7=1, Bits 6:5=00, Bit4=0(EPMODE), Bits 3:2=11
    mov     al, 0x64
    out     CFG_ADDR, al
    mov     al, 0x8C
    out     CFG_DATA, al

    // Index 0x6A: reserved - must be 0x00
    mov     al, 0x6A
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x6B (Misc2): 0 x x 1 x x x x -> Bit4=1
    mov     al, 0x6B
    out     CFG_ADDR, al
    mov     al, 0x10            // Bit4=1
    out     CFG_DATA, al

    // Index 0x74 (MMSB Control): Bits 5:4 = 00
    mov     al, 0x74
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x80 (Power Control 1): Bit3=0
    mov     al, 0x80
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x8F (Clock Control): Bit7=1, Bit4=0
    // 1 x x 0 x x x x -> 0x80 | PLL-Startup-time
    // suggested: 256ms Restart-time (XST=110b = Bits 2:0 = 6)
    mov     al, 0x8F
    out     CFG_ADDR, al
    mov     al, 0x86            // Bit7=1, XST=110b (256ms PLL-Restart)
    out     CFG_DATA, al

    // Index 0x93: reserved - must be 0x00
    mov     al, 0x93
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x9D: reserved - must be 0x40
    mov     al, 0x9D
    out     CFG_ADDR, al
    mov     al, 0x40
    out     CFG_DATA, al

    // Index 0xB1 (Function Enable 2): Bit5=0
    // for 33 MHz: HSPLLFQ = 10b (66MHz CLK2) -> Bits 4:3 = 10
    // x x 0 1 0 x x x -> 0x08 for 33 MHz
    mov     al, 0xB1
    out     CFG_ADDR, al
    mov     al, 0x08            // 33 MHz: HSPLLFQ = 10b
    out     CFG_DATA, al

    // Index 0xB4 (PCMCIA Card Reset): Bit7=0 (Standard DRAM), Bit6=1 (muss!)
    mov     al, 0xB4
    out     CFG_ADDR, al
    mov     al, 0x40            // Bit6 must be 1 sein!
    out     CFG_DATA, al

    // additional mandatory settings for 33-MHz:

    // Index 0x60: x 0 0 x 0 x x x -> Bit6=0, Bit5=0, Bit2=0
    mov     al, 0x60
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x62 (MMS Wait State 1): 0 x x 1 x x x x -> Bit4=1 for 33MHz
    mov     al, 0x62
    out     CFG_ADDR, al
    mov     al, 0x10            // Bit4=1 (MISOUT) for 33 MHz
    out     CFG_DATA, al

    // Index 0x63 (Wait State Control): x 1 1 x x x 0 x -> Bits 6:5=11 for 33MHz
    mov     al, 0x63
    out     CFG_ADDR, al
    mov     al, 0x6C            // Bits 6:5=11 + Bits 3:2=11 (suggested)
    out     CFG_DATA, al

    // Index 0x65 (ROM CFG1): xx1x xxxx -> Bit5=1 (PFWS) for 33 MHz
    mov     al, 0x65
    out     CFG_ADDR, al
    mov     al, 0x20            // Bit5=1 (PFWS for 33 MHz Pflicht), ENROMF=1 inverted!
    // Attention: Bit0 (ENROMF) reads back inverted, we write a "0" to enable it
    out     CFG_DATA, al



	// initialize the DRAM
dram_init:
	// set DRAM-bank-size and type
    mov     al, 0x66        // Index: Memory Configuration 1
    out     0x22, al
    mov     al, 0x1B		// from original DDX3216-EPROM (we are using special multiplexing here: SA13 is connected to A10 of DRAM-ICs)
    out     0x23, al

    // set Wait-States for 33 MHz (we are using 60ns DRAM)
    mov     al, 0x63        // Index: Wait State Control
    out     0x22, al
    mov     al, 0x6C		// from original DDX3216-EPROM
    out     0x23, al
    
    mov     al, 0x62        // Index: MMS Memory Wait State 1
    out     0x22, al
    mov     al, 0x10        // Bit 4 = 1 (set Bank Miss to 5 WS for 33MHz)
    out     0x23, al

	// set refresh-rate
    mov     al, 0x64        // Index: Version / Refresh Rate
    out     0x22, al
    mov     al, 0x9C		// from original DDX3216-EPROM
    out     0x23, al

    // enable refresh
    mov     al, 0xA7        // Index: PMU Control 1
    out     0x22, al
    mov     al, 0x32		// from original DDX3216-EPROM
    out     0x23, al

dram_init_delay:
    mov     cx, 200         // set loop-counter (200x ~1us)
.dram_wait_loop:
    mov     al, 0xAA
    out     0x80, al        // output 0xAA to port 0x80 (takes around 1us)
    dec     cx
    jnz     .dram_wait_loop // loop until counter is zero



clear_bss:
    // clear data in BSS to zero
    mov     ax, STACK_SEG
    mov     ds, ax

    mov     di, __bss_start
    mov     cx, __bss_end
    sub     cx, di              // calc number of bytes
    jcxz    skip_bss            // no data in bss
    xor     al, al
    rep     stosb               // write zeros
skip_bss:
    // initialize the stack
    mov     ax, STACK_SEG
    mov     ds, ax              // keep DS at RAM (0x0000)
    mov     es, ax
    mov     ss, ax				// redirect stack-segment to 0x0000
    mov     sp, STACK_TOP		// set stack-pointer to desired 0x7BFF

start_c_code:
    // call the main-c-function of the BIOS
    jmp     bios_main           // defined in main.c

    // we should never come back to here

halt:
    hlt                         // CPU anhalten
    jmp     halt


// ========================================================
// GNU-Assembler Macro for ISR-Entries
// ========================================================
.macro ISR_ENTRY cfunc
//    cli
//    cld

    // store all 16-bit registers
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push es
    push ds

    // prepare segments for C-code (DS = SS) and store
    // stackpointer in register SI that cannot be changed by C-Code
    mov ax, ss
    mov ds, ax
    mov es, ax
    mov si, sp
    mov ax, sp

    // call the C-Interrupt-Handler
    call \cfunc

    // force stackpointer back to original value
    mov sp, si

    // send EndOfInterrupt (EOI) to PIC
    mov dx, 0x0020
    mov al, 0x20
    out dx, al

    // restore all registers in inverted order
    pop ds
    pop es
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax

//    sti
    iret
.endm

// ========================================================
// Launching Bootsector
// ========================================================
.global launch_bootsector
launch_bootsector:
    cli                         // disable interrupts
    
    // Segmente kompromisslos nullen
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax                  // Optional: Stack-Segment to 0x0000
    mov sp, 0x7C00              // stack-pointer below boot-sector

    // write the boot-drive to DL
    mov dl, 0x80

    // far-jump to bootsector
    .byte 0xEA
    .word 0x7C00
    .word 0x0000

// ========================================================
// Interrupt Service Routines
// ========================================================
.global isr_int08
.global isr_int09
.global isr_int10
.global isr_int11
.global isr_int12
.global isr_int13
.global isr_int14
.global isr_int15
.global isr_int16
.global isr_int19
.global isr_int0c
.global isr_int1a
.global isr_int1c
.global isr_int_dummy

isr_int08:
    ISR_ENTRY c_int08_handler

isr_int09:
    ISR_ENTRY c_int09_handler

isr_int10:
    ISR_ENTRY c_int10_handler

isr_int11:
    ISR_ENTRY c_int11_handler

isr_int12:
    ISR_ENTRY c_int12_handler

isr_int13:
    ISR_ENTRY c_int13_handler

isr_int14:
    ISR_ENTRY c_int14_handler

isr_int15:
    ISR_ENTRY c_int15_handler

isr_int16:
    ISR_ENTRY c_int16_handler

isr_int19:
    ISR_ENTRY c_int19_handler

isr_int0c:
    ISR_ENTRY c_int19_handler

isr_int1a:
    ISR_ENTRY c_int19_handler

isr_int1c:
    ISR_ENTRY c_int19_handler

isr_int_dummy:
    ISR_ENTRY c_int_dummy_handler
