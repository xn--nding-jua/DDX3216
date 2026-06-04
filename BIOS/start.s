// BIOS for the AMD SC300 within the Behringer DDX3216
// (c) 2026 Chris Noeding, christian@noeding-online.de
// https://chrisdevblog.com
//
// This code initializes the AMD Elan SC300
// creates a stack and initializes the C-code
// the final product is a 64kB Image for the 27C512
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
// 0x80000 - 0x9BFFF | 112 kB     | EBDA (Extended BIOS Data Area)
// 0x9C000 - 0x9EFFF | 12 kB      | Global RAM for BIOS (grows from bottom to top)
// 0x9F000 - 0x9FFFF | 4 kB       | Reserved for BIOS-Stack (grows from top to bottom)
// -- high memory -----------------------------------------------------------
// 0xA0000 - 0xAFFFF | 64 kB      | Video-RAM for graphics
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
// 4. DataSegment DS and StackSegment SS are set to 0x0000, CodeSegment CS is kept at 0xF000 but C-Code and ISRs are still called from ROM_SEG
//

.intel_syntax noprefix
.code16

// segment-addresses
.equ ROM_SEG,         0xF000    // Segment where the ROM is mapped and where the BIOS code is located
.equ BIOS_SEG,        0x9C00    // Segment for global variables and stack during interrupts
.equ BIOS_STACK_TOP,  0x4000    // STACK-Pointer is 0x9C00 + 0x4000 = 0x9C000 + 0x4000 = 0xA0000
.equ BOOT_STACK_SEG,  0x0000    // only during initialization. Will be changed later to 0x9C00
.equ BOOT_STACK_TOP,  0x7C00    // STACK-Pointer during boot is 0x0000 + 0x7C00 = 0x00000 + 0x7C00 = 0x07C00

.equ CFG_ADDR,        0x22
.equ CFG_DATA,        0x23

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

    // Padding to the end and add date
    .zero (0x10 - (. - reset_vector) - 8)
    .ascii "06/04/26"           // MM/DD/YY

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
    mov     ax, BIOS_SEG
    mov     ds, ax
	mov		es, ax

	cld

    mov     di, __bss_start     // offset from linker-script relative to BIOS_SEG
    mov     cx, __bss_end       // offset from linker-script relative to BIOS_SEG
    sub     cx, di              // calc number of bytes
    jcxz    skip_bss            // no data in bss

    xor     al, al
    rep     stosb               // write zeros
skip_bss:
    // initialize the stack
    cli
    mov     ax, BIOS_SEG
    mov     ds, ax              // switch DS to RAM (0x9C00)
    mov     es, ax
    mov     ss, ax				// redirect stack-segment to 0x9C00
    mov     sp, BIOS_STACK_TOP  // set stack-pointer to desired offset of 0x4000 (right below 0x9C000 + 0x4000 = 0xA0000 = video-RAM)

start_c_code:
    // call the main-c-function of the BIOS
    jmp     bios_main           // defined in bios.c

    // we should never come back to here

halt:
    hlt                         // halt CPU
    jmp     halt




// ========================================================
// Launching Bootsector
// ========================================================
.global launch_bootsector
launch_bootsector:
    cli                         // disable interrupts
    
    // clear all segments to 0x0000
    xor ax, ax                  // set ax to 0x0000
    mov ds, ax
    mov es, ax
    mov ss, ax                  // optional: set Stack-Segment to 0x0000
    mov sp, BOOT_STACK_TOP     // stack-pointer for initial DOS right below bootsector at 0x0000:0x7C00

    // write the boot-drive to DL
    mov dl, 0x80

    sti                         // enable interrupts

    // far-jump to bootsector
    jmp 0x0000:0x7C00




// ========================================================
// GNU-Assembler Macros for ISR-Entries
// ========================================================
.macro ISR_HW_ENTRY cfunc
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

    // prepare segments for C-code: set DataSegment to BIOS_SEG to access global variables
    mov ax, BIOS_SEG
    mov ds, ax
    mov es, ax

    mov bp, sp
    mov ax, bp

    cld

    // call the C-Interrupt-Handler
    call \cfunc

    // force stackpointer back to original value
    mov sp, bp

    // send EndOfInterrupt (EOI) to PIC
    mov al, 0x20
    out 0x20, al

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

    iretw
.endm

.macro ISR_SW_ENTRY cfunc
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

    // prepare segments for C-code: set DataSegment to BIOS_SEG to access global variables
    mov ax, BIOS_SEG
    mov ds, ax
    mov es, ax

    mov bp, sp
    mov ax, bp

    cld

    // call the C-Interrupt-Handler
    call \cfunc

    // force stackpointer back to original value
    mov sp, bp

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

    iretw
.endm

.equ INT_FRAME_WORDS,  12

.macro ISR_SAFE_STACK_ENTRY cfunc, save_old_ss, save_old_sp, save_old_fs, save_old_gs, save_frame_sp
    cli

    push ax
    push fs

    // set FS to BIOS_SEG to safe variables
    mov ax, BIOS_SEG
    mov fs, ax

    pop ax // AX contains old FS
    mov WORD PTR fs:[\save_old_fs], ax

    mov ax, gs
    mov WORD PTR fs:[\save_old_gs], ax

    pop ax // restore original AX

    // safe registers to current caller-stack
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push es
    push ds
    // IP, CS, FLAGS already on stack from interrupt

    // safe old stack-state
    mov ax, ss
    mov WORD PTR fs:[\save_old_ss], ax
    mov ax, sp
    mov WORD PTR fs:[\save_old_sp], ax

    // change stack to safe-BIOS-stack
    mov ax, BIOS_SEG
    mov ss, ax
    mov sp, BIOS_STACK_TOP

    // set segments for C to allow usage of near-pointer
    mov ds, ax
    mov es, ax

    // reserve some space for register-frame on BIOS-stack
    sub sp, (INT_FRAME_WORDS * 2)
    mov di, sp
    mov WORD PTR fs:[\save_frame_sp], di

    // copy register-frame from DOS-Stack to BIOS-stack
    mov ax, WORD PTR fs:[\save_old_ss]
    mov gs, ax
    mov si, WORD PTR fs:[\save_old_sp]
    mov cx, INT_FRAME_WORDS

1:  // local variables for copy-loop (direction to C-function)
    mov ax, WORD PTR gs:[si]
    mov WORD PTR [di], ax
    add si, 2
    add di, 2
    loop 1b

    // call C-Handler via regparm(1) -> Struct-Pointer is in AX
    mov ax, WORD PTR fs:[\save_frame_sp]
    cld
    call \cfunc

    // for the case C has changed the segments: back to BIOS-stack
    mov ax, BIOS_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax

    // load framepointer from BIOS-stack
    mov si, WORD PTR fs:[\save_frame_sp]

    // modify frame back to original DOS-stack
    mov ax, WORD PTR fs:[\save_old_ss]
    mov gs, ax
    mov di, WORD PTR fs:[\save_old_sp]
    mov cx, INT_FRAME_WORDS

2:  // local variables for copy-loop (direction to DOS-functions)
    mov ax, WORD PTR [si]
    mov WORD PTR gs:[di], ax
    add si, 2
    add di, 2
    loop 2b

    // read original segment-configuration from memory
    mov ax, WORD PTR fs:[\save_old_ss]
    mov bx, WORD PTR fs:[\save_old_sp]
    //sub bx, (INT_FRAME_WORDS * 2)          // adjust stackpointer to point to original register-frame on DOS-stack with the updated register-values
    mov cx, WORD PTR fs:[\save_old_gs]
    mov dx, WORD PTR fs:[\save_old_fs]

    mov gs, cx
    mov fs, dx

    // change back to original DOS-stack
    cli
    mov ss, ax
    mov sp, bx

    // pop registers from restored original stack
    pop ds
    pop es
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax

    iretw
.endm


/*
.macro ISR_SAFE_STACK_ENTRY cfunc, save_old_ss, save_old_sp, save_old_fs, save_old_gs, save_frame_sp
    cli

    push ax
    push fs

    // set FS to BIOS_SEG to safe variables
    mov ax, BIOS_SEG
    mov fs, ax

    pop ax // AX contains old FS
    mov WORD PTR fs:[\save_old_fs], ax
    mov ax, gs
    mov WORD PTR fs:[\save_old_gs], ax

    pop ax // restore original AX

    // safe registers to current caller-stack
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push es
    push ds
    // IP, CS, FLAGS already on stack from interrupt

    // safe old stack-state
    mov ax, ss
    mov WORD PTR fs:[\save_old_ss], ax
    mov ax, sp
    mov WORD PTR fs:[\save_old_sp], ax

    // change stack to safe-BIOS-stack
    mov ax, BIOS_SEG
    mov ss, ax
    mov sp, BIOS_STACK_TOP

    // set segments for C to allow usage of near-pointer
    mov ds, ax
    mov es, ax

    // reserve some space for register-frame on BIOS-stack
    sub sp, (INT_FRAME_WORDS * 2)
    mov di, sp
    mov WORD PTR fs:[\save_frame_sp], di

    // copy register-frame from DOS-Stack to BIOS-stack
    mov ax, WORD PTR fs:[\save_old_ss]
    mov gs, ax
    mov si, WORD PTR fs:[\save_old_sp]
    mov cx, INT_FRAME_WORDS

1:  // local variables for copy-loop (direction to C-function)
    mov ax, WORD PTR gs:[si]
    mov WORD PTR [di], ax
    add si, 2
    add di, 2
    loop 1b

    // call C-Handler via regparm(1) -> Struct-Pointer is in AX/AX
    mov ax, WORD PTR fs:[\save_frame_sp]
    cld
    call \cfunc

    // for the case C has changed the segments: back to BIOS-stack
    mov ax, BIOS_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax

    // load framepointer from BIOS-stack
    mov sp, WORD PTR fs:[\save_frame_sp]

    // modify frame back to original DOS-stack
    mov ax, WORD PTR fs:[\save_old_ss]
    mov gs, ax

    mov si, sp
    mov di, WORD PTR fs:[\save_old_sp]
    mov cx, INT_FRAME_WORDS

2:  // local variables for copy-loop (direction to DOS-functions)
    mov ax, WORD PTR [si]
    mov WORD PTR gs:[di], ax
    add si, 2
    add di, 2
    loop 2b

    // read original segment-configuration from memory
    mov ax, WORD PTR fs:[\save_old_ss]
    mov bx, WORD PTR fs:[\save_old_sp]
    mov cx, WORD PTR fs:[\save_old_gs]
    mov dx, WORD PTR fs:[\save_old_fs]

    mov gs, cx
    mov fs, dx

    // change back to original DOS-stack
    cli
    mov ss, ax
    mov sp, bx

    // pop registers from restored original stack
    pop ds
    pop es
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax

    iretw
.endm
*/

// ========================================================
// Interrupt Service Routines
// ========================================================
// we leave 0x0100 bytes at segment 0x9C00 for the following variables

# storage for old register-values during safe-stack-ISR
.equ BIOS_SW_ISR_SS,        0x0020
.equ BIOS_SW_ISR_SP,        0x0022
.equ BIOS_SW_ISR_FS,        0x0024
.equ BIOS_SW_ISR_GS,        0x0026
.equ BIOS_SW_ISR_FRAME,     0x0028

.equ BIOS_INT15_SS,         0x002A
.equ BIOS_INT15_SP,         0x002C
.equ BIOS_INT15_FS,         0x002E
.equ BIOS_INT15_GS,         0x0030
.equ BIOS_INT15_FRAME,      0x0032

.global isr_int08
.global isr_int09
.global isr_int10
.global isr_int11
.global isr_int12
.global isr_int13
.global isr_int14
.global isr_int15
.global isr_int16
.global isr_int17
.global isr_int19
.global isr_int0c
.global isr_int1a
.global isr_int1c
.global isr_int_dummy

// hardware-interrupts with EOI
isr_int08:
    push ax
    push bx
    push ds

    xor ax, ax
    mov ds, ax

    // increase BIOS-timer-tick in BDA
    inc WORD PTR ds:[0x046C]
    jnz 1f                          // no overflow
    inc WORD PTR ds:[0x046E]
1:
    // check for 24h overflow (0x1800B0 Ticks)
    mov ax, WORD PTR ds:[0x046E]
    cmp ax, 0x0018
    jne 2f                          // no 24h overflow
    mov ax, WORD PTR ds:[0x046C]
    cmp ax, 0x00B0
    jne 2f                          // no 24h overflow

    // reset and set overflow-flag
    xor ax, ax
    mov WORD PTR ds:[0x046C], ax
    mov WORD PTR ds:[0x046E], ax
    mov BYTE PTR ds:[0x0470], 1     // overflow-flag in BDA
2:
    pop ds

    // set EOI before INT1C
    mov al, 0x20
    out 0x20, al

    // call user-interrupt INT 1C
    int 0x1C

    pop bx
    pop ax

    iretw

isr_int09:
    ISR_HW_ENTRY c_int09_handler // keyboard-interrupt

isr_int0c:
    ISR_HW_ENTRY c_int0c_handler // UART-interrupt

// software-interrupts without EOI
isr_int10:
    ISR_SW_ENTRY c_int10_handler

isr_int11:
    ISR_SW_ENTRY c_int11_handler

isr_int12:
    ISR_SW_ENTRY c_int12_handler

isr_int13:
    ISR_SAFE_STACK_ENTRY c_int13_handler, BIOS_SW_ISR_SS, BIOS_SW_ISR_SP, BIOS_SW_ISR_FS, BIOS_SW_ISR_GS, BIOS_SW_ISR_FRAME

isr_int14:
    ISR_SW_ENTRY c_int14_handler

isr_int15:
//    ISR_SW_ENTRY c_int15_handler
    ISR_SAFE_STACK_ENTRY c_int15_handler, BIOS_INT15_SS, BIOS_INT15_SP, BIOS_INT15_FS, BIOS_INT15_GS, BIOS_INT15_FRAME
    // TODO: check if we can move the stack-variables to general BIOS_SW_ISR_xxx lateron

isr_int16:
    ISR_SW_ENTRY c_int16_handler

isr_int17:
    ISR_SW_ENTRY c_int17_handler

isr_int19:
    ISR_SW_ENTRY c_int19_handler

isr_int1a:
    ISR_SW_ENTRY c_int1a_handler

isr_int1c:
    ISR_SW_ENTRY c_int1c_handler

isr_int_dummy:
    ISR_SW_ENTRY c_int_dummy_handler
