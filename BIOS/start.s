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
.equ BASIC_SEG,       0x2000    // Segment for tiny8086basic
.equ BIOS_SEG,        0x9F80    // Segment for global variables and stack during interrupts
.equ BIOS_STACK_TOP,  0x0800    // STACK-Pointer is 0x9F80 + 0x0800 = 0x9F800 + 0x0800 = 0xA0000
.equ BOOT_STACK_SEG,  0x0000    // only during initialization. Will be changed later to 0x9F80
.equ BOOT_STACK_TOP,  0x7C00    // STACK-Pointer during boot is 0x0000 + 0x7C00 = 0x00000 + 0x7C00 = 0x07C00

.equ CFG_ADDR,        0x22
.equ CFG_DATA,        0x23

.equ BASIC_SIZE_WORDS, 2048     // size of basic BIOS in words (2048 words = 4096 bytes = 4 kB)

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
    
    mov ax, BOOT_STACK_SEG
    mov ss, ax
    mov sp, BOOT_STACK_TOP
    
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
    mov     al, 0b10010000      // Bit7=1, Bit4=1 (DISDEN suggested)
    out     CFG_DATA, al

    // Index 0x51 (ROM CFG2): Bits 3:2 = 00
    mov     al, 0x51
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x60 (Waitstates to maximum (default)):
    mov     al, 0x60
    out     CFG_ADDR, al
    mov     al, 0b00000000
    out     CFG_DATA, al

    // Index 0x62 (MMS Wait State to 5 (default)):
    mov     al, 0x62
    out     CFG_ADDR, al
    mov     al, 0b00010000      // select 5 waitstates
    out     CFG_DATA, al

    // Index 0x63 (Wait State Control)
    mov     al, 0x63
    out     CFG_ADDR, al
    mov     al, 0b01100100      // set internal IO WS to 4 WS, set 16-bit IO WS to 3WS
    out     CFG_DATA, al

    // Index 0x64 (Version)
    // Bit7=1, Bits 6:5=00, Bit4=0(EPMODE), Bits 3:2=11
    mov     al, 0x64
    out     CFG_ADDR, al
    mov     al, 0b10011100      // b7 must be 1, bit6/5 must be 0, bit 3/2 must be 1
    out     CFG_DATA, al

    // Index 0x65 (ROM CFG1)
    mov     al, 0x65
    out     CFG_ADDR, al
    mov     al, 0b00100000      // Bit5=1 (PFWS for 33 MHz Pflicht), ENROMF=1 inverted!
    // Attention: Bit0 (ENROMF) reads back inverted, we write a "0" to enable it
    out     CFG_DATA, al

    // Index 0x6A: reserved - must be 0x00
    mov     al, 0x6A
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x6B (Misc2)
    mov     al, 0x6B
    out     CFG_ADDR, al
    mov     al, 0b00010000      // according to ReferenceManual: Bit 4 must be 1, Bit 2 must be 0 for 25/33MHz
    out     CFG_DATA, al

    // Index 0x74 (MMSB Control)
    mov     al, 0x74
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x80 (Power Control 1)
    mov     al, 0x80
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x8F (Clock Control)
    // suggested: 256ms Restart-time (XST=110b = Bits 2:0 = 6)
    mov     al, 0x8F
    out     CFG_ADDR, al
    mov     al, 0b10000100       // Bit7=1, XST=110b (256ms PLL-Restart)
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
    mov     al, 0xB1
    out     CFG_ADDR, al
    mov     al, 0b00010000      // 33 MHz: HSPLLFQ = 10b
    out     CFG_DATA, al

    // Index 0xB4 (PCMCIA Card Reset): Bit7=0 (Standard DRAM)
    mov     al, 0xB4
    out     CFG_ADDR, al
    mov     al, 0b01000000      // Bit6 must be 1!
    out     CFG_DATA, al

    mov     cx, 0x7530
1:  // wait for hardware to get up
    nop
    loop    1b

    // additional mandatory settings for 33-MHz:

    // Index 0x50 (MMS Waitstate)
    mov     al, 0x50
    out     CFG_ADDR, al
    mov     al, 0b01100110      // ROMDOS Command Delay, PCMCIA Waitstate enable to 4 WS, ROMDOS Waitstate enabled to 2 WS
    out     CFG_DATA, al

    // Index 0x51 (ROM Configuration 2)
    mov     al, 0x51
    out     CFG_ADDR, al
    mov     al, 0b00000010      // enable 16-bit access for ROMDOS
    out     CFG_DATA, al

    // Index 0x60
    mov     al, 0x60
    out     CFG_ADDR, al
    mov     al, 0x00
    out     CFG_DATA, al

    // Index 0x61 (IO Waitstates)
    mov     al, 0x61
    out     CFG_ADDR, al
    mov     al, 0b00110000      // IO Waitstates to 2 WS
    out     CFG_DATA, al

    // Index 0x62 (MMS Wait State 1)
    mov     al, 0x62
    out     CFG_ADDR, al
    mov     al, 0b00010000      // Bit4=1 (MISOUT) for 33 MHz
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

	// set memory-type
    mov     al, 0x70        // Misc 6 register
    out     0x22, al
    mov     al, 0x00		// select DRAM and set PGP0 to input
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


    // more registers

    // set drive strength
    mov     al, 0xB9        // Memory Configuration 2 Register
    out     0x22, al
    mov     al, 0b00010001  // Strength C for MemDat and RAS#, Strength E for MA, SA and MWE#
    out     0x23, al

    // set drive strength
    mov     al, 0xB3        // Misc 5 Register
    out     0x22, al
    mov     al, 0x00        // Everything to default values
    out     0x23, al

    // Index 0x61 (IO Waitstates)
    mov     al, 0x61
    out     CFG_ADDR, al
    mov     al, 0b01110000      // Enable CPU HighSpeed, IO Waitstates to 2 WS
    out     CFG_DATA, al

    // Index 0x62 (MMS Wait State):
    mov     al, 0x62
    out     CFG_ADDR, al
    mov     al, 0b00011111      // select 5 waitstates, 2 WS for 8-bit ISA, 1 WS for 16-bit ISA
    out     CFG_DATA, al


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
    mov ss, ax                 // optional: set Stack-Segment to 0x0000
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
    cld

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
    .code16gcc
    call \cfunc
    .code16

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
.endm

.equ INT_FRAME_WORDS,  12

.macro ISR_SAFE_STACK_ENTRY cfunc, save_old_ss, save_old_sp, save_old_fs, save_old_gs, save_frame_sp
    cld
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
    cld

1:  // local variables for copy-loop (direction to C-function)
    mov ax, WORD PTR gs:[si]
    mov WORD PTR [di], ax
    add si, 2
    add di, 2
    loop 1b

    // call C-Handler via regparm(1) -> Struct-Pointer is in AX
    mov ax, WORD PTR fs:[\save_frame_sp]
    .code16gcc
    call \cfunc
    .code16

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
    cld

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
.endm

// ========================================================
// Interrupt Service Routines
// ========================================================
// we leave 0x0100 bytes at segment 0x9C00 for the following variables

# storage for old register-values during safe-stack-ISR
.equ BIOS_SW_ISR_SS,            0x0020
.equ BIOS_SW_ISR_SP,            0x0022
.equ BIOS_SW_ISR_FS,            0x0024
.equ BIOS_SW_ISR_GS,            0x0026
.equ BIOS_SW_ISR_FRAME,         0x0028

.equ BIOS_INT08_ISR_SS,         0x002A
.equ BIOS_INT08_ISR_SP,         0x002C
.equ BIOS_INT08_ISR_FS,         0x002E
.equ BIOS_INT08_ISR_GS,         0x0030
.equ BIOS_INT08_ISR_FRAME,      0x0032

.equ BIOS_INT10_ISR_SS,         0x0034
.equ BIOS_INT10_ISR_SP,         0x0036
.equ BIOS_INT10_ISR_FS,         0x0038
.equ BIOS_INT10_ISR_GS,         0x003A
.equ BIOS_INT10_ISR_FRAME,      0x003C

.equ BIOS_INT13_ISR_SS,         0x003E
.equ BIOS_INT13_ISR_SP,         0x0040
.equ BIOS_INT13_ISR_FS,         0x0042
.equ BIOS_INT13_ISR_GS,         0x0044
.equ BIOS_INT13_ISR_FRAME,      0x0046

.equ BIOS_INT15_ISR_SS,         0x0048
.equ BIOS_INT15_ISR_SP,         0x004A
.equ BIOS_INT15_ISR_FS,         0x004C
.equ BIOS_INT15_ISR_GS,         0x004E
.equ BIOS_INT15_ISR_FRAME,      0x0050

.equ BIOS_INT16_ISR_SS,         0x0052
.equ BIOS_INT16_ISR_SP,         0x0054
.equ BIOS_INT16_ISR_FS,         0x0056
.equ BIOS_INT16_ISR_GS,         0x0058
.equ BIOS_INT16_ISR_FRAME,      0x005A

// timer-interrupts
.global isr_int08
isr_int08:
    cld

    push ax
    push bx
    push cx
    push dx
    push ds

    // set DS to 0x0000 to access BDA at 0x0000:0x0400
    xor ax, ax
    mov ds, ax

    // increase BIOS-timer-tick in BDA
    inc WORD PTR ds:[0x046C]
    jnz .timer_no_overflow           // no overflow
    inc WORD PTR ds:[0x046E]
.timer_no_overflow:
    // check for 24h overflow (0x1800B0 Ticks)
    mov ax, WORD PTR ds:[0x046E]
    cmp ax, 0x0018
    jne .timer_no_24h_wrap           // no 24h overflow
    mov ax, WORD PTR ds:[0x046C]
    cmp ax, 0x00B0
    jne .timer_no_24h_wrap           // no 24h overflow

    // reset and set overflow-flag
    xor ax, ax
    mov WORD PTR ds:[0x046C], ax
    mov WORD PTR ds:[0x046E], ax
    mov BYTE PTR ds:[0x0470], 1     // overflow-flag in BDA
.timer_no_24h_wrap:

    // handle INT15, Function 0x83
    mov al, BYTE PTR ds:[0x04A0]    // check BDA_WAIT_ACTIVE_FLAG
    test al, 0x01                   // check if bit 1 is set
    jz .timer_done                  // nothing to do here

    mov ax, WORD PTR ds:[0x049C]    // load counter LOW
    mov dx, WORD PTR ds:[0x049E]    // load counter HIGH

    mov cx, ax
    or  cx, dx
    jz .check_timer_done            // if CX == 0, counter was 0

    // decrement counter
    sub ax, 1
    sbb dx, 0
    mov WORD PTR ds:[0x049C], ax    // write back counter LOW
    mov WORD PTR ds:[0x049E], dx    // write back counter HIGH

.check_timer_done:
    mov cx, ax
    or  cx, dx
    jnz .timer_done                       // if not zero -> done

    // timer has finished
    mov bx, WORD PTR ds:[0x0498]
    mov cx, WORD PTR ds:[0x049A]
    mov es, cx                      // set user-segment to ES

    // set bit 7 in user-segment
    mov al, es:[bx]
    or al, 0x80
    mov es:[bx], al                 // write value back

    // reset WAIT ACTIVE FLAG bit
    mov BYTE PTR ds:[0x04A0], 0x00     // BDA_WAIT_ACTIVE_FLAG = 0

.timer_done:
    mov al, 0x20
    out 0x20, al

    pop ds
    pop dx
    pop cx
    pop bx
    pop ax

    // call user-interrupt INT 1C
    int 0x1C

    iret

.global isr_int1c
isr_int1c:
    iret

// hardware-interrupts with EOI

.global isr_int09
isr_int09:
    ISR_HW_ENTRY c_int09_handler // keyboard-interrupt
    iret

.global isr_int0c
isr_int0c:
    ISR_HW_ENTRY c_int0c_handler // UART-interrupt
    iret


// software-interrupts without EOI

.global isr_int10
isr_int10:
    ISR_SAFE_STACK_ENTRY c_int10_handler, BIOS_INT10_ISR_SS, BIOS_INT10_ISR_SP, BIOS_INT10_ISR_FS, BIOS_INT10_ISR_GS, BIOS_INT10_ISR_FRAME
    iret

.global isr_int11
isr_int11:
    ISR_SAFE_STACK_ENTRY c_int11_handler, BIOS_SW_ISR_SS, BIOS_SW_ISR_SP, BIOS_SW_ISR_FS, BIOS_SW_ISR_GS, BIOS_SW_ISR_FRAME
    iret

.global isr_int12
isr_int12:
    ISR_SAFE_STACK_ENTRY c_int12_handler, BIOS_SW_ISR_SS, BIOS_SW_ISR_SP, BIOS_SW_ISR_FS, BIOS_SW_ISR_GS, BIOS_SW_ISR_FRAME
    iret

.global isr_int13
isr_int13:
    ISR_SAFE_STACK_ENTRY c_int13_handler, BIOS_INT13_ISR_SS, BIOS_INT13_ISR_SP, BIOS_INT13_ISR_FS, BIOS_INT13_ISR_GS, BIOS_INT13_ISR_FRAME
    iret

.global isr_int14
isr_int14:
    ISR_SAFE_STACK_ENTRY c_int14_handler, BIOS_SW_ISR_SS, BIOS_SW_ISR_SP, BIOS_SW_ISR_FS, BIOS_SW_ISR_GS, BIOS_SW_ISR_FRAME
    iret

.global isr_int15
isr_int15:
    ISR_SAFE_STACK_ENTRY c_int15_handler, BIOS_INT15_ISR_SS, BIOS_INT15_ISR_SP, BIOS_INT15_ISR_FS, BIOS_INT15_ISR_GS, BIOS_INT15_ISR_FRAME
    iret

.global isr_int16
isr_int16:
    ISR_SAFE_STACK_ENTRY c_int16_handler, BIOS_INT16_ISR_SS, BIOS_INT16_ISR_SP, BIOS_INT16_ISR_FS, BIOS_INT16_ISR_GS, BIOS_INT16_ISR_FRAME
    iret

.global isr_int17
isr_int17:
    ISR_SAFE_STACK_ENTRY c_int17_handler, BIOS_SW_ISR_SS, BIOS_SW_ISR_SP, BIOS_SW_ISR_FS, BIOS_SW_ISR_GS, BIOS_SW_ISR_FRAME
    iret

.global isr_int18
isr_int18:
    jmp launch_basic

.global isr_int19
isr_int19:
    ISR_SAFE_STACK_ENTRY c_int19_handler, BIOS_SW_ISR_SS, BIOS_SW_ISR_SP, BIOS_SW_ISR_FS, BIOS_SW_ISR_GS, BIOS_SW_ISR_FRAME
    iret

.global isr_int1a
isr_int1a:
    ISR_SAFE_STACK_ENTRY c_int1a_handler, BIOS_SW_ISR_SS, BIOS_SW_ISR_SP, BIOS_SW_ISR_FS, BIOS_SW_ISR_GS, BIOS_SW_ISR_FRAME
    iret

.global isr_int29
isr_int29:
    ISR_SAFE_STACK_ENTRY c_int29_handler, BIOS_SW_ISR_SS, BIOS_SW_ISR_SP, BIOS_SW_ISR_FS, BIOS_SW_ISR_GS, BIOS_SW_ISR_FRAME
    iret


// special interrupts

.global isr_spurious_irq7
isr_spurious_irq7:
    // Spurious IRQ7: dont send EOI here
    // check PIC ISR-register if it is really spurious
    push ax
    
    mov al, 0x0B        // OCW3: Read ISR
    out 0x20, al
    in al, 0x20         // read ISR
    test al, 0x80       // Bit 7 = IRQ7?
    jnz 1f              // Echter IRQ7: send EOI
    
    // its spurious: dont send EOI
    
    pop ax

    iret

1:
    mov al, 0x20        // Echter IRQ7: EOI
    out 0x20, al

    pop ax
    
    iret

.global isr_spurious_irq15
isr_spurious_irq15:
    // Spurious IRQ15: send EOI only to master

    push ax

    mov al, 0x0B
    out 0xA0, al        // read slave ISR
    in al, 0xA0
    test al, 0x80
    jnz 1f

    // spurious: just master
    mov al, 0x20
    out 0x20, al

    pop ax

    iret

1:
    mov al, 0x20
    out 0xA0, al        // Slave EOI
    out 0x20, al        // Master EOI

    pop ax

    iret

.global isr_int_error
isr_int_error:
    ISR_SAFE_STACK_ENTRY c_int_error_handler, BIOS_SW_ISR_SS, BIOS_SW_ISR_SP, BIOS_SW_ISR_FS, BIOS_SW_ISR_GS, BIOS_SW_ISR_FRAME 
    iret

.global isr_hw_int_dummy
isr_hw_int_dummy:
	push ax
	mov al, 0x20
	out 0x20, al
	pop ax

    iret

.global isr_sw_int_dummy
isr_sw_int_dummy:
    //ISR_SAFE_STACK_ENTRY c_int_dummy_handler, BIOS_SW_ISR_SS, BIOS_SW_ISR_SP, BIOS_SW_ISR_FS, BIOS_SW_ISR_GS, BIOS_SW_ISR_FRAME
	iret

// ========================================================
// FUNCTIONS FOR TINY8086 BASIC
// ========================================================

.global launch_basic
launch_basic:
    cli

    // copy BASIC from ROM to RAM (0x2000:0x0000)
    call copy_basic_to_ram

	push ax
    push ds
    push es

    // basic uses an own stack at 0x1000:0x0000
    mov     ax, 0x1000           // STACK_SEG ist 1000h
    mov     ss, ax        
    mov     ax, 0xFFFF           // STACK_OFF ist 0FFFFh
    mov     sp, ax
    mov     bp, ax

    // set DataSegment to desired Segment
    mov     ax, BASIC_SEG        // PROG_SEG ist 2000h
    mov     ds, ax
    mov     es, ax

    sti

    // jump to BASIC in RAM at 0x2000:0x0000
    jmp 0x2000:0x0000

    pop es
    pop ds
	pop ax

    ret

copy_basic_to_ram:
	push ax
    push ds
    push es
    
    // get the source from ROM
    mov ax, ROM_SEG              // BIOS-Segment
    mov ds, ax
    mov si, offset basic_binary  // offset of BASIC in ROM
    
    // destination in RAM
    mov ax, BASIC_SEG            // destination-segment
    mov es, ax
    xor di, di                   // Offset 0x0000
    
    // copy BASIC from ROM to RAM
    mov cx, BASIC_SIZE_WORDS     // number of words to copy
    cld                          // clear direction flag (copy forwards)
    rep movsw                    // copy DS:SI to ES:DI
    
    pop es
    pop ds
	pop ax

    ret

.align 16
basic_binary:
    .incbin "bin/basic.bin"

// ========================================================
// END OF TINY8086 BASIC
// ========================================================
