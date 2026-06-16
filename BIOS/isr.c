/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#include "isr.h"

// **********************************************************
// Interrupt functions
// **********************************************************
// list of interrupts see here:
// https://grandidierite.github.io/bios-interrupts
// https://en.wikipedia.org/wiki/BIOS_interrupt_call
// 

void pirq_init() {
    // map PIRQ0 (external UART) to INT04

    // PIRQ Configuration Register (Index B2h)
    // Bits 7-4: PIRQ1, Bits 3-0: PIRQ0
    //                      11110000 = PIRQ0
    write_sc300_cfg(0xB2, 0b00000100); // PIRQ0 -> INT04. See page p5-80
}

/*
// IRQ0 -> INT 08h: timer-interrupt
__attribute__((externally_visible, regparm(1))) void c_int08_handler(struct interrupt_registers *regs) {
    #if BIOS_DEBUG == 1
        uart_print_string("I08\n");
        lcd_print_string("I08\n", 0x07);
    #endif

    // send End of Interrupt (EOI) to PIC
    outb(0x20, 0x20);
}

// INT 1Ch: user-timer-interrupt
__attribute__((externally_visible, regparm(1))) void c_int1c_handler(struct interrupt_registers *regs) {
    #if BIOS_DEBUG == 1
        uart_print_string("I1C\n");
        lcd_print_string("I1C\n", 0x07);
    #endif
}
*/

// IRQ1 -> INT 09h: keyboard-interrupt
__attribute__((externally_visible, regparm(1))) void c_int09_handler(struct interrupt_registers *regs) {
    #if BIOS_DEBUG == 1
        uart_print_string("I09\n");
        lcd_print_string("I09", 0x07);
    #endif

    // read scancode from keyboard-controller
	uint8_t xt_scancode = inb(KBD_DATA_PORT);

    // clear shift-register and IRQ in keyboard-controller
    uint8_t ctrl = inb(KBD_CTRL_PORT);
    outb(KBD_CTRL_PORT, ctrl | KBD_CTRL_CLEAR);   // set Bit 7
    outb(KBD_CTRL_PORT, ctrl & ~KBD_CTRL_CLEAR);  // clear Bit 7

    // check if it is make or break
    if (xt_scancode & 0x80) {
        // break (key is lifted up)

        // check if one of the control-keys has been lifted up
        switch(xt_scancode & 0x7F) {
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
            case 0x46: // Scrolllock
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) & ~KBD_FLAG_SCROLLLOCK);
                break;
            case 0x45: // NumLock
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) & ~KBD_FLAG_NUMLOCK);
                break;
            case 0x3A: // CapsLock
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) & ~KBD_FLAG_CAPSLOCK);
                break;
            case 0x52: // Insert
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) & ~KBD_FLAG_INSERTMODE);
                break;
            default:
                // regular key-releases will not be handled here
                break;
        }
    }else{
        // make (key is pressed down)
        switch(xt_scancode) {
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
            case 0x46: // Scrolllock
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) | KBD_FLAG_SCROLLLOCK);
                break;
            case 0x45: // NumLock
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) | KBD_FLAG_NUMLOCK);
                break;
            case 0x3A: // CapsLock
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) | KBD_FLAG_CAPSLOCK);
                break;
            case 0x52: // Insert
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, readFarByte(0x0000, BDA_KBD_STATUS_FLAGS) | KBD_FLAG_INSERTMODE);
                break;
            default: {
                // regular key, put scancode into keyboard-buffer
                char ascii = 0;

                // check if shift is active
                uint8_t status_flags = readFarByte(0x0000, BDA_KBD_STATUS_FLAGS);
                if (status_flags & (KBD_FLAG_LSHIFT | KBD_FLAG_RSHIFT)) {
                    if (xt_scancode < sizeof(xt_to_ascii_shift)) {
                        uint16_t rom_offset = (uint16_t)(uintptr_t)&xt_to_ascii_shift[xt_scancode];
                        ascii = (char)readRomByte(rom_offset);
                    }else{
                        // not a scancode that can be converted to ASCII
                        ascii = 0;
                    }
                } else {
                    if (xt_scancode < sizeof(xt_to_ascii_normal)) {
                        uint16_t rom_offset = (uint16_t)(uintptr_t)&xt_to_ascii_normal[xt_scancode];
                        ascii = (char)readRomByte(rom_offset);
                    }else{
                        // not a scancode that can be converted to ASCII
                        ascii = 0;
                    }
                }

                // if valid ascii-character, put it into the keyboard-buffer together with the scancode, otherwise ignore it
                if (ascii != 0 || xt_scancode != 0) {
                    uint16_t next_tail = readFarWord(0x0000, BDA_KBD_TAIL) + sizeof(uint16_t);
                
                    // wrap write-pointer around if it reaches the end of the buffer
                    if (next_tail > BDA_KBD_BUF_END) {
                        next_tail = BDA_KBD_BUF_START;
                    }

                    // check if buffer has some space left for the new scancode
                    if (next_tail != readFarWord(0x0000, BDA_KBD_HEAD)) {
                        // BIOS is storing a 16-bit value for each key-press in the keyboard-buffer,
                        // where the high-byte is the scancode and the low-byte is the ASCII-character

                        // High-Byte = Scancode, Low-Byte = ASCII
                        uint16_t key_data = ((uint16_t)xt_scancode << 8) | (uint8_t)ascii;
                        
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
    }
}

// INT04 (PIRQ0 is mapped here)
__attribute__((externally_visible, regparm(1))) void c_int0c_handler(struct interrupt_registers *regs) {
    #if BIOS_DEBUG == 1
        uart_print_string("I0C\n");
        lcd_print_string("I0C", 0x07);
    #endif

	while (!(inb(UART_IIR) & IIR_PENDING)) {
        uint8_t reason = inb(UART_IIR) & IIR_REASON;

        switch (reason) {
            case IIR_RLS:
                // receiver line status (errors, break)
                inb(UART_LSR); // LSR lesen löscht diesen Interrupt
                break;

            case IIR_RDA: {
                // receive data available
                uint8_t c = inb(UART_RBR); // this resets the IRQ

                // !!!DEBUG for TESTING!!!
                if (c == 'R') {
                    // on 'R' the CPU should reset
                    cpu_reset(); // IMPORTANT: REMOVE WHEN EVERYTHING IS WORKING SOMEDAY!!!
                }
                break;
            }

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

// INT 10h: Video-interrupt
__attribute__((externally_visible, regparm(1))) void c_int10_handler(struct interrupt_registers *regs) {
    // read registers first
    uint8_t ah = (uint8_t)(regs->ax >> 8);
    uint8_t al = (uint8_t)(regs->ax & 0xFF);

    switch (ah) {
        case 0x00: // set video mode
            // for videomodes see here: https://mendelson.org/wpdos/videomodes.txt
            switch (al) {
                case 0x03: // standard DOS color-textmode
                    lcd_init(true);
                    lcd_clear();
                    break;
                case 0x06: // 640x200 monochrome pixel graphics mode
                    lcd_init(false);
                    lcd_clear();
                    break;
                default:
                    // unsupported mode
                    break;
            }

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

        case 0x0C: // write graphics pixel
            // AL = Color, BH = Page Number, CX = x, DX = y
            lcd_draw_pixel(regs->cx, regs->dx, al);
            break;

        case 0x0D: // read graphics pixel
            // BH = Page Number, CX = x, DX = y
            // return AL = Color
            regs->ax = (regs->ax & 0xFF00) | lcd_read_pixel(regs->cx, regs->dx);
            break;

        case 0x0E: // write character
            #if BIOS_DEBUG == 1
                uart_putc(al);
            #endif

            // write char to display and handle control-characters like newline, carriage return, backspace, etc. internally
            lcd_putc(al, 0x07); // light gray on black

            break;

        case 0x0F: // get Current Video Mode
            // AL = Video Mode, AH = number of character columns, BH = active page

            // AH = Columns (40), AL = Video Mode (00h), BH = Page (0)
            // number of rows is set in BDA and not returned via INT10h, because DOS will ignore the returned value anyway and would always assume 25 rows
            regs->ax = ((uint16_t)40 << 8) | 0x00; // 40x25 char in grayscale-text mode
            regs->bx = 0x0000; // BH = 0
            break;

        default:
            // simply ignore other video-functions like set cursor, etc.
            break;
    }
}

// INT 11h: Get Equipment List
__attribute__((externally_visible, regparm(1))) void c_int11_handler(struct interrupt_registers *regs) {
    #if BIOS_DEBUG == 1
        uart_print_string("I11\n");
        uart_print_string("BDA_EQUIPMENT_WORD = ");
        uart_print_uint16(readFarWord(0x0000, BDA_EQUIPMENT_WORD), true);
        uart_putc('\n');
    #endif

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
    #if BIOS_DEBUG == 1
        uart_print_string("I12\n");
        uart_print_string("BDA_MEM_SIZE = ");
        uart_print_uint16(readFarWord(0x0000, BDA_MEM_SIZE), true);
        uart_putc('\n');
    #endif

	regs->ax = readFarWord(0x0000, BDA_MEM_SIZE);
}

// INT13h: disk-interrupt
__attribute__((externally_visible, regparm(1))) void c_int13_handler(struct interrupt_registers *regs) {
    #if BIOS_DEBUG == 1
        uart_print_string("I13\n");
        uart_print_string("AX=");
        uart_print_uint16(regs->ax, true);
        uart_print_string("BX=");
        uart_print_uint16(regs->bx, true);
        uart_print_string("CX=");
        uart_print_uint16(regs->cx, true);
        uart_print_string("DX=");
        uart_print_uint16(regs->dx, true);
        uart_print_string("ES=");
        uart_print_uint16(regs->es, true);
        uart_print_string("SP=");
        uint16_t sp;
        __asm__ volatile ("mov %%sp, %0" : "=r" (sp));
        uart_print_uint16(sp, true);
        uart_putc('\n');

        lcd_putc('.', 0x07);
    #else
        ddx3216_setLEDs(); // toggle all LEDs on IDE access
    #endif

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
            regs->flags &= ~ISR_FLAGS_CF;
            break;
        
        case 0x02: { // Read Sectors
            uint8_t  sectors_to_read = al;
            uint8_t  sector          = cl & 0b00111111;
            uint16_t cylinder        = (((uint16_t)(cl & 0b11000000)) << 2) | (uint16_t)ch;
            uint8_t  head            = dh;
            uint16_t dest_bx         = regs->bx; // offset within ES
            uint16_t dest_es         = regs->es; // segment of destination buffer
            uint8_t  sectors_done    = 0;
            uint8_t  error           = 0;
            uint32_t lba             = disk_chs_to_lba(cylinder, head, sector);

            // loop for all requested sectors
            for (uint8_t s = 0; s < sectors_to_read; s++) {
                uint32_t cur_lba     = lba + (uint32_t)s;
                uint16_t cur_offset  = dest_bx + ((uint16_t)s * 512); // moving offset within ES: (BX + s * 512)

                error = ide_read_sector(cur_lba, dest_es, cur_offset);
                if (error != 0x00) {
                    lcd_print_string("IDE ERROR", 0x07);
                    break;
                }

                sectors_done++;
            }

            #if BIOS_DEBUG == 1
                // check last two bytes at SEG 0x9B80, Offset 0x0200 -> should be 0xAA55 = MagicByte
                if (regs->es == 0x9B80) {
                    uart_print_string("Last two bytes of SEG 0x9B80 = ");
                    uart_print_uint16(readFarWord(0x9B80, 0x0200 + 510), true);
                    uart_putc('\n');
                }
                
                // check BPB in bootsector: SEG 0x9B80, Offset 0x0200 + 24...27
                if (regs->es == 0x0070) {
                    uart_print_string("BPB at 0x014E + 24 = 0x");
                    uart_print_uint16(readFarWord(0x0070, 0x014E + 24), true);
                    uart_print_string(" and at 0x014E + 26 = 0x");
                    uart_print_uint16(readFarWord(0x0070, 0x014E + 26), true);
                    uart_putc('\n');
                }
                
                // reading name of formatter
                if (regs->es == 0x0070) {
                    char oem_name[9];
                    for (int i = 0; i < 8; i++) {
                        oem_name[i] = (char)readFarByte(0x0070, 0x014E + 3 + i); 
                    }
                    oem_name[8] = '\0';

                    uart_print_string("OEM Name of Disk-Formatter: ");
                    uart_print_string_ram(oem_name);
                    uart_putc('\n');
                } 
            #endif

            // return answer
            if (error == 0) {
                // success: AH = 00h (Success), AL = number of read sectors
                regs->ax = 0x0000 | sectors_done;
                regs->flags &= ~ISR_FLAGS_CF; // clear carray-flag on success
                #if BIOS_DEBUG == 1
                    uart_print_string("Read ");
                    uart_print_uint16(sectors_done, false);
                    uart_print_string(" sectors!\n");
                #endif
            } else {
                // error occurred
                regs->ax = ((uint16_t)error << 8) | sectors_done;
                regs->flags |= ISR_FLAGS_CF;  // set carry flag on error
                lcd_print_string("IDE ERROR\n", 0x07);
            }
            break;
        }

        case 0x08: { // Get Drive Parameters
            // what kind of drive is requested?
            if (dl == 0x80) {
                // harddisk / CF-Card
                uint16_t max_cyl_idx    = hd0_params.cylinders - 1;     // cylinder-index is 0-based
                uint8_t  max_head_idx   = hd0_params.heads - 1;         // head-index is 0-based
                uint8_t  max_sector     = hd0_params.sectors_per_track; // sectors are 1-based

                // CX-encoding:
                // Bits 15-8 : Bits 7-0 of cylinders
                // Bits 7-6  : Bits 9-8 of cylinders
                // Bits 5-0  : sectors per track
                uint8_t cl_val = (uint8_t)((max_cyl_idx >> 2) & 0b11000000) | (uint8_t)(max_sector & 0b00111111);
                uint8_t ch_val = (uint8_t)(max_cyl_idx & 0xFF); // low(!) eight bits of cylinder-number
                
                regs->ax = 0x0000; // Return-Code = Success
                regs->bx = 0x0000; // Drive type (AT/PS2 floppies only)
                regs->cx = ((uint16_t)ch_val << 8) | cl_val; // 15..6 = logical last index of cylinders = number of cylinders - 1 / 5..0 logical last index of sectors per track = number of sectors (starts at 1!)
                regs->dx = ((uint16_t)max_head_idx << 8) | 0x01; // DL = number of hard-disk-drives / DH = logical last index of heads = numer of heads - 1

                // set ES and DI to zero (pointer to drive parameter table, but only for floppies)
                regs->es = 0x0000;
                regs->di = 0x0000;

                regs->flags &= ~ISR_FLAGS_CF;
            }else if (dl >= 0x81) {
                // we do not support more drives
                regs->ax = 0x0100;  // AH=01 = Invalid Command
                regs->bx = 0x0000;
                regs->cx = 0x0000;
                regs->dx = 0x0000;  // DL=0 = no more drives

                regs->flags |= ISR_FLAGS_CF;              
            }else{
                // floppy
                regs->ax = 0x0100; // AH = 01h (Invalid Command / Drive Not Ready)
                regs->bx = 0x0000;
                regs->cx = 0x0000;
                regs->dx = 0x0000;      // zero floppy-drives available
                regs->flags |= ISR_FLAGS_CF;  // Set Carry Flag = Fehler!
            }
            break;
        }

        case 0x15: { // Get Disk Type
            if (dl == 0x80) {
                // HDD #0
                uint32_t total = (uint32_t)hd0_params.cylinders * (uint32_t)hd0_params.heads * (uint32_t)hd0_params.sectors_per_track;
                regs->ax    = 0x0300 | (regs->ax & 0x00FF);
                regs->cx    = (uint16_t)(total >> 16);
                regs->dx    = (uint16_t)(total & 0xFFFF);
                regs->flags &= ~ISR_FLAGS_CF;
            }else if (dl >= 0x81) {
                // HDD #1 or more
                regs->ax    = 0x0000;    // AH = 00h (Drive not present)
                regs->flags |= ISR_FLAGS_CF;  // Set Carry = Fehler / Nicht vorhanden
            }else{
                // floppy drive
                regs->ax    = 0x0000;    // AH = 00h (Drive not present)
                regs->flags |= ISR_FLAGS_CF;  // Set Carry = Fehler / Nicht vorhanden
            }
            break;
        }

        case 0x41: // Extensions Present
            // LBA-Support

            // check for magic word 0x55AA
            if (regs->bx != 0x55AA) {
                regs->flags |= ISR_FLAGS_CF;  // CF=1: Fehler
                regs->ax = 0x0100;
                break;
            }

            // enable LBA-support
            regs->bx    = 0xAA55;      // send back magic word
            regs->ax    = 0x3000;      // AH = Version 3.0
            regs->cx    = 0x0007;      // Bit 0: Extended Disk Access
                                       // Bit 1: Removable Drive Support
                                       // Bit 2: EDD (Enhanced Disk Drive)
            regs->flags &= ~ISR_FLAGS_CF;    // CF=0: Extensions available
            break;

        case 0x42: { // Extended Read Sectors
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
                regs->ax    = 0x0100; // bad data
                regs->flags |= ISR_FLAGS_CF;
                break;
            }

            // 64-bit LBA: upper 32 Bit must be 0 (CF-Card < 4GB)
            if (dap.lba_high != 0) {
                regs->ax    = 0x0100; // LBA out of border
                regs->flags |= ISR_FLAGS_CF;
                break;
            }

            uint32_t lba          = dap.lba_low;
            uint16_t sector_count = dap.sector_count;
            uint16_t dest_offset  = dap.dest_offset;
            uint16_t dest_segment = dap.dest_segment;
            uint16_t sectors_done = 0;
            uint8_t  error        = 0;

            for (uint32_t s = 0; s < sector_count; s++) {
                uint32_t total_offset = (uint32_t)dest_offset + ((uint32_t)s * 512);
                uint16_t cur_seg    = dest_segment + (uint16_t)((total_offset >> 4) & 0xF000);
                uint16_t cur_offset = (uint16_t)(total_offset & 0xFFFF);

                error = ide_read_sector(lba + s, dest_segment, cur_offset);                
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
                regs->ax    = ((uint16_t)error << 8);
                regs->flags |= ISR_FLAGS_CF;
            }
            break;
        }

        case 0x48: { // Get Drive Parameters Extended
            uint16_t buf_segment = regs->ds;
            uint16_t buf_offset  = regs->si;

            // check buffer-size (first two bytes)
            uint16_t buf_size = readFarWord(buf_segment, buf_offset);

            struct drive_params_ext params;
            params.size           = buf_size;
            params.flags          = 0x0002;        // Bit 1: LBA supported
            params.cylinders      = hd0_params.cylinders;
            params.heads          = hd0_params.heads;
            params.sectors        = hd0_params.sectors_per_track;
            params.total          = (uint64_t)hd0_params.cylinders * (uint64_t)hd0_params.heads * (uint64_t)hd0_params.sectors_per_track;
            params.bytes_per_sect = 512; // we only support 512 bytes per sector, so this is fixed to 512

            // write structure in buffer of caller
            uint8_t *p = (uint8_t*)&params;
            for (uint8_t i = 0; i < sizeof(params); i++) {
                writeFarByte(buf_segment, buf_offset + i, p[i]);
            }

            regs->ax    = 0x0000;
            regs->flags &= ~ISR_FLAGS_CF;
            break;
        }

        default:
            regs->ax = 0x0100;
            regs->flags |= ISR_FLAGS_CF; // Carry Set
            break;
    }
}

// uart-interrupt
__attribute__((externally_visible, regparm(1))) void c_int14_handler(struct interrupt_registers *regs) {
    #if BIOS_DEBUG == 1
        uart_print_string("I14\n");
    #endif

    // get registers
    uint8_t ah = (uint8_t)(regs->ax >> 8);
    uint8_t al = (uint8_t)(regs->ax & 0xFF);

	switch (ah) {
        case 0x00: // initializing
            // DOS wants to initalize with ax = 2400 Baud, no parity, 1 stopbit and 8 databits

            // the desired port is stored in register DX
            // DOS initializes from 0x03 downto 0x00
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

// multi-purpose Interrupt 15
__attribute__((externally_visible, regparm(1))) void c_int15_handler(struct interrupt_registers *regs) {
    #if BIOS_DEBUG == 1
        uart_print_string("I15\n");

        lcd_print_string("I15", 0x07);
        lcd_print_uint16(regs->ax, true);
    #endif

    uint8_t ah = (uint8_t)(regs->ax >> 8);
    uint8_t al = (uint8_t)(regs->ax & 0xFF);

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
            // the following functions are rarely documented and are related to the IBM Micro Channel
            // we do not support this

            // DOS hangs after this interrupt. AX=0x4101 which stands for
            // "Wait for External Event"

            switch(al) {
                case 0x01: {
                    regs->ax = 0x0000;
                    regs->flags &= ~ISR_FLAGS_CF; 
                    break;
                }
                default:
                    regs->ax = 0x8600;
                    regs->flags |= ISR_FLAGS_CF;
                    break;
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
            // Request Extended Memory Size
            regs->ax = BIOS_TOTAL_MEMORY_MB * 1024;
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

            regs->es = 0xF000;
            regs->bx = (uint16_t)(uintptr_t)&sysconfig; // pointer to config. TODO: check if this is correct

            // we do not support this for now
            regs->ax = 0x0000;
            regs->flags &= ~ISR_FLAGS_CF;  // Set Carry Flag
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

        case 0xE8: {
            switch (al) {
                case 0x20:
                    // memmap
                    break;
                case 0x01:
                    // memsize2
                    break;
            }
            break;
            }
        case 0x8A: {
            // memsize3
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
    #if BIOS_DEBUG == 1
        uart_print_string("I16\n");
        lcd_print_string("I16", 0x07);
        lcd_print_uint16(regs->ax, true);
    #endif

    uint8_t ah = regs->ax >> 8;

    switch (ah) {
        case 0x00: // Read key, blocking
            regs->flags |= ISR_FLAGS_ZF; // no key in buffer
            break;

        case 0x01: // Check key
            if (readFarWord(0x0000, BDA_KBD_HEAD) != readFarWord(0x0000, BDA_KBD_TAIL)) {
                // a new key is in buffer
                uint16_t current_head = readFarWord(0x0000, BDA_KBD_HEAD);
                regs->ax = readFarWord(0x0000, current_head);

                // move head forward
                uint16_t next_head = current_head + sizeof(uint16_t);
                if (next_head > BDA_KBD_BUF_END) {
                    next_head = BDA_KBD_BUF_START;
                }
                writeFarWord(0x0000, BDA_KBD_HEAD, next_head);

                regs->flags &= ~ISR_FLAGS_ZF;
            } else {
                // buffer empty -> set ZF
                regs->flags |= ISR_FLAGS_ZF;
            }
            break;

        default:
            regs->flags |= ISR_FLAGS_ZF;
            break;
    }
}

// parallel printer
__attribute__((externally_visible, regparm(1))) void c_int17_handler(struct interrupt_registers *regs) {
    #if BIOS_DEBUG == 1
        uart_print_string("I17\n");
    #endif

    // DOS sends AH = 0x01 to initialize the printer port
    // in AL the desired port is given

    regs->ax = 0x0000;
    regs->flags &= ~ISR_FLAGS_CF; // clear carray-flag on success
}

// boot-process
__attribute__((externally_visible, regparm(1))) void c_int19_handler(struct interrupt_registers *regs) {
    #if BIOS_DEBUG == 1
        uart_print_string("I19\n");
        lcd_print_string("I19", 0x07);
    #endif

    lcd_print_string("Rebooting...", 0x07);

	// reset DS to 0
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
}

// INT 1Ah for RTC access
__attribute__((externally_visible, regparm(1))) void c_int1a_handler(struct interrupt_registers *regs) {
    #if BIOS_DEBUG == 1
        uart_print_string("I1A\n");
        lcd_print_string("I1A", 0x7F);
    #endif

    uint8_t ah = (uint8_t)(regs->ax >> 8);

    switch (ah) {
        case 0x00: { // Get system time
            uint16_t timer_lo = readFarWord(0x0000, BDA_TIMER_COUNTER_LOW);
            uint16_t timer_hi = readFarWord(0x0000, BDA_TIMER_COUNTER_HIGH);
            uint8_t midnight = readFarByte(0x0000, BDA_MIDNIGHT_FLAG);

            regs->cx = timer_hi;
            regs->dx = timer_lo;
            regs->ax = 0x0000 | midnight;       // AL = midnight flag, AH = 0
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

        case 0x02: { // Read RTC
            uint16_t hour = 0;
            uint8_t min = 0;
            uint16_t sec = 0;

            regs->cx = (hour << 8) | min;
            regs->dx = (sec << 8);
            regs->flags &= ~ISR_FLAGS_CF;
            break;
        }

        default:
            regs->ax = 0x8600;
            regs->flags |= ISR_FLAGS_CF;
            break;
    }
}

// INT29h: Fast Console Output
__attribute__((externally_visible, regparm(1))) void c_int29_handler(struct interrupt_registers *regs) {
    #if BIOS_DEBUG == 1
        uart_print_string("INT29\n");
        uart_putc(regs->ax & 0xFF);
        lcd_print_string("INT29\n", 0x07);
        lcd_putc((char)(regs->ax & 0xFF), 0x07);
    #endif
}

static uint8_t pic_read_isr_master(void) {
    outb(0x20, 0x0B);
    return inb(0x20);
}

static uint8_t pic_read_irr_master(void) {
    outb(0x20, 0x0A);
    return inb(0x20);
}

__attribute__((externally_visible, regparm(1))) void c_int_error_handler(struct interrupt_registers *regs) {
    uart_print_string("IEE\n");
    lcd_print_string("IEE", 0x07);

    __asm__ volatile ("cli; hlt");
}

__attribute__((externally_visible, regparm(1))) void c_int_dummy_handler(struct interrupt_registers *regs) {
    uart_print_string("I??\n");
    lcd_print_string("I??", 0x07);

    __asm__ volatile ("cli; hlt");

/*
    char buf[5];
    
    // IP und CS ausgeben
    lcd_print_string_pos(4, 0, "IP=", 0x07);
    uint16_to_hex(regs->ip, buf);
    lcd_print_string_ram(buf, 0x07);
    
    lcd_print_string(" CS=", 0x07);
    uint16_to_hex(regs->cs, buf);
    lcd_print_string_ram(buf, 0x07);
    
    lcd_print_string(" FL=", 0x07);
    uint16_to_hex(regs->flags, buf);
    lcd_print_string_ram(buf, 0x07);

    // PIC-Status
    outb(0x20, 0x0B);       // ISR lesen
    uint8_t isr = inb(0x20);
    outb(0x20, 0x0A);       // IRR lesen
    uint8_t irr = inb(0x20);
    
    lcd_print_string_pos(5, 0, "ISR=", 0x07);
    uint8_to_hex(isr, buf);
    lcd_print_string_ram(buf, 0x07);
    lcd_print_string(" IRR=", 0x07);
    uint8_to_hex(irr, buf);
    lcd_print_string_ram(buf, 0x07);
    
    // IMR lesen
    uint8_t imr = inb(0x21);
    lcd_print_string(" IMR=", 0x07);
    uint8_to_hex(imr, buf);
    lcd_print_string_ram(buf, 0x07);
    
    // INT-Code
    uint8_t opcode = readFarByte(regs->cs, regs->ip - 2);  // sollte 0xCD sein
    uint8_t int_no = readFarByte(regs->cs, regs->ip - 1);  // INT-Nummer

    lcd_print_string_pos(6, 0, "INT=", 0x07);
    uint8_to_hex(int_no, buf);
    lcd_print_string_ram(buf, 0x07);
    uint8_to_hex(opcode, buf);
    lcd_print_string(" OP=", 0x07);
    lcd_print_string_ram(buf, 0x07);
    
    // EINFRIEREN für Analyse!
    __asm__ volatile ("cli; hlt");

    // this interrupt is not supposed to be called, but if it is called, we print some debug information and halt the system

    uart_print_string("DUMMY INT / EXCEPTION\n");

    uart_print_string("CS=");
    uart_putc(regs->cs >> 8);
    uart_putc(regs->cs & 0xFF);

    uart_print_string(" IP=");
    uart_putc(regs->ip >> 8);
    uart_putc(regs->ip & 0xFF);

    uart_print_string(" AX=");
    uart_putc(regs->ax >> 8);
    uart_putc(regs->ax & 0xFF);

    uint8_t isr = pic_read_isr_master();
    uint8_t irr = pic_read_irr_master();

    uart_print_string(" PIC ISR=");
    uart_putc(isr);
    uart_print_string(" IRR=");
    uart_putc(irr);

    uart_print_string("\nHALT\n");
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
*/
}
