/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#include "isr.h"

// **********************************************************
// Interrupt functions
// **********************************************************

// scancode-table
const uint8_t scancode_to_ascii[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, // 0x00-0x0E
    9, 'q', 'w', 'e', 'r', 't', 'z', 'u', 'i', 'o', 'p', '[', ']', 13,    // 0x0F-0x1C
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,   // 0x1D-0x2B
    '\\', 'y', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0            // 0x2C-0x36
};

void pirq_init() {
    // map PIRQ0 (external UART) to IRQ4
    outb(CFG_ADDR, 0x60); // Beispiel-Index für PIRQ0 Routing
    outb(CFG_DATA, 0x04); // Map auf IRQ 4
}

// INT 08h: timer-interrupt
__attribute__((interrupt)) void c_int08_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;

	// store DS and reset it to 0
	__asm__ volatile("mov %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("mov $0, %%ax; mov %%ax, %%ds" ::: "ax");

	// increment BIOS-Timer-Counter in BDA
    (*BDA_TIMER_COUNTER)++;

	// call INT 1Ch (User Timer Tick)
    __asm__ volatile ("int $0x1C");

    // send end of interrupt to port 0x20
    outb(0x20, 0x20);
	
	// restore DS
	__asm__ volatile("mov %0, %%ds" : : "r"(old_ds));
}

// INT 09h: keyboard-interrupt
__attribute__((interrupt)) void c_int09_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;
    uint8_t scancode;
    
	// store DS and reset it to 0
	__asm__ volatile("mov %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("mov $0, %%ax; mov %%ax, %%ds" ::: "ax");

	scancode = inb(KBD_DATA_PORT);

    // Break-Codes (key released) are ignored here
    if (!(scancode & 0x80)) {
        uint8_t ascii = (scancode < 0x37) ? scancode_to_ascii[scancode] : 0;
        
        uint16_t tail = *BDA_KBD_TAIL;
        uint16_t next_tail = tail + 2;
        if (next_tail >= BDA_KBD_BUF_END) next_tail = BDA_KBD_BUF_START;

        // if buffer is not full (Tail + 2 != Head)
        if (next_tail != *BDA_KBD_HEAD) {
            // store scancode (high) and ASCII (low)
            uint16_t *ptr = (uint16_t*)(uintptr_t)(0x0400 + tail);
            *ptr = (uint16_t)((scancode << 8) | ascii);
            *BDA_KBD_TAIL = next_tail;
        }
    }

    // send EOI to PIC, to allow more interrupts
    outb(0x20, 0x20);
	
	// restore DS
	__asm__ volatile("mov %0, %%ds" : : "r"(old_ds));
}

// INT 10h: Video-interrupt
__attribute__((interrupt)) void c_int10_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;
    uint8_t ah, al;

	// store DS and reset it to 0
	__asm__ volatile("mov %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("mov $0, %%ax; mov %%ax, %%ds" ::: "ax");
    
    // AH = Funktion, AL = Character (on function 0Eh)
    __asm__ volatile ("movb %%ah, %0" : "=r"(ah));
    __asm__ volatile ("movb %%al, %0" : "=r"(al));

    switch (ah) {
        case 0x0E: // Video - Write Character in TTY Mode
            // simply redirect character to TTY
            if (al == '\n') uart_putc('\r'); // DOS often uses only \n, but UART needs \r\n
            uart_putc(al);
            break;

        case 0x0F: // get Current Video Mode
			// fake a 80x25 textmode to DOS
            // AH = Columns (80), AL = Video Mode (03h), BH = Page (0)
            __asm__ volatile (
                "movb $80, %%ah\n"
                "movb $0x03, %%al\n"
                "movb $0x00, %%bh"
                ::: "ax", "bx"
            );
            break;

        default:
            // simply ignore other video-functions like set cursor, etc.
            break;
    }
	
	// restore DS
	__asm__ volatile("mov %0, %%ds" : : "r"(old_ds));
}

// INT 11h: Get Equipment List
__attribute__((interrupt)) void c_int11_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;

	// store DS and reset it to 0
	__asm__ volatile("mov %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("mov $0, %%ax; mov %%ax, %%ds" ::: "ax");

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
}

// INT 12h: Get Memory Size
__attribute__((interrupt)) void c_int12_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;

	// store DS and reset it to 0
	__asm__ volatile("mov %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("mov $0, %%ax; mov %%ax, %%ds" ::: "ax");

	uint16_t mem_kb = *(uint16_t*)BDA_MEM_SIZE;
    __asm__ volatile ("movw %0, %%ax" : : "r"(mem_kb)); // write result into AX-register of caller directly
	
	// restore DS
	__asm__ volatile("mov %0, %%ds" : : "r"(old_ds));
}

// disk-interrupt
__attribute__((interrupt)) void c_int13_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;
    uint8_t ah, al, ch, cl, dh, dl;
    uint16_t es, bx;

	// store DS and reset it to 0
	__asm__ volatile("mov %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("mov $0, %%ax; mov %%ax, %%ds" ::: "ax");

    // get register from context
    __asm__ volatile ("movb %%ah, %0" : "=r"(ah));
    __asm__ volatile ("movb %%al, %0" : "=r"(al));
    __asm__ volatile ("movb %%cl, %0" : "=r"(cl)); // sector
    __asm__ volatile ("movb %%ch, %0" : "=r"(ch)); // cylinder low
    
    if (ah == 0x00) { // reset disk system
        __asm__ volatile ("movb $0x00, %%ah" ::: "ah");
        __asm__ volatile ("andw $0xFFFE, 6(%ebp)"); // reset carry-flag (success)
    } 
    else if (ah == 0x02) { // read sectors
		uint8_t sectors_to_read;
        uint16_t target_bx, target_es;
        
        // get parameters from registers of caller
        __asm__ volatile ("movb %%al, %0" : "=r"(sectors_to_read));
        __asm__ volatile ("movw %%bx, %0" : "=r"(target_bx));
        __asm__ volatile ("movw %%es, %0" : "=r"(target_es));
        
        // CH, CL, DH contains the CHS-addresses (used as LBA here for simplification)
        uint8_t sector, cylinder, head;
        __asm__ volatile ("movb %%cl, %0" : "=r"(sector)); 
        __asm__ volatile ("movb %%ch, %0" : "=r"(cylinder));
        __asm__ volatile ("movb %%dh, %0" : "=r"(head));

        for (uint8_t s = 0; s < sectors_to_read; s++) {
            ide_wait_ready();
            
            // send IDE command (LBA-addressing)
            outb(0x1F2, 1);                         // 1 Sector
            outb(0x1F3, sector + s);                // LBA Low
            outb(0x1F4, cylinder);                  // LBA Mid
            outb(0x1F5, 0);                         // LBA High
            outb(0x1F6, 0xE0 | (head & 0x0F));      // Drive/Head + LBA Mode
            outb(0x1F7, 0x20);                      // Command: Read with retry

			// ATA-specification requires a 400ns delay before reading the state-port
			inb(0x1F7); // dummy read
			inb(0x1F7); // dummy read
			inb(0x1F7); // dummy read
			inb(0x1F7); // dummy read
            ide_wait_ready();

            // data transmission to ES:BX
            // simulate a far-pointer here
            for (int i = 0; i < 256; i++) {
                uint16_t word = inw(0x1F0);
                
                // as GCC works DS-relative in 16-bit mode we use inline-assembly
                // for segmented write-access
				__asm__ volatile (
					"movw %0, %%es:(%%bx)" 
					: 
					: "r"(word), "b"(target_bx) 
				);
				target_bx += 2;
            }
        }

        // send success (AH=0, Carry-Flag resetted)
        __asm__ volatile ("movb $0x00, %%ah" ::: "ah");
        __asm__ volatile ("andw $0xFFFE, 6(%%ebp)" ::: "memory");
    }
	
	// restore DS
	__asm__ volatile("mov %0, %%ds" : : "r"(old_ds));
}

// uart-interrupt
__attribute__((interrupt)) void c_int14_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;

	// store DS and reset it to 0
	__asm__ volatile("mov %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("mov $0, %%ax; mov %%ax, %%ds" ::: "ax");

    // AH contains the function-number (0=Init, 1=Transmit, 2=Receive, 3=State)
    // use inline-assembler as GCC-interrupts are more complex
	uint8_t service;
    uint8_t al_val;
    __asm__ volatile ("movb %%ah, %0" : "=r"(service));
    __asm__ volatile ("movb %%al, %0" : "=r"(al_val));

	switch (service) {
        case 0x00: // initializing (not implemented)
			// we are initializing the UART with fixed 38400 Baud for now. But later we
			// could use this to switch between different speeds and settings
            //uart_init(al_val); // al_val contains the initializing-parameter
            __asm__ volatile ("movb $0x00, %ah"); // state OK
            break;

        case 0x01: // transmit char
            uart_putc((char)al_val);
            // AH Bit 7 = 0 (success), other bits = LSR Status
            __asm__ volatile ("movb $0x60, %ah"); 
            break;

        case 0x02: // receive char (not implemented)
            __asm__ volatile ("movb $0x00, %al");
            __asm__ volatile ("movb $0x80, %ah"); // Timeout/Error
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
	__asm__ volatile("mov %0, %%ds" : : "r"(old_ds));
}

// this interrupt is called by DOS to request next char from ringbuffer
__attribute__((interrupt)) void c_int16_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;
    uint8_t ah;

	// store DS and reset it to 0
	__asm__ volatile("mov %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("mov $0, %%ax; mov %%ax, %%ds" ::: "ax");
	
    __asm__ volatile ("movb %%ah, %0" : "=r"(ah));

    if (ah == 0x00) { // read character (blocking)
        while (*BDA_KBD_HEAD == *BDA_KBD_TAIL) {
            __asm__ volatile ("hlt"); // halt CPU until next interrupt
        }
        
        uint16_t head = *BDA_KBD_HEAD;
        uint16_t val = *(uint16_t*)(uintptr_t)(0x0400 + head);
        
        uint16_t next_head = head + 2;
        if (next_head >= BDA_KBD_BUF_END) next_head = BDA_KBD_BUF_START;
        *BDA_KBD_HEAD = next_head;

        // return in AX: AH = Scancode, AL = ASCII
        __asm__ volatile ("movw %0, %%ax" : : "r"(val) : "ax");
    }
    else if (ah == 0x01) { // check state
        if (*BDA_KBD_HEAD != *BDA_KBD_TAIL) {
            // Key in buffer -> delete ZF
            __asm__ volatile ("andw $0xFFBF, 6(%ebp)");
            uint16_t val = *(uint16_t*)(uintptr_t)(0x0400 + *BDA_KBD_HEAD);
            __asm__ volatile ("movw %0, %%ax" : : "r"(val) : "ax");
        } else {
            // buffer empty -> set ZF
            __asm__ volatile ("orw $0x0040, 6(%ebp)");
        }
    }
	
	// restore DS
	__asm__ volatile("mov %0, %%ds" : : "r"(old_ds));
}

// INT04 (PIRQ0 is mapped here)
__attribute__((interrupt)) void c_int0c_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;
	
	// store DS and reset it to 0
	__asm__ volatile("mov %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("mov $0, %%ax; mov %%ax, %%ds" ::: "ax");


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
                    uint16_t *ptr = (uint16_t*)(uintptr_t)(0x0400 + tail);
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
	__asm__ volatile("mov %0, %%ds" : : "r"(old_ds));
}

// INT 1Ah for RTC access
__attribute__((interrupt)) void c_int1a_handler(struct interrupt_frame *frame) {
	uint16_t old_ds;
	uint8_t ah;

	// store DS and reset it to 0
	__asm__ volatile("mov %%ds, %0" : "=r"(old_ds));
	__asm__ volatile("mov $0, %%ax; mov %%ax, %%ds" ::: "ax");

    __asm__ volatile ("movb %%ah, %0" : "=r"(ah));
	
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
    __asm__ volatile("mov %0, %%ds" : : "r"(old_ds));
}

// software-interrupt
__attribute__((interrupt)) void c_int1c_handler(struct interrupt_frame *frame) {
	// placeholder for user-program
}

__attribute__((interrupt)) void c_int_dummy_handler(struct interrupt_frame *frame) {
	// placeholder for user-program
}
