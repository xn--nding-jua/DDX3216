/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#include "isr.h"

uint16_t g_old_ds;

// **********************************************************
// Interrupt functions
// **********************************************************

void pirq_init() {
    // map PIRQ0 (external UART) to IRQ4
    outb(CFG_ADDR, 0x60); // Beispiel-Index für PIRQ0 Routing
    outb(CFG_DATA, 0x04); // Map auf IRQ 4
}

__attribute__((externally_visible, regparm(1))) void c_int08_handler(struct interrupt_registers *regs) {
    uart_putc('T');
    //lcd_putc('T', 0x07);

    // increment BIOS-Timer-Counter in BDA
    (*BDA_TIMER_COUNTER)++;

    // call INT 1Ch (User Timer Tick)
    //__asm__ volatile ("int $0x1C");
}

// INT 09h: keyboard-interrupt
volatile uint8_t g_kbd_scancode;
static volatile uint8_t g_kbd_set1_code;
static volatile bool g_kbd_is_break;
static volatile char g_kbd_ascii;
static volatile uint16_t g_kbd_tail;
static volatile uint16_t g_kbd_next_tail;
static volatile uint16_t* g_kbd_ptr;

__attribute__((externally_visible, regparm(1))) void c_int09_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('0');
    uart_putc('9');

    // read scancode from keyboard-controller
	g_kbd_scancode = inb(KBD_DATA_PORT);

    // clear shift-register and IRQ in keyboard-controller
    outb(KBD_CTRL_PORT, inb(KBD_CTRL_PORT) | KBD_CTRL_CLEAR);
    outb(KBD_CTRL_PORT, inb(KBD_CTRL_PORT) & ~KBD_CTRL_CLEAR);

    switch (kbd_state) {
        case KBD_STATE_IDLE:
            if (g_kbd_scancode == SC2_EXTENDED) {
                kbd_state = KBD_STATE_EXTENDED;
                goto eoi_int09;
            }
            if (g_kbd_scancode == SC2_BREAK) {
                kbd_state = KBD_STATE_BREAK;
                goto eoi_int09;
            }
            // regular make-code (key pressed)
            //g_kbd_set1_code = set2_to_set1[g_kbd_scancode];
            g_kbd_set1_code = readRomByte((uint16_t)(uintptr_t)&set2_to_set1[g_kbd_scancode]);
            g_kbd_is_break  = false;
            break;

        case KBD_STATE_EXTENDED:
            if (g_kbd_scancode == SC2_BREAK) {
                kbd_state = KBD_STATE_EXT_BREAK;
                goto eoi_int09;
            }
            // extended make-code (key pressed)
            //g_kbd_set1_code = set2_ext_to_set1[g_kbd_scancode];
            g_kbd_set1_code = readRomByte((uint16_t)(uintptr_t)&set2_ext_to_set1[g_kbd_scancode]);
            g_kbd_is_break  = false;
            kbd_state = KBD_STATE_IDLE;
            break;

        case KBD_STATE_BREAK:
            // regular break-code (key released)
            //g_kbd_set1_code = set2_to_set1[g_kbd_scancode];
            g_kbd_set1_code = readRomByte((uint16_t)(uintptr_t)&set2_to_set1[g_kbd_scancode]);
            g_kbd_is_break  = true;
            kbd_state = KBD_STATE_IDLE;
            break;

        case KBD_STATE_EXT_BREAK:
            // Extended Break-Code (key released)
            //g_kbd_set1_code = set2_ext_to_set1[g_kbd_scancode];
            g_kbd_set1_code = readRomByte((uint16_t)(uintptr_t)&set2_ext_to_set1[g_kbd_scancode]);
            g_kbd_is_break  = true;
            kbd_state = KBD_STATE_IDLE;
            break;
    }

    // manage modifier-key
    if (g_kbd_set1_code) {
        switch (g_kbd_set1_code) {
            case 0x2A:  // Left Shift
            case 0x36:  // Right Shift
                shift_pressed = !g_kbd_is_break;
                goto eoi_int09;

            case 0x1D:  // Ctrl
                ctrl_pressed = !g_kbd_is_break;
                goto eoi_int09;

            case 0x38:  // Alt
                alt_pressed = !g_kbd_is_break;
                goto eoi_int09;

            case 0x3A:  // Caps Lock (only on Make)
                if (!g_kbd_is_break) caps_lock = !caps_lock;
                goto eoi_int09;
        }

        uint8_t flags = 0;
        if (shift_pressed) flags |= KBD_FLAG_RSHIFT;
        if (ctrl_pressed)  flags |= KBD_FLAG_CTRL;
        if (alt_pressed)   flags |= KBD_FLAG_ALT;
        if (caps_lock)     flags |= KBD_FLAG_CAPSLOCK;
        *(volatile uint8_t*)BDA_KBD_STATUS_FLAGS = flags;

        // only convert make-codes to ASCII and put them on the LCD, break-codes are ignored for ASCII output
        if (!g_kbd_is_break) {
            g_kbd_ascii = set1_to_ascii(g_kbd_set1_code, shift_pressed, caps_lock);
            if (g_kbd_ascii) {
                // store scancode and ASCII-character in keyboard-buffer in BDA, if there is space (2 bytes per entry: high byte = scancode, low byte = ASCII)
                g_kbd_tail = *BDA_KBD_TAIL;
                g_kbd_next_tail = g_kbd_tail + 2;
                if (g_kbd_next_tail >= BDA_KBD_BUF_END) g_kbd_next_tail = BDA_KBD_BUF_START;

                if (g_kbd_next_tail != *BDA_KBD_HEAD) {
                    // store scancode (high) and ASCII (low)
                    g_kbd_ptr = (uint16_t*)(uintptr_t)(g_kbd_tail);
                    *g_kbd_ptr = (uint16_t)((g_kbd_scancode << 8) | g_kbd_ascii);
                    *BDA_KBD_TAIL = g_kbd_next_tail;
                }
            }
        }
    }

    eoi_int09:
}

// INT04 (PIRQ0 is mapped here)
__attribute__((externally_visible, regparm(1))) void c_int0c_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('0');
    uart_putc('C');

/*
	// store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(g_old_ds));
	__asm__ volatile("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");

	while (!(inb(UART_IIR) & IIR_PENDING)) {
        uint8_t reason = inb(UART_IIR) & IIR_REASON;

        switch (reason) {
            case IIR_RDA:
            case IIR_TIMEOUT:
                // receive data
                // simulate a keyboard-pressing via UART
                uint8_t c = inb(UART_BASE); // reading the data clears the RDA interrupt
                
                // put the received character into the keyboard-buffer
                uint16_t tail = *BDA_KBD_TAIL;
                uint16_t next_tail = tail + 2;
                if (next_tail >= BDA_KBD_BUF_END) next_tail = BDA_KBD_BUF_START;
                
                if (next_tail != *BDA_KBD_HEAD) {
                    uint16_t *ptr = (uint16_t*)(uintptr_t)(tail);
                    *ptr = (uint16_t)c; // Scancode 0, ASCII = c
                    *BDA_KBD_TAIL = next_tail;
                }
                break;

            case IIR_RLS:
                inb(UART_BASE + 5); // LSR lesen löscht diesen Interrupt
                break;

            case IIR_THRE:
                // Sende-Puffer leer. Wenn wir keine Queue haben, 
                // wird dieser Int durch das nächste Schreiben oder IIR-Lesen beruhigt.
                break;
                
            case IIR_MS:
                inb(UART_BASE + 6); // MSR lesen löscht diesen Interrupt
                break;
        }
    }

	// restore DS
	__asm__ volatile("movw %0, %%ds" : : "r"(g_old_ds));
*/
}

// INT 10h: Video-interrupt
__attribute__((externally_visible, regparm(1))) void c_int10_handler(struct interrupt_registers *regs) {
    // read registers first
    uint8_t ah = (uint8_t)(regs->ax >> 8);
    uint8_t al = (uint8_t)(regs->ax & 0xFF);

    switch (ah) {
        case 0x0E: // write character
            // write char to display and handle control-characters like newline, carriage return, backspace, etc. internally
            lcd_putc(al, 0x07); // light gray on black
            
            // output ASCII-character to UART (including control-characters like newline, etc.)
            uart_putc(al);
            break;

        case 0x02: // set cursor position
        	writeFarByte(BASE_SEG, BDA_CURSOR_POS_ROW, (regs->dx >> 8) & 0xFF); // DH = Row
        	writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, regs->dx & 0xFF); // DL = Column

            // update hardware cursor position
            uint16_t offset = ((readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) * (LCD_WIDTH / 8)) + readFarByte(BASE_SEG, BDA_CURSOR_POS_COL)) * 2;
            write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_UPPER, (offset >> 9) & 0xFF); // upper 7 bits of offset (divide by 512)
            write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_LOWER, (offset >> 1) & 0xFF); // lower 8 bits of offset (divide by 2)
            break;

        case 0x03: // get cursor position
            // return cursor position in DX (DH = Row, DL = Column)
            regs->dx = (readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) << 8) | readFarByte(BASE_SEG, BDA_CURSOR_POS_COL);
            break;

        case 0x0F: // get Current Video Mode
            // AH = Columns (30), AL = Video Mode (00h), BH = Page (0)
            // number of rows is set in BDA and not returned via INT10h, because DOS will ignore the returned value anyway and would always assume 25 rows
            regs->ax = (30 << 8) | 0x00; // 40x25 char in grayscale-text mode
            regs->bx = 0x0000; // BH = 0
            break;

        default:
            // simply ignore other video-functions like set cursor, etc.
            break;
    }
}

// INT 11h: Get Equipment List
__attribute__((externally_visible, regparm(1))) void c_int11_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('1');

    //   Bitmask for Equipment Word:
    //   Bit 0: Floppy drive? (0 = No)
    //   Bit 1: Math-Coprozessor (0 = No)
    //   Bit 4-5: Video-Mode (00 = EGA/VGA/LCD, 10 = Color 80x25)
    //   Bit 9-11: Number of serial interfaces (001 = One)
    //   Bit 14-15: Number of printers (00 = No)
    
	regs->ax = *(uint16_t*)BDA_EQUIPMENT_WORD;
}

// INT 12h: Get Memory Size
__attribute__((externally_visible, regparm(1))) void c_int12_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('2');

	regs->ax = *(uint16_t*)BDA_MEM_SIZE;
}

// INT13h: disk-interrupt
__attribute__((externally_visible, regparm(1))) void c_int13_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('3');

    // get registers
    uint8_t ah = (uint8_t)(regs->ax >> 8);
    uint8_t al = (uint8_t)(regs->ax & 0xFF);
    uint8_t ch = (uint8_t)(regs->cx >> 8);
    uint8_t cl = (uint8_t)(regs->cx & 0xFF);
    uint8_t dh = (uint8_t)(regs->dx >> 8);
    uint8_t dl = (uint8_t)(regs->dx & 0xFF);

    uart_putc(ah);
    uart_putc(al);
    uart_putc(ch);
    uart_putc(cl);
    uart_putc(dh);
    uart_putc(dl);

    // clear carry-flag in flags-register
    regs->flags &= ~0x0001; 

    switch(ah) {
        case 0x00: // Reset Disk
            outb(IDE_DEV_CTRL, 0x06);
            for (volatile uint16_t i = 0; i < 10000; i++);
            outb(IDE_DEV_CTRL, 0x02);
            ide_wait_ready();
            regs->ax = 0x0000; // Return-Code = Success
            break;
        
        case 0x02: { // Read Sectors
            uint8_t  sectors_to_read = al;
            uint8_t  sector          = cl & 0b00111111;
            uint16_t cylinder        = ((uint16_t)(cl & 0b11000000) << 2) | ch;
            uint8_t  head            = dh;
            uint16_t dest_bx         = regs->bx; // offset within ES
            uint16_t dest_es         = regs->es; // segment of destination buffer
            uint8_t  sectors_done    = 0;
            uint8_t  error           = 0;
            uint32_t lba             = CHS_TO_LBA(cylinder, head, sector);

            uart_putc('B');
            uart_putc('X');
            uart_putc(dest_bx >> 8);
            uart_putc(dest_bx & 0xFF);
            uart_putc('E');
            uart_putc('S');
            uart_putc(dest_es >> 8);
            uart_putc(dest_es & 0xFF);
            uart_putc('S');
            uart_putc('P');
            uint16_t aktueller_sp;
            __asm__ __volatile__("movw %%sp, %0" : "=r"(aktueller_sp));
            uart_putc(aktueller_sp >> 8);
            uart_putc(aktueller_sp & 0xFF);

            // loop for all requested sectors
            for (uint8_t s = 0; s < sectors_to_read; s++) {
                uint32_t cur_lba = lba + s;

                // moving offset within ES (BX + s * 512)
                uint16_t cur_offset  = dest_bx + ((uint16_t)s * 512);

                error = ide_read_sector(cur_lba, dest_es, cur_offset);
                if (error != 0x00) {
                    break;
                }
                sectors_done++;
            }

            // return answer
            if (error == 0) {
                // success: AH = 00h (Success), AL = number of read sectors
                regs->ax = (0x00 << 8) | sectors_done; 
                regs->flags &= ~0x0001; // clear carray-flag on success
            } else {
                // error occurred
                regs->ax = (error << 8) | sectors_done;
                regs->flags |= 0x0001;  // set carry flag on error
            }
            break;
        }

        case 0x08: { // Get Drive Parameters
            // what kind of drive is requested?
            if (dl >= 0x80) {
                // harddisk / CF-Card
                uint8_t  max_heads   = CF_HEADS - 1;
                uint8_t  max_sectors = CF_SECTORS;       
                uint16_t max_cyls    = CF_CYLINDERS - 1; 

                // CX-encoding:
                // Bits 15-8 : Bits 7-0 of cylinders
                // Bits 7-6  : Bits 9-8 of cylinders
                // Bits 5-0  : sectors per track
                uint8_t cl_val = (uint8_t)((max_cyls >> 2) & 0b11000000) | (uint8_t)(max_sectors & 0b00111111);
                uint8_t ch_val = (uint8_t)(max_cyls & 0xFF); // low(!) eight bits of cylinder-number
                
                regs->ax = 0x0000; // Return-Code = Success
                regs->bx = 0x0000; // Drive type (AT/PS2 floppies only)
                regs->cx = ((uint16_t)ch_val << 8) | cl_val; // 15..6 = logical last index of cylinders = number of cylinders - 1 / 5..0 logical last index of sectors per track = number of sectors (starts at 1!)
                regs->dx =  ((uint16_t)max_heads << 8) | 0x01; // DL = number of hard-disk-drives / DH = logical last index of heads = numer of heads - 1

                // set ES and DI to zero (pointer to drive parameter table, but only for floppies)
                regs->es = 0x0000;
                regs->di = 0x0000;

                regs->flags &= ~0x0001;
            }else{
                // floppy
                regs->ax = (0x01 << 8); // AH = 01h (Invalid Command / Drive Not Ready)
                regs->bx = 0x0000;
                regs->cx = 0x0000;
                regs->dx = 0x0000;      // <--- Sagt dem MBR: 0 Diskettenlaufwerke installiert!
                regs->flags |= 0x0001;  // Set Carry Flag = Fehler!
            }
            break;
        }

        case 0x15: // Get Disk Type
            // AH=03h: Fixed Disk with Sector-Count in CX:DX
            uint32_t total = CF_TOTAL_SECTS;
            regs->ax    = (0x03 << 8);
            regs->cx    = (uint16_t)(total >> 16);
            regs->dx    = (uint16_t)(total & 0xFFFF);
            regs->flags &= ~0x0001;
            break;

        case 0x41: // Extensions Present (we do not use this as we emulate CHS)
            //regs->ax = 0x0100;     // AH = 01h (Invalid function)
            //regs->flags |= 0x0001; // Carry Set = not supported

            // TEST: LBA-Support
            // check for magic word 0x55AA
            if (regs->bx != 0x55AA) {
                regs->flags |= 0x0001;  // CF=1: Fehler
                regs->ax = (0x01 << 8);
                break;
            }

            // enable LBA-support
            regs->bx    = 0xAA55;      // send back magic word
            regs->ax    = (0x30 << 8); // AH = Version 3.0
            regs->cx    = 0x0007;      // Bit 0: Extended Disk Access
                                       // Bit 1: Removable Drive Support
                                       // Bit 2: EDD (Enhanced Disk Drive)
            regs->flags &= ~0x0001;    // CF=0: Extensions available
            break;

        case 0x42: // Extended Read Sectors
            // DAP is at DS:SI
            uint16_t dap_segment = regs->ds;
            uint16_t dap_offset  = regs->si;

            // read DAP bytewise from caller
            struct disk_address_packet dap;
            uint8_t* dap_ptr = (uint8_t*)&dap;
            for (uint8_t i = 0; i < sizeof(dap); i++) {
                dap_ptr[i] = readFarByte(dap_segment, dap_offset + i);
            }

            // check the structure
            if (dap.size != 0x10) {
                regs->ax    = (0x01 << 8); // bad data
                regs->flags |= 0x0001;
                break;
            }

            // 64-bit LBA: upper 32 Bit must be 0 (CF-Card < 4GB)
            if (dap.lba_high != 0) {
                regs->ax    = (0x01 << 8); // LBA out of border
                regs->flags |= 0x0001;
                break;
            }

            uint32_t lba          = dap.lba_low;
            uint16_t sector_count = dap.sector_count;
            uint16_t dest_offset  = dap.dest_offset;
            uint16_t dest_segment = dap.dest_segment;
            uint16_t sectors_done = 0;
            uint8_t  error        = 0;

            for (uint16_t s = 0; s < sector_count; s++) {
                uint32_t cur_lba    = lba + s;
                uint16_t cur_offset = dest_offset + (s * 512);

                error = ide_read_sector(cur_lba, dest_segment, cur_offset);
                if (error != 0x00) {
                    break;
                }
                sectors_done++;
            }

            // update sector-count in DAP
            //writeFarByte(dap_segment, dap_offset + 2, (uint8_t)(sectors_done & 0xFF));
            //writeFarByte(dap_segment, dap_offset + 3, (uint8_t)(sectors_done >> 8));

            if (error == 0) {
                regs->ax    = 0x0000; // Return-Code = Success
                regs->flags &= ~0x0001;
            } else {
                regs->ax    = (error << 8);
                regs->flags |= 0x0001;
            }
            break;

        case 0x48: // Get Drive Parameters Extended
            uint16_t buf_segment = regs->ds;
            uint16_t buf_offset  = regs->si;

            // check buffer-size (first two bytes)
            uint16_t buf_size = readFarWord(buf_segment, buf_offset);
            if (buf_size < 0x1A) {
                regs->ax    = (0x01 << 8);
                regs->flags |= 0x0001;
                break;
            }

            struct drive_params_ext params;
            params.size           = 0x1A;
            params.flags          = 0x0002;        // Bit 1: Geometrie gültig
            params.cylinders      = CF_CYLINDERS;
            params.heads          = CF_HEADS;
            params.sectors        = CF_SECTORS;
            params.total_low      = CF_TOTAL_SECTS;
            params.total_high     = 0;
            params.bytes_per_sect = 512;

            // write structure in buffer of caller
            uint8_t *p = (uint8_t*)&params;
            for (uint8_t i = 0; i < sizeof(params); i++) {
                writeFarByte(buf_segment, buf_offset + i, p[i]);
            }

            regs->ax    = 0x0000;
            regs->flags &= ~0x0001;
            break;

        default:
            regs->ax = (0x01 << 8);
            regs->flags |= 0x0001; // Carry Set
            break;
    }
}

// uart-interrupt
__attribute__((externally_visible, regparm(1))) void c_int14_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('4');

/*
    // AH contains the function-number (0=Init, 1=Transmit, 2=Receive, 3=State)
    // use inline-assembler as GCC-interrupts are more complex
	uint8_t service;
    uint8_t al_val;
    __asm__ volatile (
        "movb %%ah, %0\n"
        "movb %%al, %1\n"
        : "=r"(service), "=r"(al_val)
        :
        :
    );

    // store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(g_old_ds));
	__asm__ volatile("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");

	switch (service) {
        case 0x00: // initializing (not implemented)
			// we are initializing the UART with fixed 38400 Baud for now. But later we
			// could use this to switch between different speeds and settings
            //uart_init(al_val); // al_val contains the initializing-parameter
            __asm__ volatile ("movb $0x00, %%ah" ::: "ax"); // state OK
            break;

        case 0x01: // transmit char
            uart_putc((char)al_val);
            // AH Bit 7 = 0 (success), other bits = LSR Status
            __asm__ volatile ("movb $0x60, %%ah" ::: "ax"); 
            break;

        case 0x02: // receive char (not implemented)
            __asm__ volatile ("movb $0x00, %%al" ::: "ax");
            __asm__ volatile ("movb $0x80, %%ah" ::: "ax"); // Timeout/Error
            break;

        case 0x03: // request state
            {
                uint8_t status = inb(UART_LSR); // LSR
                __asm__ volatile ("movb %0, %%ah" : : "r"(status));
                uint8_t modem = inb(UART_MSR);  // MSR
                __asm__ volatile ("movb %0, %%al" : : "r"(modem));
            }
            break;
    }
	
	// restore DS
	__asm__ volatile("movw %0, %%ds" : : "r"(g_old_ds));
*/
}

__attribute__((externally_visible, regparm(1))) void c_int15_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('5');

    regs->flags &= ~0x0001; // clear carray-flag on success
    
/*
    uint16_t current_sp;
    __asm__ __volatile__("movw %%sp, %0" : "=r"(current_sp));

    uint16_t current_ax;
    __asm__ __volatile__("movw %%ax, %0" : "=r"(current_ax));
    uint8_t ah = (uint8_t)(current_ax >> 8);

    switch (ah) {
        case 0x24:  // A20 Gate Control
            {
                uint8_t al = current_ax & 0xFF;
                if (al == 0x01) {
                    a20_enable();
                    regs->flags &= ~0x0001;
                } else if (al == 0x00) {
                    a20_disable();
                    regs->flags &= ~0x0001;
                }
            }
            break;

        case 0x88: {
            // request enhanced memory (return 1024 KB in AX)
            uint16_t return_value = 1024;

            __asm__ __volatile__(
                "movw %0, %%ax" 
                : 
                : "r"(return_value) 
                : "ax"
            );
            
            regs->flags &= ~0x0001; // send success by deleting CarryFlag
            break;
        }

        case 0x87:
            // function 0x87: block-move (send success)
            __asm__ __volatile__("xorw %%ax, %%ax" : : : "ax");
            regs->flags &= ~0x0001; // send success by deleting CarryFlag
            break;

        default:
            // unknown function -> send error (AH = 0x86, Carry Flag = 1)
            __asm__ __volatile__("movw $0x8600, %%ax" : : : "ax");
            regs->flags |= 0x0001; // Set Carry Flag
            break;
    }
*/
}

// this interrupt is called by DOS to request next char from ringbuffer
__attribute__((externally_visible, regparm(1))) void c_int16_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('6');

    regs->ax = 0x0100;
    regs->flags &= ~0x0001; // clear carray-flag on success

/*
    uint8_t ah;

    __asm__ volatile ("movb %%ah, %0" : "=r"(ah));

    // store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(g_old_ds));
	__asm__ volatile("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");
	
    if (ah == 0x00) { // read character (blocking)
        while (*BDA_KBD_HEAD == *BDA_KBD_TAIL) {
            __asm__ volatile ("sti\n hlt\n cli"); // halt CPU until next interrupt
        }
        
        uint16_t head = *BDA_KBD_HEAD;
        uint16_t val = *(uint16_t*)(uintptr_t)(head);
        
        uint16_t next_head = head + 2;
        if (next_head >= BDA_KBD_BUF_END) next_head = BDA_KBD_BUF_START;
        *BDA_KBD_HEAD = next_head;

        // return in AX: AH = Scancode, AL = ASCII
        __asm__ volatile ("movw %0, %%ax" : : "r"(val) : "ax");
    }
    else if (ah == 0x01) { // check state
        if (*BDA_KBD_HEAD != *BDA_KBD_TAIL) {
            // Key in buffer -> delete ZF
            __asm__ volatile ("andw $0xFFBF, 6(%%bp)" ::: "memory");
            uint16_t val = *(uint16_t*)(uintptr_t)(*BDA_KBD_HEAD);
            __asm__ volatile ("movw %0, %%ax" : : "r"(val) : "ax");
        } else {
            // buffer empty -> set ZF
            __asm__ volatile ("orw  $0x0040, 6(%%bp)" ::: "memory");
        }
    }
	
	// restore DS
	__asm__ volatile("movw %0, %%ds" : : "r"(g_old_ds));
*/
}

// this interrupt is called by DOS to request next char from ringbuffer
__attribute__((externally_visible, regparm(1))) void c_int17_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('7');

    regs->ax = 0x3000; // timeout
    regs->flags &= ~0x0001; // clear carray-flag on success
}

__attribute__((externally_visible, regparm(1))) void c_int19_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('9');

/*
	// store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(g_old_ds));
	__asm__ volatile("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");

    // disable interrupts
    __asm__ volatile("cli");
    
    // reset stack
    __asm__ volatile(
        "mov $0x0000, %%ax\n"
        "mov %%ax, %%ss\n"
        "mov $0x7C00, %%sp\n"
        ::: "ax"
    );
    
    // reboot from CF-card
    boot_dos();
    
    while(1) { __asm__("hlt"); }
*/
}

// INT 1Ah for RTC access
__attribute__((externally_visible, regparm(1))) void c_int1a_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('A');

    regs->ax = 0x0000;
    regs->cx = 0x0000;
    regs->dx = 0x0000;
    regs->flags &= ~0x0001; // clear carray-flag on success

/*
	uint8_t ah;

    __asm__ volatile ("movb %%ah, %0" : "=r"(ah));

    // store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(g_old_ds));
	__asm__ volatile("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");
	
    switch (ah) {
        case 0x00: // Get System Time
            {
                uint32_t ticks = *BDA_TIMER_COUNTER;
                uint16_t low = (uint16_t)(ticks & 0xFFFF);
                uint16_t high = (uint16_t)(ticks >> 16);

                __asm__ volatile (
                    "movw %0, %%cx\n"
                    "movw %1, %%dx\n"
                    "xorw %%ax, %%ax" // AL = 0
                    : 
                    : "r"(high), "r"(low) 
                    : "ax", "cx", "dx"
                );
            }
            break;

        case 0x02: // Read Real Time Clock Time
            {
				uint8_t h = read_rtc(0x04);
                uint8_t m = read_rtc(0x02);
                uint8_t s = read_rtc(0x00);
                
                __asm__ volatile (
                    "movb %0, %%ch\n"
                    "movb %1, %%cl\n"
                    "movb %2, %%dh\n"
                    "clc" 
                    : 
                    : "g"(h), "g"(m), "g"(s) // "g" erlaubt Register, Memory oder Immediate
                    : "cx", "dx", "cc"
                );
            }
            break;

        default:
            __asm__ volatile ("stc");
            break;
    }

	// restore DS
    __asm__ volatile("movw %0, %%ds" : : "r"(g_old_ds));
*/
}

// software-interrupt
__attribute__((externally_visible, regparm(1))) void c_int1c_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('C');

	// placeholder for user-program
}

__attribute__((externally_visible, regparm(1))) void c_int_dummy_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('?');
    uart_putc('?');

    uint8_t ah = (uint8_t)(regs->ax >> 8);
    uint8_t al = (uint8_t)(regs->ax & 0xFF);
    uint8_t ch = (uint8_t)(regs->cx >> 8);
    uint8_t cl = (uint8_t)(regs->cx & 0xFF);
    uint8_t dh = (uint8_t)(regs->dx >> 8);
    uint8_t dl = (uint8_t)(regs->dx & 0xFF);

    uart_putc(ah);
    uart_putc(al);
    uart_putc(ch);
    uart_putc(cl);
    uart_putc(dh);
    uart_putc(dl);

    //regs->ax = 0x8600; // "Function not supported"
    //regs->flags |= 0x0001;
    regs->ax = 0x0000;
    regs->flags &= ~0x0001; // clear carray-flag on success
}
