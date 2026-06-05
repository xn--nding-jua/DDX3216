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
    // map PIRQ0 (external UART) to INT04

    // PIRQ Configuration Register (Index B2h)
    // Bits 7-4: PIRQ1, Bits 3-0: PIRQ0
    //                      11110000 = PIRQ0
    write_sc300_cfg(0xB2, 0b00000100); // PIRQ0 -> INT04. See page p5-80
}

// INT04 (PIRQ0 is mapped here)
__attribute__((externally_visible, regparm(1))) void c_int04_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('0');
    uart_putc('4');

	while (!(inb(UART_IIR) & IIR_PENDING)) {
        uint8_t reason = inb(UART_IIR) & IIR_REASON;

        switch (reason) {
            case IIR_RLS:
                // receiver line status (errors, break)
                inb(UART_LSR); // LSR lesen löscht diesen Interrupt
                break;

            case IIR_RDA:
                // receive data available
                uint8_t c = inb(UART_RBR); // this resets the IRQ

                // !!!DEBUG for TESTING!!!
                if (c == 'R') {
                    // on 'R' the CPU should reset
                    cpu_reset();
                }
                break;

            case IIR_TIMEOUT:
                inb(UART_RBR); // this resets the IRQ
                break;

            case IIR_THRE:
                // reading IIR or writing THR will reset this IRQ
                break;

            case IIR_MS:
                inb(UART_MSR); // this resets the IRQ
                break;
        }
    }
}

// INT 09h: keyboard-interrupt
__attribute__((externally_visible, regparm(1))) void c_int09_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('0');
    uart_putc('9');

    // read scancode from keyboard-controller
	uint8_t scancode = inb(KBD_DATA_PORT);

    // clear shift-register and IRQ in keyboard-controller
    uint8_t ctrl = inb(KBD_CTRL_PORT);
    outb(KBD_CTRL_PORT, ctrl | KBD_CTRL_CLEAR);   // set Bit 7
    outb(KBD_CTRL_PORT, ctrl & ~KBD_CTRL_CLEAR);  // clear Bit 7

    // check if it is make or break
    if (scancode & 0x80) {
        // break (key is lifted up)
        uint8_t real_scancode = scancode & 0x7F; // Bit 7 entfernen

        switch(real_scancode) {
            case 0x2A: // Left Shift
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) & ~KBD_FLAG_LSHIFT);
                break;
            case 0x36: // Right Shift
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) & ~KBD_FLAG_RSHIFT);
                break;
            case 0x1D: // Ctrl
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) & ~KBD_FLAG_CTRL);
                break;
            case 0x38: // Alt
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) & ~KBD_FLAG_ALT);
                break;
        }
    }else{
        // make (key is pressed down)
        switch(scancode) {
            case 0x2A: // Left Shift
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) | KBD_FLAG_LSHIFT);
                break;
            case 0x36: // Right Shift
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) | KBD_FLAG_RSHIFT);
                break;
            case 0x1D: // Ctrl
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) | KBD_FLAG_CTRL);
                break;
            case 0x38: // Alt
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) | KBD_FLAG_ALT);
                break;
            default:
                // regular key, put scancode into keyboard-buffer
                char ascii = 0;

                // check if shift is active
                uint8_t status_flags = readFarByte(0x0000, BDA_KBD_STATUS_FLAGS);
                if (status_flags & (KBD_FLAG_LSHIFT | KBD_FLAG_RSHIFT)) {
                    if (scancode < sizeof(xt_to_ascii_shift)) {
                        uint16_t rom_offset = (uint16_t)(uintptr_t)&xt_to_ascii_shift[scancode];
                        ascii = (char)readRomByte(rom_offset);
                    }else{
                        ascii = 0;
                    }
                } else {
                    if (scancode < sizeof(xt_to_ascii_normal)) {
                        uint16_t rom_offset = (uint16_t)(uintptr_t)&xt_to_ascii_normal[scancode];
                        ascii = (char)readRomByte(rom_offset);
                    }else{
                        ascii = 0;
                    }
                }

                // if valid ascii-character, put it into the keyboard-buffer together with the scancode, otherwise ignore it
                if (ascii != 0 || scancode != 0) {
                    uint16_t next_tail = readFarWord(0x0000, BDA_KBD_TAIL) + sizeof(uint16_t);
                
                    // wrap write-pointer around if it reaches the end of the buffer
                    if (next_tail >= BDA_KBD_BUF_END) {
                        next_tail = BDA_KBD_BUF_START;
                    }

                    // check if buffer has some space left for the new scancode
                    if (next_tail != readFarWord(0x0000, BDA_KBD_HEAD)) {
                        // BIOS is storing a 16-bit value for each key-press in the keyboard-buffer,
                        // where the high-byte is the scancode and the low-byte is the ASCII-character

                        // High-Byte = Scancode, Low-Byte = ASCII
                        uint16_t key_data = (scancode << 8) | (uint8_t)ascii;
                        
                        // write keydata to keyboard-buffer at position
                        writeFarWord(0x0000, readFarWord(0x0000, BDA_KBD_TAIL), key_data);

                        // update tail-pointer
                        writeFarWord(0x0000, BDA_KBD_TAIL, next_tail);
                    } else {
                        // buffer overflow -> ignore new key-press
                        // regular BIOS would beep here, but we have no speaker
                    }
                }

                break;
        }
    }

    // send End of Interrupt (EOI) to PIC
    outb(0x20, 0x20);
}

// INT 10h: Video-interrupt
__attribute__((externally_visible, regparm(1))) void c_int10_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('0');

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
            // AH = Columns (40), AL = Video Mode (00h), BH = Page (0)
            // number of rows is set in BDA and not returned via INT10h, because DOS will ignore the returned value anyway and would always assume 25 rows
            regs->ax = (40 << 8) | 0x00; // 40x25 char in grayscale-text mode
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
    
	regs->ax = readFarWord(0x0000, BDA_EQUIPMENT_WORD);
}

// INT 12h: Get Memory Size
__attribute__((externally_visible, regparm(1))) void c_int12_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('2');

	regs->ax = readFarWord(0x0000, BDA_MEM_SIZE);
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

    // clear carry-flag in flags-register
    regs->flags &= ~ISR_FLAGS_CF; 

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
            uint32_t lba             = disk_chs_to_lba(cylinder, head, sector);

            // loop for all requested sectors
            for (uint8_t s = 0; s < sectors_to_read; s++) {
                uint32_t cur_lba = lba + s;

                // moving offset within ES (BX + s * 512)
                uint16_t cur_offset  = dest_bx + ((uint16_t)s * 512);

                uint16_t cur_seg     = dest_es;

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
                regs->flags &= ~ISR_FLAGS_CF; // clear carray-flag on success
            } else {
                // error occurred
                regs->ax = (error << 8) | sectors_done;
                regs->flags |= ISR_FLAGS_CF;  // set carry flag on error
            }
            break;
        }

        case 0x08: { // Get Drive Parameters
            // what kind of drive is requested?
            if (dl >= 0x80) {
                // harddisk / CF-Card
                uint8_t  max_heads   = hd0_params.heads - 1;
                uint8_t  max_sectors = hd0_params.sectors_per_track;
                uint16_t max_cyls    = hd0_params.cylinders - 1;

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

                regs->flags &= ~ISR_FLAGS_CF;
            }else{
                // floppy
                regs->ax = (0x01 << 8); // AH = 01h (Invalid Command / Drive Not Ready)
                regs->bx = 0x0000;
                regs->cx = 0x0000;
                regs->dx = 0x0000;      // <--- Sagt dem MBR: 0 Diskettenlaufwerke installiert!
                regs->flags |= ISR_FLAGS_CF;  // Set Carry Flag = Fehler!
            }
            break;
        }

        case 0x15: // Get Disk Type
            // AH=03h: Fixed Disk with Sector-Count in CX:DX
            uint32_t total = (uint32_t)hd0_params.cylinders * (uint32_t)hd0_params.heads * (uint32_t)hd0_params.sectors_per_track;
            regs->ax    = (0x03 << 8);
            regs->cx    = (uint16_t)(total >> 16);
            regs->dx    = (uint16_t)(total & 0xFFFF);
            regs->flags &= ~ISR_FLAGS_CF;
            break;

        case 0x41: // Extensions Present
            //regs->ax = 0x0100;     // AH = 01h (Invalid function)
            //regs->flags |= ISR_FLAGS_CF; // Carry Set = not supported

            // TEST: LBA-Support
            // check for magic word 0x55AA
            if (regs->bx != 0x55AA) {
                regs->flags |= ISR_FLAGS_CF;  // CF=1: Fehler
                regs->ax = (0x01 << 8);
                break;
            }

            // enable LBA-support
            regs->bx    = 0xAA55;      // send back magic word
            regs->ax    = (0x30 << 8); // AH = Version 3.0
            regs->cx    = 0x0007;      // Bit 0: Extended Disk Access
                                       // Bit 1: Removable Drive Support
                                       // Bit 2: EDD (Enhanced Disk Drive)
            regs->flags &= ~ISR_FLAGS_CF;    // CF=0: Extensions available
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
                regs->flags |= ISR_FLAGS_CF;
                break;
            }

            // 64-bit LBA: upper 32 Bit must be 0 (CF-Card < 4GB)
            if (dap.lba_high != 0) {
                regs->ax    = (0x01 << 8); // LBA out of border
                regs->flags |= ISR_FLAGS_CF;
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
                uint32_t total_offset = (uint32_t)dest_offset + ((uint32_t)s * 512);
                uint16_t cur_seg    = dest_segment + (uint16_t)((total_offset >> 4) & 0xF000);
                uint16_t cur_offset = (uint16_t)(total_offset & 0xFFFF);

                error = ide_read_sector(cur_lba + s, dest_segment, cur_offset);                
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
                regs->flags &= ~ISR_FLAGS_CF;
            } else {
                regs->ax    = (error << 8);
                regs->flags |= ISR_FLAGS_CF;
            }
            break;

        case 0x48: // Get Drive Parameters Extended
            uint16_t buf_segment = regs->ds;
            uint16_t buf_offset  = regs->si;

            // check buffer-size (first two bytes)
            uint16_t buf_size = readFarWord(buf_segment, buf_offset);
            if (buf_size < 0x1A) {
                regs->ax    = (0x01 << 8);
                regs->flags |= ISR_FLAGS_CF;
                break;
            }

            struct drive_params_ext params;
            params.size           = 0x1A;
            params.flags          = 0x0002;        // Bit 1: Geometrie gültig
            params.cylinders      = hd0_params.cylinders;
            params.heads          = hd0_params.heads;
            params.sectors        = hd0_params.sectors_per_track;
            params.total_low      = (uint32_t)hd0_params.cylinders * (uint32_t)hd0_params.heads * (uint32_t)hd0_params.sectors_per_track;
            params.total_high     = 0; // we are not supporting disks larger than 4GB, so upper 32 Bit of total sectors is always zero
            params.bytes_per_sect = 512; // we only support 512 bytes per sector, so this is fixed to 512

            // write structure in buffer of caller
            uint8_t *p = (uint8_t*)&params;
            for (uint8_t i = 0; i < sizeof(params); i++) {
                writeFarByte(buf_segment, buf_offset + i, p[i]);
            }

            regs->ax    = 0x0000;
            regs->flags &= ~ISR_FLAGS_CF;
            break;

        default:
            regs->ax = (0x01 << 8);
            regs->flags |= ISR_FLAGS_CF; // Carry Set
            break;
    }
}

// uart-interrupt
__attribute__((externally_visible, regparm(1))) void c_int14_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('4');

    // get registers
    uint8_t ah = (uint8_t)(regs->ax >> 8);
    uint8_t al = (uint8_t)(regs->ax & 0xFF);

	switch (ah) {
        case 0x00: // initializing (not implemented)
			// we are initializing the UART with fixed 38400 Baud for now. But later we
			// could use this to switch between different speeds and settings
            regs->ax    = 0x0000;
            regs->flags &= ~ISR_FLAGS_CF;
            break;

        case 0x01: // transmit char
            uart_putc((char)al);
            regs->ax    = 0x6000;
            regs->flags &= ~ISR_FLAGS_CF;
            break;

        case 0x02: // receive char (not implemented)
            regs->ax    = 0x8000; // timeout
            regs->flags &= ~ISR_FLAGS_CF;
            break;

        case 0x03: // request state
            {
                uint8_t status = inb(UART_LSR); // LSR
                uint8_t modem = inb(UART_MSR);  // MSR
                regs->ax    = ((uint16_t)status << 8) | modem;
                regs->flags &= ~ISR_FLAGS_CF;
            }
            break;
    }
}

// Detecting Memory
__attribute__((externally_visible, regparm(1))) void c_int15_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('5');

    uint8_t ah = (uint8_t)(regs->ax >> 8);
    uint8_t al = (uint8_t)(regs->ax & 0xFF);

    uart_putc(ah);
    uart_putc(al);

    switch (ah) {
        case 0x24:  // A20 Gate Control
            if (al == 0x00) { // disable Gate A20
                a20_disable();
                regs->flags &= ~ISR_FLAGS_CF; // Success
            }else if (al == 0x01) { // enable Gate A20
                a20_enable();
                regs->flags &= ~ISR_FLAGS_CF; // Success
            } else if (al == 0x02) { // request if Gate A20 is opened
                regs->ax = a20_is_enabled() ? ISR_FLAGS_CF : 0x0000;
                regs->flags &= ~ISR_FLAGS_CF;
            } else {
                regs->ax = 0x8600; // unknown AL-Subcode
                regs->flags |= ISR_FLAGS_CF;
            }
            break;

        case 0x41:
            if (al == 0x01) {
                regs->ax = 0x0000;
                regs->flags &= ~ISR_FLAGS_CF;
            } else {
                regs->ax = 0x8600;
                regs->flags |= ISR_FLAGS_CF;
            }
            break;

        case 0x86: {
            uint32_t usec = ((uint32_t)regs->cx << 16) | regs->dx;

            if (usec > 1000000UL) {
                usec = 1000000UL;
            }

            delay_us(usec);

            regs->ax = 0x0000;
            regs->flags &= ~ISR_FLAGS_CF;
            break;
        }

        case 0x87:
            // Move block to/from extended memory -> unsupported
            regs->ax = 0x8600;
            regs->flags |= ISR_FLAGS_CF;
            break;

        case 0x88: {
            // Request Extended Memory Size (up to 64MB)
            //regs->ax = 1024; // TODO: implement the full 16 MB
            regs->ax = 0; // for Debugging: no HIMEM
            regs->flags &= ~ISR_FLAGS_CF; // Clear Carry Flag (Success)
            break;
        }

        case 0x90: // AH=90h: Device busy
        case 0x91: // AH=91h: Interrupt complete
            regs->ax = 0x0000;
            regs->flags &= ~ISR_FLAGS_CF;
            break;

        case 0xC0: {
            // Get Configuration Table
            // we do not support this for now
            regs->ax = 0x8600;      // AH = 0x86
            regs->flags |= ISR_FLAGS_CF;  // Set Carry Flag
            break;
        }

        case 0xC1: // Get EBDA segment
            regs->es = BIOS_SEG;
            regs->ax = 0x0000;
            regs->flags &= ~ISR_FLAGS_CF;
            break;

        case 0xDA: {
            // E820 / alternative memory-APIs
            // we do not support this for now
            regs->ax = 0x8600;
            regs->flags |= ISR_FLAGS_CF;
            break;
        }

        default:
            // Unbekannte Funktion -> Fehler (AH = 0x86, Carry Flag = 1)
            regs->ax = 0x8600;
            regs->flags |= ISR_FLAGS_CF; // Set Carry Flag
            break;

    }
}

// this interrupt is called by DOS to request next char from ringbuffer
__attribute__((externally_visible, regparm(1))) void c_int16_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('6');

    uint8_t ah = regs->ax >> 8;
/*
    switch (ah) {
        case 0x00: // Read key, blocking
            while (*BDA_KBD_HEAD == *BDA_KBD_TAIL) {
                __asm__ volatile ("sti\n hlt\n cli"); // halt CPU until next interrupt
            }
            
            uint16_t head = *BDA_KBD_HEAD;
            uint16_t val = *(uint16_t*)(uintptr_t)(head);
            
            uint16_t next_head = head + 2;
            if (next_head >= BDA_KBD_BUF_END) next_head = BDA_KBD_BUF_START;
            *BDA_KBD_HEAD = next_head;

            regs->ax = val;
            break;

        case 0x01: // Check key
            if (*BDA_KBD_HEAD != *BDA_KBD_TAIL) {
                // Key in buffer -> delete ZF
                __asm__ volatile ("andw $0xFFBF, 6(%%bp)" ::: "memory");
                uint16_t val = *(uint16_t*)(uintptr_t)(*BDA_KBD_HEAD);
                regs->ax = val;
            } else {
                // buffer empty -> set ZF
                __asm__ volatile ("orw  $0x0040, 6(%%bp)" ::: "memory");
            }
            break;

        default:
            regs->flags |= ISR_FLAGS_ZF;
            break;
    }
*/
}

// this interrupt is called by DOS to request next char from ringbuffer
__attribute__((externally_visible, regparm(1))) void c_int17_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('7');

    regs->ax = 0x3000; // timeout
    regs->flags &= ~ISR_FLAGS_CF; // clear carray-flag on success
}

// boot-process
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
    uint8_t ah = (uint8_t)(regs->ax >> 8);

    switch (ah) {
        case 0x00: { // Get system time
            uint16_t lo = readFarWord(0x0000, BDA_TIMER_COUNTER_LOW);
            uint16_t hi = readFarWord(0x0000, BDA_TIMER_COUNTER_HIGH);
            uint8_t midnight = readFarByte(0x0000, BDA_MIDNIGHT_FLAG);

            regs->cx = hi;
            regs->dx = lo;
            regs->ax = midnight;       // AL = midnight flag, AH = 0
            regs->flags &= ~ISR_FLAGS_CF;    // CF=0

            // Optional: clear IBM-compatible midnight-flag after reading
            writeFarByte(0x0000, BDA_MIDNIGHT_FLAG, 0);
            break;
        }

        case 0x01: { // Set system time CX:DX
            writeFarWord(0x0000, BDA_TIMER_COUNTER_LOW, regs->dx);
            writeFarWord(0x0000, BDA_TIMER_COUNTER_HIGH, regs->cx);
            writeFarByte(0x0000, BDA_MIDNIGHT_FLAG, 0);
            regs->ax = 0x0000;
            regs->flags &= ~ISR_FLAGS_CF;
            break;
        }

        default:
            regs->ax = 0x8600;
            regs->flags |= ISR_FLAGS_CF;
            break;
    }
}

// periodic interrupt
__attribute__((externally_visible, regparm(1))) void c_int1c_handler(struct interrupt_registers *regs) {
    uart_putc('I');
    uart_putc('1');
    uart_putc('C');

	// placeholder for user-program
}

static uint8_t pic_read_isr_master(void) {
    outb(0x20, 0x0B);
    return inb(0x20);
}

static uint8_t pic_read_irr_master(void) {
    outb(0x20, 0x0A);
    return inb(0x20);
}

__attribute__((externally_visible, regparm(1))) void c_int_dummy_handler(struct interrupt_registers *regs) {
    // this interrupt is not supposed to be called, but if it is called, we print some debug information and halt the system

    uart_print("DUMMY INT / EXCEPTION\n");

    uart_print("CS=");
    uart_putc(regs->cs >> 8);
    uart_putc(regs->cs & 0xFF);

    uart_print(" IP=");
    uart_putc(regs->ip >> 8);
    uart_putc(regs->ip & 0xFF);

    uart_print(" AX=");
    uart_putc(regs->ax >> 8);
    uart_putc(regs->ax & 0xFF);

    uint8_t isr = pic_read_isr_master();
    uint8_t irr = pic_read_irr_master();

    uart_print(" PIC ISR=");
    uart_putc(isr);
    uart_print(" IRR=");
    uart_putc(irr);

    uart_print("\nHALT\n");

    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}
