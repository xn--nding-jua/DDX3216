/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#include "isr.h"

// **********************************************************
// Interrupt functions
// **********************************************************

void pirq_init() {
    // map PIRQ0 (external UART) to IRQ4
    outb(CFG_ADDR, 0x60); // Beispiel-Index für PIRQ0 Routing
    outb(CFG_DATA, 0x04); // Map auf IRQ 4
}

// INT 08h: timer-interrupt
__attribute__((interrupt)) void c_int08_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;

	// store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");

	// increment BIOS-Timer-Counter in BDA
    (*BDA_TIMER_COUNTER)++;

	// call INT 1Ch (User Timer Tick)
    __asm__ volatile ("int $0x1C");

    // restore DS after INT1Ch
    __asm__ volatile("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");

    // send end of interrupt to port 0x20
    outb(0x20, 0x20);
	
	// restore DS
	__asm__ volatile("movw %0, %%ds" : : "r"(old_ds));
}

// INT 09h: keyboard-interrupt
__attribute__((interrupt)) void c_int09_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;
    uint8_t scancode;

    // store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");

	scancode = inb(KBD_DATA_PORT);

    // clear shift-register and IRQ in keyboard-controller
    uint8_t ctrl = inb(KBD_CTRL_PORT);
    outb(KBD_CTRL_PORT, ctrl | KBD_CTRL_CLEAR);
    outb(KBD_CTRL_PORT, ctrl & ~KBD_CTRL_CLEAR);

    lcd_putc(scancode, 0x07);

/*
    // Break-Codes (key released) are ignored here
    if (!(scancode & 0x80)) {
        uint16_t tail = *BDA_KBD_TAIL;
        uint16_t next_tail = tail + 2;
        if (next_tail >= BDA_KBD_BUF_END) next_tail = BDA_KBD_BUF_START;

        // if buffer is not full (Tail + 2 != Head)
        if (next_tail != *BDA_KBD_HEAD) {
            // store scancode (high) and ASCII (low)
            uint16_t *ptr = (uint16_t*)(uintptr_t)(tail);
            *ptr = (uint16_t)((scancode << 8) | ascii);
            *BDA_KBD_TAIL = next_tail;
        }
    }
*/

    // send EOI to PIC, to allow more interrupts
    outb(0x20, 0x20);

	// restore DS
	__asm__ volatile("movw %0, %%ds" : : "r"(old_ds));
}

// INT 10h: Video-interrupt
__attribute__((interrupt)) void c_int10_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;
    uint16_t current_ax;
    uint16_t current_dx;

    // read registers first
    __asm__ __volatile__("movw %%ax, %0" : "=r"(current_ax));
    uint8_t ah = (uint8_t)(current_ax >> 8);
    uint8_t al = (uint8_t)(current_ax & 0xFF);
    __asm__ __volatile__("movw %%dx, %0" : "=r"(current_dx));

	// store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");
    
    switch (ah) {
        case 0x0E: // write character
            // write char to display and handle control-characters like newline, carriage return, backspace, etc. internally
            lcd_putc(al, 0x07); // light gray on black
            
            // output ASCII-character to UART (including control-characters like newline, etc.)
            uart_putc(al);
            break;

        case 0x02: // set cursor position
        	writeFarByte(BASE_SEG, BDA_CURSOR_POS_ROW, (current_dx >> 8) & 0xFF); // DH = Row
        	writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, current_dx & 0xFF); // DL = Column

            // update hardware cursor position
            uint16_t offset = ((readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) * (LCD_WIDTH / 8)) + readFarByte(BASE_SEG, BDA_CURSOR_POS_COL)) * 2;
            write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_UPPER, (offset >> 9) & 0xFF); // upper 7 bits of offset (divide by 512)
            write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_LOWER, (offset >> 1) & 0xFF); // lower 8 bits of offset (divide by 2)
            break;

        case 0x03: // get cursor position
            // return cursor position in DX (DH = Row, DL = Column)
            uint16_t return_dx = (readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) << 8) | readFarByte(BASE_SEG, BDA_CURSOR_POS_COL);
            __asm__ __volatile__("movw %0, %%dx" : : "r"(return_dx) : "dx");

            break;

        case 0x0F: // get Current Video Mode
            // AH = Columns (30), AL = Video Mode (00h), BH = Page (0)
            // number of rows is set in BDA and not returned via INT10h, because DOS will ignore the returned value anyway and would always assume 25 rows
            uint16_t return_ax = (30 << 8) | 0x00; // 40x25 char in grayscale-text mode
            uint16_t return_bx = 0x0000; // BH = 0
            __asm__ __volatile__("movw %0, %%ax" : : "r"(return_ax) : "ax");
            __asm__ __volatile__("movw %0, %%bx" : : "r"(return_bx) : "bx");

            break;

        default:
            // simply ignore other video-functions like set cursor, etc.
            break;
    }
	
	// restore DS
	__asm__ volatile("movw %0, %%ds" : : "r"(old_ds));
}

// INT 11h: Get Equipment List
__attribute__((interrupt)) void c_int11_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;

	// store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");

    /*
       Bitmask for Equipment Word:
       Bit 0: Floppy drive? (0 = No)
       Bit 1: Math-Coprozessor (0 = No)
       Bit 4-5: Video-Mode (00 = EGA/VGA/LCD, 10 = Color 80x25)
       Bit 9-11: Number of serial interfaces (001 = One)
       Bit 14-15: Number of printers (00 = No)
    */
	uint16_t equipment = *(uint16_t*)BDA_EQUIPMENT_WORD;
    __asm__ volatile ("movw %0, %%ax" : : "r"(equipment)); // write result into AX-register of caller directly

    // restore DS
	__asm__ volatile("movw %0, %%ds" : : "r"(old_ds));
}

// INT 12h: Get Memory Size
__attribute__((interrupt)) void c_int12_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;

	// store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");

	uint16_t mem_kb = *(uint16_t*)BDA_MEM_SIZE;
    __asm__ volatile ("movw %0, %%ax" : : "r"(mem_kb)); // write result into AX-register of caller directly
	
	// restore DS
	__asm__ volatile("movw %0, %%ds" : : "r"(old_ds));
}

// INT13h: disk-interrupt
__attribute__((interrupt)) void c_int13_handler(struct interrupt_frame *frame) {
    uint16_t old_ds;
    uint8_t  ah, al, ch, cl, dh, dl;
    uint16_t target_bx, target_es;

    __asm__ volatile ("movb %%ah, %0" : "=m"(ah)        : : );
    __asm__ volatile ("movb %%al, %0" : "=m"(al)        : : );
    __asm__ volatile ("movb %%ch, %0" : "=m"(ch)        : : );
    __asm__ volatile ("movb %%cl, %0" : "=m"(cl)        : : );
    __asm__ volatile ("movb %%dh, %0" : "=m"(dh)        : : );
    __asm__ volatile ("movb %%dl, %0" : "=m"(dl)        : : );
    __asm__ volatile ("movw %%bx, %0" : "=m"(target_bx) : : );
    __asm__ volatile (
        "movw %%es, %%ax\n"
        "movw %%ax, %0"
        : "=m"(target_es) : : "ax"
    );

    // store DS and switch to RAM (0x0000)
    __asm__ volatile ("movw %%ds, %0"                   : "=r"(old_ds));
    __asm__ volatile ("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");

    // support only for drive 0x80
    if (dl != 0x80 && ah != 0x00) {
        __asm__ volatile (
            "movb $0x01, %%ah\n"
            "orw  $0x0001, 6(%%bp)"
            ::: "ax", "memory"
        );
        __asm__ volatile ("movw %0, %%ds" : : "r"(old_ds));
        return;
    }

    if (ah == 0x00) {
        // ------------------------------------------------------
        // AH=00h: Reset Disk System
        // ------------------------------------------------------
        outb(IDE_DEV_CTRL, 0x06);
        for (volatile uint16_t i = 0; i < 10000; i++);
        outb(IDE_DEV_CTRL, 0x02);
        ide_wait_ready();

        __asm__ volatile (
            "movb $0x00, %%ah\n"
            "andw $0xFFFE, 6(%%bp)"
            ::: "ax", "memory"
        );
    }
    else if (ah == 0x02) {
        // ------------------------------------------------------
        // AH=02h: Read Sectors
        // ------------------------------------------------------
        uint8_t  sectors_to_read = al;
        uint8_t  sector          = cl & 0x3F;
        uint16_t cylinder        = ((uint16_t)(cl & 0xC0) << 2) | ch;
        uint8_t  head            = dh;
        uint32_t lba             = CHS_TO_LBA(cylinder, head, sector);
        uint8_t  error           = 0;

        for (uint8_t s = 0; s < sectors_to_read; s++) {
            uint32_t cur_lba = lba + s;

            if (!ide_wait_ready()) {
                error = 0xAA;
                break;
            }

            // send LBA command to disk
            outb(IDE_SECT_COUNT, 1);
            outb(IDE_LBA_LOW,   (uint8_t)( cur_lba        & 0xFF));
            outb(IDE_LBA_MID,   (uint8_t)((cur_lba >>  8) & 0xFF));
            outb(IDE_LBA_HIGH,  (uint8_t)((cur_lba >> 16) & 0xFF));
            outb(IDE_DRIVE_HEAD, 0xE0 | (uint8_t)((cur_lba >> 24) & 0x0F));
            outb(IDE_COMMAND, IDE_CMD_READ);

            if (!ide_wait_drq()) {
                error = 0xBB;
                break;
            }

            // read 256 words -> write to ES:BX
            for (uint16_t i = 0; i < 256; i++) {
                uint16_t word = inw(IDE_DATA);
                __asm__ volatile (
                    "movw %0, %%es:(%%bx)"
                    : : "r"(word), "b"(target_bx)
                );
                target_bx += 2;
            }
        }

        if (error == 0) {
            __asm__ volatile (
                "movb $0x00, %%ah\n"
                "andw $0xFFFE, 6(%%bp)"
                ::: "ax", "memory"
            );
        } else {
            __asm__ volatile (
                "movb %0, %%ah\n"
                "orw  $0x0001, 6(%%bp)"
                : : "r"(error) : "ax", "memory"
            );
        }
    }
    else if (ah == 0x08) {
        // ------------------------------------------------------
        // AH=08h: Get Drive Parameters
        // ------------------------------------------------------
        uint8_t max_heads   = CF_HEADS - 1;     // TODO: max_heads = bootsector->bpb.num_heads; // <- DS will be changed so we have to store this somewhere else for later use in INT13h AH=02h
        uint8_t max_sectors = CF_SECTORS;       // TODO: max_sectors = bootsector->bpb.sectors_per_track; // <- DS will be changed so we have to store this somewhere else for later use in INT13h AH=02h
        uint16_t max_cyls   = CF_CYLINDERS - 1; // TODO: max_cyls = bootsector->bpb.total_sectors / (bootsector->bpb.num_heads * bootsector->bpb.sectors_per_track) - 1;

        uint8_t cl_val = (max_sectors & 0x3F) |
                         (uint8_t)((max_cyls >> 2) & 0xC0);
        uint8_t ch_val = (uint8_t)(max_cyls & 0xFF);

        __asm__ volatile (
            "movb $0x00, %%ah\n"
            "movb $0x01, %%bl\n"
            "movb %0,    %%ch\n"
            "movb %1,    %%cl\n"
            "movb %2,    %%dh\n"
            "movb $0x01, %%dl\n"
            "andw $0xFFFE, 6(%%bp)"
            : : "m"(ch_val), "m"(cl_val), "m"(max_heads)
            : "ax", "bx", "cx", "dx", "memory"
        );
    }
    else if (ah == 0x15) {
        // ------------------------------------------------------
        // AH=15h: Get Disk Type
        // ------------------------------------------------------
        __asm__ volatile (
            "movb $0x03, %%ah\n"
            "andw $0xFFFE, 6(%%bp)"
            ::: "ax", "memory"
        );
    }
    else {
        // ------------------------------------------------------
        // Unbekannte Funktion
        // ------------------------------------------------------
        __asm__ volatile (
            "movb $0x01, %%ah\n"
            "orw  $0x0001, 6(%%bp)"
            ::: "ax", "memory"
        );
    }

    // ④ DS wiederherstellen
    __asm__ volatile ("movw %0, %%ds" : : "r"(old_ds));
}

// uart-interrupt
__attribute__((interrupt)) void c_int14_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;

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
	__asm__ volatile("movw %%ds, %0" : "=r"(old_ds));
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
	__asm__ volatile("movw %0, %%ds" : : "r"(old_ds));
}

__attribute__((interrupt)) void c_int15_handler(struct interrupt_frame *frame) {
    uint16_t current_sp;
    __asm__ __volatile__("movw %%sp, %0" : "=r"(current_sp));

    uint16_t current_ax;
    __asm__ __volatile__("movw %%ax, %0" : "=r"(current_ax));
    uint8_t ah = (uint8_t)(current_ax >> 8);

    switch (ah) {
        case 0x88: {
            // request enhanced memory (return 1024 KB in AX)
            uint16_t return_value = 1024;

            __asm__ __volatile__(
                "movw %0, %%ax" 
                : 
                : "r"(return_value) 
                : "ax"
            );
            
            frame->flags &= ~0x0001; // send success by deleting CarryFlag
            return;
        }

        case 0x87:
            // function 0x87: block-move (send success)
            __asm__ __volatile__("xorw %%ax, %%ax" : : : "ax");
            frame->flags &= ~0x0001; // send success by deleting CarryFlag
            return;

        default:
            // unknown function -> send error (AH = 0x86, Carry Flag = 1)
            __asm__ __volatile__("movw $0x8600, %%ax" : : : "ax");
            frame->flags |= 0x0001; // Set Carry Flag
            return;
    }
}

// this interrupt is called by DOS to request next char from ringbuffer
__attribute__((interrupt)) void c_int16_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;
    uint8_t ah;

    __asm__ volatile ("movb %%ah, %0" : "=r"(ah));

    // store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("xorw %%ax, %%ax\n movw %%ax, %%ds" ::: "ax");
	
    if (ah == 0x00) { // read character (blocking)
        while (*BDA_KBD_HEAD == *BDA_KBD_TAIL) {
            __asm__ volatile ("sti; hlt; cli"); // halt CPU until next interrupt
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
	__asm__ volatile("movw %0, %%ds" : : "r"(old_ds));
}

// INT04 (PIRQ0 is mapped here)
__attribute__((interrupt)) void c_int0c_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;
	
	// store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(old_ds));
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


    // send EOI to PIC
    outb(0x20, 0x20);

	// restore DS
	__asm__ volatile("movw %0, %%ds" : : "r"(old_ds));
}

// INT 1Ah for RTC access
__attribute__((interrupt)) void c_int1a_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;
	uint8_t ah;

    __asm__ volatile ("movb %%ah, %0" : "=r"(ah));

    // store DS and reset it to 0
	__asm__ volatile("movw %%ds, %0" : "=r"(old_ds));
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
    __asm__ volatile("movw %0, %%ds" : : "r"(old_ds));
}

// software-interrupt
__attribute__((interrupt)) void c_int1c_handler(struct interrupt_frame *frame) {
	// placeholder for user-program
}

__attribute__((interrupt)) void c_int_dummy_handler(struct interrupt_frame *frame) {
	// placeholder for user-program
}
