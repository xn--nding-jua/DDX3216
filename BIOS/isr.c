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

// IRQ1 -> INT 09h: keyboard-interrupt
__attribute__((externally_visible)) void c_int09_handler() {
    #if BIOS_DEBUG == 1
        uart_print_string("I09\n");
        lcd_print_string("I09", 0x07);
    #endif

    // read scancode from keyboard-controller
	uint8_t xt_scancode = inb(KBD_DATA_PORT);

    #if BIOS_DEBUG == 1
        lcd_print_string_pos(7, 0, "XT ScanCode: 0x", 0x07);
        lcd_print_uint16(xt_scancode, true);
    #endif

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
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) & ~KBD_FLAG_LSHIFT);
                break;
            case 0x36: // Right Shift
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) & ~KBD_FLAG_RSHIFT);
                break;
            case 0x1D: // Ctrl
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) & ~KBD_FLAG_CTRL);
                break;
            case 0x38: // Alt
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) & ~KBD_FLAG_ALT);
                break;
            case 0x46: // Scrolllock
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) & ~KBD_FLAG_SCROLLLOCK);
                break;
            case 0x45: // NumLock
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) & ~KBD_FLAG_NUMLOCK);
                break;
            case 0x3A: // CapsLock
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) & ~KBD_FLAG_CAPSLOCK);
                break;
            case 0x52: // Insert
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) & ~KBD_FLAG_INSERTMODE);
                break;
            default:
                // regular key-releases will not be handled here
                break;
        }
    }else{
        // make (key is pressed down)
        switch(xt_scancode) {
            case 0x2A: // Left Shift
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) | KBD_FLAG_LSHIFT);
                break;
            case 0x36: // Right Shift
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) | KBD_FLAG_RSHIFT);
                break;
            case 0x1D: // Ctrl
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) | KBD_FLAG_CTRL);
                break;
            case 0x38: // Alt
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) | KBD_FLAG_ALT);
                break;
            case 0x46: // Scrolllock
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) | KBD_FLAG_SCROLLLOCK);
                break;
            case 0x45: // NumLock
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) | KBD_FLAG_NUMLOCK);
                break;
            case 0x3A: // CapsLock
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) | KBD_FLAG_CAPSLOCK);
                break;
            case 0x52: // Insert
                writeFarByte(0x0000, BDA_KBD_STATUS_FLAG0, readFarByte(0x0000, BDA_KBD_STATUS_FLAG0) | KBD_FLAG_INSERTMODE);
                break;
            default: {
                // regular key, put scancode into keyboard-buffer
                char ascii = 0; // DOS will receive and check the original XT scancode in AH if no ASCII-char is recognized below

                uint8_t status_flags = readFarByte(0x0000, BDA_KBD_STATUS_FLAG0);

                // check if the XT-scancode is within the ASCII-characters
                if (xt_scancode < sizeof(xt_to_ascii_normal)) {
                    // check if shift, capslock or ctrl+alt is active
                    uint16_t rom_offset;
                    if (status_flags & (KBD_FLAG_LSHIFT | KBD_FLAG_RSHIFT | KBD_FLAG_CAPSLOCK)) {
                        // shift or capslock is pressed
                        rom_offset = (uint16_t)(uintptr_t)&xt_to_ascii_shift[xt_scancode];
                        ascii = (char)readRomByte(rom_offset);
                    }else if ((status_flags & KBD_FLAG_CTRL) && (status_flags & KBD_FLAG_ALT)) {
                        // ALT+GR is pressed
                        //rom_offset = (uint16_t)(uintptr_t)&xt_to_ascii_altgr[xt_scancode];
                        //ascii = (char)readRomByte(rom_offset);
                    }else if (status_flags & KBD_FLAG_CTRL) {
                        // CTRL is pressed
                        if (xt_scancode == 0x2E) {
                            // CTRL + C is pressed

                            // trigger software-interrupt 1B immediately
                            __asm__ volatile ("int $0x1B");

                            // return ETX and the xt-scancode
                            ascii = 0x03; // send ETX
                        }else if (xt_scancode == 0x1F) {
                            // CTRL + S is pressed

                            ascii = 0x13; // send DeviceControl3
                        }
                    }else if (status_flags & KBD_FLAG_ALT) {
                        // ALT is pressed
                    }else{
                        rom_offset = (uint16_t)(uintptr_t)&xt_to_ascii_normal[xt_scancode];
                        ascii = (char)readRomByte(rom_offset);
                    }
                }else{
                    // check for some control keys like CTRL+ALT+DELETE
                    if ((status_flags & KBD_FLAG_CTRL) && (status_flags & KBD_FLAG_ALT)) {
                        // ALT+GR is pressed
                        if (xt_scancode == 0x53) {
                            // CTRL + ALT + DELETE is pressed -> reboot
                            cpu_reset();
                        }
                    }
                }

                // if valid ascii-character, put it into the keyboard-buffer together with the scancode, otherwise ignore it
                if ((ascii > 0) || (xt_scancode > 0)) {
                    // get current keyboard-buffer tail
                    uint16_t tail = readFarWord(0x0000, BDA_KBD_TAIL_PTR);

                    // read start/end of ringbuffer from BDA
                    uint16_t buf_start = readFarWord(0x0000, BDA_KBD_BUF_START_PTR);
                    uint16_t buf_end = readFarWord(0x0000, BDA_KBD_BUF_END_PTR);

                    // calc next tail with wraparound
                    uint16_t next_tail = tail + sizeof(uint16_t);
                    if (next_tail >= buf_end) {
                        next_tail = buf_start;
                    }

                    // check if buffer is full: this is when (tail + 2 == head)
                    if (next_tail == readFarWord(0x0000, BDA_KBD_HEAD_PTR)) {
                        // buffer overflow -> ignore new key-press
                        // regular BIOS would beep here, but we have no speaker
                    } else {
                        // BIOS is storing a 16-bit value for each key-press in the keyboard-buffer,
                        // where the high-byte is the scancode and the low-byte is the ASCII-character

                        // High-Byte = Scancode, Low-Byte = ASCII
                        uint16_t key_data = ((uint16_t)xt_scancode << 8) | (uint8_t)ascii;
                        
                        // write current keydata to keyboard-buffer
                        writeFarWord(0x0040, tail, key_data); // tail is stored as offset in segment 0x0040!

                        // update tail-pointer
                        writeFarWord(0x0000, BDA_KBD_TAIL_PTR, next_tail);
                    }
                }

                break;
            }
        }
    }
}

// INT04 (PIRQ0 is mapped here)
__attribute__((externally_visible)) void c_int0c_handler() {
    #if BIOS_DEBUG == 1
        uart_print_string("I0C\n");
        lcd_print_string("I0C", 0x07);
    #endif

	while (!(inb(UART_IIR) & IIR_PENDING)) {
        uint8_t reason = inb(UART_IIR) & IIR_REASON;

        if (reason == IIR_RLS) {
            // receiver line status (errors, break)
            inb(UART_LSR); // reading LSR clears the interrupt
        }else if (reason == IIR_RDA) {
            // receive data available
            uint8_t c = inb(UART_RBR); // this resets the IRQ
        }else if (reason == IIR_TIMEOUT) {
            inb(UART_RBR); // this resets the IRQ
        }else if (reason == IIR_THRE) {
            // reading IIR or writing THR will reset this IRQ
        }else if (reason == IIR_MS) {
            inb(UART_MSR); // this resets the IRQ
        }
    }
}

// INT 10h: Video-interrupt
__attribute__((externally_visible, regparm(1))) void c_int10_handler(struct interrupt_registers *regs) {
    // read registers first
    uint8_t ah = (uint8_t)(regs->ax >> 8);
    uint8_t al = (uint8_t)(regs->ax & 0xFF);

    if (ah == 0x00) {
        // set video mode
        // for videomodes see here: https://mendelson.org/wpdos/videomodes.txt
        if (al == 0x03) {
            // standard DOS color-textmode
            lcd_init(true);
            lcd_clear();
        }else if (al == 0x06) {
            // 640x200 monochrome pixel graphics mode
            lcd_init(false);
            lcd_clear();
        }else{
            // unsupported mode
        }
    }else if (ah == 0x01) {
        // set text-mode cursor shape
        writeFarWord(0x0000, BDA_CURSOR_STYLE, regs->cx);
    }else if (ah == 0x02) {
        // set cursor position
        uint8_t dh = (uint8_t)(regs->dx >> 8); // DH = Row
        uint8_t dl = (uint8_t)(regs->dx & 0xFF); // DL = Column
        if ((dh < 8) && (dl < 30)) {
            writeFarByte(BASE_SEG, BDA_CURSOR_POS_ROW, dh);
            writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, dl);

            // update hardware cursor position
            uint16_t offset = ((readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) * (LCD_WIDTH / 8)) + readFarByte(BASE_SEG, BDA_CURSOR_POS_COL)) * 2;
            write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_UPPER, (offset >> 9) & 0xFF); // upper 7 bits of offset (divide by 512)
            write_sc300_lcd_cfg(LCD_VID_IDX_CURSOR_ADDR_LOWER, (offset >> 1) & 0xFF); // lower 8 bits of offset (divide by 2)
        }
    }else if (ah == 0x03) {
        // get cursor position and shape
        // return cursor position in DX (DH = Row, DL = Column)
        regs->ax = 0x0000;
        regs->cx = readFarWord(0x0000, BDA_CURSOR_STYLE); // CH = start scanline of cursor, CL = end scanline of cursor
        regs->dx = (readFarByte(BASE_SEG, BDA_CURSOR_POS_ROW) << 8) | readFarByte(BASE_SEG, BDA_CURSOR_POS_COL);
    }else if (ah == 0x06) {
        // scroll window up
        if (al == 0) {
            // scroll full window up
            lcd_scroll_up();
            lcd_scroll_up();
            lcd_scroll_up();
            lcd_scroll_up();
            lcd_scroll_up();
            lcd_scroll_up();
            lcd_scroll_up();
        }
    //}else if (ah == 0x07) {
        // scroll windows down
    }else if (ah == 0x09) {
        // write character and attribute at cursor position
        // regs->cx contains number of timers of character

        //uint8_t attr = regs->bx & 0xFF;
        //for (uint16_t i = 0; i < regs->cx; i++) {
        //    lcd_putc(al, attr);
        //}
        lcd_putc(al, regs->bx & 0xFF);
    }else if (ah == 0x0A) {
        // write character only at cursor position
        // regs->cx contains number of timers of character
        //for (uint16_t i = 0; i < regs->cx; i++) {
        //    lcd_putc(al, 0x07);
        //}
        lcd_putc(al, 0x07); // light gray on black
    }else if (ah == 0x0C) {
        // write graphics pixel
        // AL = Color, BH = Page Number, CX = x, DX = y
        lcd_draw_pixel(regs->cx, regs->dx, al);
    }else if (ah == 0x0D) {
        // read graphics pixel
        // BH = Page Number, CX = x, DX = y
        // return AL = Color
        regs->ax = (regs->ax & 0xFF00) | lcd_read_pixel(regs->cx, regs->dx);
    }else if (ah == 0x0E) {
        // write character
        #if BIOS_DEBUG == 1
            uart_putc(al);
        #endif

        // write char to display and handle control-characters like newline, carriage return, backspace, etc. internally
        lcd_putc(al, 0x07); // light gray on black
    }else if (ah == 0x0F) {
        // get Current Video Mode
        // AL = Video Mode, AH = number of character columns, BH = active page

        // AH = Columns (40), AL = Video Mode (00h), BH = Page (0)
        // number of rows is set in BDA and not returned via INT10h, because DOS will ignore the returned value anyway and would always assume 25 rows
        regs->ax = ((uint16_t)30 << 8) | 0x03; // 40x25 char in grayscale-text mode
        regs->bx = 0x0000; // BH = 0
    }else if (ah == 0x13) {
        // write string

        bool update_cursor = al & 0x01;
        bool has_attr      = al & 0x02;

        uint8_t row = regs->dx >> 8;
        uint8_t col = regs->dx & 0xFF;

        writeFarByte(BASE_SEG, BDA_CURSOR_POS_ROW, row);
        writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, col);

        uint16_t ptr = regs->bp;

        for (uint16_t i = 0; i < regs->cx; i++) {
            char ch = (char)readFarByte(regs->es, ptr++);

            uint8_t attr;
            if (has_attr) {
                attr = readFarByte(regs->es, ptr++);
            } else {
                attr = regs->bx & 0xFF;
            }

            lcd_putc(ch, attr);
        }

        if (!update_cursor) {
            writeFarByte(BASE_SEG, BDA_CURSOR_POS_ROW, row);
            writeFarByte(BASE_SEG, BDA_CURSOR_POS_COL, col);
        }

        regs->flags &= ~ISR_FLAGS_CF;
    }else{
        // simply ignore other video-functions like set cursor, etc.
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

static uint8_t __attribute__((noinline)) do_read_sectors(uint32_t start_lba, uint16_t seg, uint16_t off, uint16_t count) {
    uint32_t linear = ((uint32_t)seg << 4) + off;

    for (uint16_t s = 0; s < count; s++) {
        uint16_t cur_seg = (uint16_t)(linear >> 4);
        uint16_t cur_off = (uint16_t)(linear & 0x000F);

        uint8_t err = ide_read_sector(start_lba + (uint32_t)s, cur_seg, cur_off);
        if (err) {
            return err;
        }

        linear += 512;
    }
    return 0;
}

static uint8_t __attribute__((noinline)) do_write_sectors(uint32_t start_lba, uint16_t seg, uint16_t off, uint16_t count) {
    uint32_t linear = ((uint32_t)seg << 4) + off;

    for (uint16_t s = 0; s < count; s++) {
        uint16_t cur_seg = (uint16_t)(linear >> 4);
        uint16_t cur_off = (uint16_t)(linear & 0x000F);

        uint8_t err = ide_write_sector(start_lba + (uint32_t)s, cur_seg, cur_off);
        if (err) {
            return err;
        }

        linear += 512;
    }
    return 0;
}

static uint8_t __attribute__((noinline)) do_verify_sectors(uint32_t start_lba, uint16_t count) {
    for (uint16_t s = 0; s < count; s++) {
        uint8_t err = ide_verify_sector(start_lba + (uint32_t)s);
        if (err) return err;
    }
    return 0;
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

    if (ah == 0x00) {
        // Reset Disk
        outb(IDE_DEV_CTRL, 0x06);
        for (volatile uint16_t i = 0; i < 10000; i++);
        outb(IDE_DEV_CTRL, 0x02);
        ide_wait_ready();
        regs->ax = 0x0000; // Return-Code = Success
        regs->flags &= ~ISR_FLAGS_CF;
    }else if (ah == 0x01) {
        // Get Status of Last Drive Operation
        regs->ax = 0x0000; // return no error every time. TODO: implement a global variable to return the correct error-code
        regs->flags &= ~ISR_FLAGS_CF;
    }else if (ah == 0x02) {
        // Read Sectors from drive
        uint8_t  sectors_to_read = al;
        uint8_t  sector           = cl & 0x3F;
        uint16_t cylinder         = (((uint16_t)(cl & 0xC0)) << 2) | ch;
        uint8_t  head             = dh;

        // loop through all requested sectors
        uint8_t error = do_read_sectors(
            disk_chs_to_lba(cylinder, head, sector),
            regs->es,   // segment of destination buffer
            regs->bx,   // offset within ES
            sectors_to_read
        );

        if (error) {
            regs->ax     = ((uint16_t)error << 8) | 0;
            regs->flags |= ISR_FLAGS_CF;
        }else{
            regs->ax     = sectors_to_read;
            regs->flags &= ~ISR_FLAGS_CF;
        }
    }else if (ah == 0x03) {
        // write sectors to drive
        uint8_t  sectors_to_write = al;
        uint8_t  sector           = cl & 0x3F;
        uint16_t cylinder         = (((uint16_t)(cl & 0xC0)) << 2) | ch;
        uint8_t  head             = dh;

        uint8_t error = do_write_sectors(
            disk_chs_to_lba(cylinder, head, sector),
            regs->es,   // segment of destination
            regs->bx,   // offset within destination-segment
            sectors_to_write
        );

        if (error) {
            regs->ax     = ((uint16_t)error << 8) | 0;
            regs->flags |= ISR_FLAGS_CF;
        }else{
            regs->ax     = sectors_to_write;
            regs->flags &= ~ISR_FLAGS_CF;
        }
    }else if (ah == 0x04) {
        // verify Sectors
        uint8_t  sectors_to_verify = al;
        uint8_t  sector            = cl & 0x3F;
        uint16_t cylinder          = (((uint16_t)(cl & 0xC0)) << 2) | ch;
        uint8_t  head              = dh;

        // loop through all requested sectors
        uint8_t error = do_verify_sectors(
            disk_chs_to_lba(cylinder, head, sector),
            sectors_to_verify
        );

        if (error) {
            regs->ax     = ((uint16_t)error << 8) | 0;
            regs->flags |= ISR_FLAGS_CF;
        }else{
            regs->ax     = sectors_to_verify;
            regs->flags &= ~ISR_FLAGS_CF;
        }
    }else if (ah == 0x08) {
        // Get Drive Parameters
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
    }else if (ah == 0x15) {
        // Get Disk Type
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
    }else if (ah == 0x41) {
        // Extensions Present
        // LBA-Support

        // check for magic word 0x55AA
        if (regs->bx != 0x55AA) {
            regs->flags |= ISR_FLAGS_CF;  // CF=1: Fehler
            regs->ax = 0x0100;
        }else{
            // enable LBA-support
            regs->bx    = 0xAA55;      // send back magic word
            //regs->ax    = 0x3000;      // AH = Version 3.0
            //regs->cx    = 0x0007;      // Bit 0: Extended Disk Access
                                        // Bit 1: Removable Drive Support
                                        // Bit 2: EDD (Enhanced Disk Drive)
            regs->ax = 0x2100;
            regs->cx = 0x0001;          // only support extended Read/Write AH=42h/43h

            regs->flags &= ~ISR_FLAGS_CF;    // CF=0: Extensions available
        }
    }else if (ah == 0x42) {
        // Extended Read Sectors
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
        if (dap.size < 0x10) {
            regs->ax    = 0x0100; // bad data
            regs->flags |= ISR_FLAGS_CF;
        }else if (dap.lba_high != 0) { // 64-bit LBA: upper 32 Bit must be 0 (CF-Card < 4GB)
            regs->ax    = 0x0B00; // Data error / LBA out of bounds (AH=0Bh)
            regs->flags |= ISR_FLAGS_CF;
        }else{
            // loop through all requested sectors
            uint8_t error = do_read_sectors(
                dap.lba_low,
                dap.dest_segment,  // segment of destination buffer
                dap.dest_offset,   // offset within segment
                dap.sector_count
            );

            if (error) {
                // update read sectors in DAP
                //writeFarByte(dap_segment, dap_offset + 2, (uint8_t)(sectors_read & 0xFF));
                //writeFarByte(dap_segment, dap_offset + 3, (uint8_t)(sectors_read >> 8));

                regs->ax     = ((uint16_t)error << 8) | 0;
                regs->flags |= ISR_FLAGS_CF;
            }else{
                regs->ax     = 0x0000; // return code = no error
                regs->flags &= ~ISR_FLAGS_CF;
            }
        }
    }else if (ah == 0x43) {
        // Extended Write Sectors
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
        if (dap.size < 0x10) {
            regs->ax    = 0x0100; // bad data
            regs->flags |= ISR_FLAGS_CF;
        }else if (dap.lba_high != 0) { // 64-bit LBA: upper 32 Bit must be 0 (CF-Card < 4GB)
            regs->ax    = 0x0B00; // Data error / LBA out of bounds (AH=0Bh)
            regs->flags |= ISR_FLAGS_CF;
        }else{
            // loop through all requested sectors
            uint8_t error = do_write_sectors(
                dap.lba_low,
                dap.dest_segment,  // segment of destination buffer
                dap.dest_offset,   // offset within segment
                dap.sector_count
            );

            if (error) {
                // update read sectors in DAP
                //writeFarByte(dap_segment, dap_offset + 2, (uint8_t)(sectors_written & 0xFF));
                //writeFarByte(dap_segment, dap_offset + 3, (uint8_t)(sectors_written >> 8));

                regs->ax     = ((uint16_t)error << 8) | 0;
                regs->flags |= ISR_FLAGS_CF;
            }else{
                regs->ax     = 0x0000; // return code = no error
                regs->flags &= ~ISR_FLAGS_CF;
            }
        }
    }else if (ah == 0x48) {
        // Get Drive Parameters Extended
        uint16_t buf_segment = regs->ds;
        uint16_t buf_offset  = regs->si;

        uint16_t caller_size = readFarWord(buf_segment, buf_offset);

        if (caller_size < 0x1A) {
            regs->ax = 0x0100;
            regs->flags |= ISR_FLAGS_CF;
        }else{
            struct drive_params_ext params;
            params.size           = 0x001A;
            params.flags          = 0x0002;        // geometry valid / LBA info
            params.cylinders      = hd0_params.cylinders;
            params.heads          = hd0_params.heads;
            params.sectors        = hd0_params.sectors_per_track;
            params.total          = (uint64_t)hd0_params.cylinders * (uint64_t)hd0_params.heads * (uint64_t)hd0_params.sectors_per_track;
            params.bytes_per_sect = 512; // we only support 512 bytes per sector, so this is fixed to 512

            // write structure in buffer of caller
            uint16_t write_size = sizeof(params);
            if (write_size > caller_size) {
                write_size = caller_size;
            }

            uint8_t *p = (uint8_t*)&params;
            for (uint16_t i = 0; i < write_size; i++) {
                writeFarByte(buf_segment, buf_offset + i, p[i]);
            }

            regs->ax    = 0x0000;
            regs->flags &= ~ISR_FLAGS_CF;
        }
    }else{
        regs->ax = 0x0100;
        regs->flags |= ISR_FLAGS_CF; // Carry Set
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

	if (ah == 0x00) {
        // initializing
        // DOS wants to initalize with ax = 2400 Baud, no parity, 1 stopbit and 8 databits

        // the desired port is stored in register DX
        // DOS initializes from 0x03 downto 0x00
        regs->ax    = 0x0000;
        regs->flags &= ~ISR_FLAGS_CF;
    }else if (ah == 0x01) {
        // transmit char
        uart_putc((char)al);
        regs->ax    = 0x6000;
        regs->flags &= ~ISR_FLAGS_CF;
    }else if (ah == 0x02) {
        // receive char (not implemented)
        regs->ax    = 0x8000; // timeout
        regs->flags &= ~ISR_FLAGS_CF;
    }else if (ah == 0x03) {
        // request state
            uint8_t status = inb(UART_LSR); // LSR
            uint8_t modem = inb(UART_MSR);  // MSR
            regs->ax    = ((uint16_t)status << 8) | modem;
            regs->flags &= ~ISR_FLAGS_CF;
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

    if (ah == 0x24) {
        // A20 Gate Control
        if (al == 0x00) { // disable Gate A20
            a20_disable();
            regs->ax = 0x0000;
            regs->flags &= ~ISR_FLAGS_CF; // Success
        }else if (al == 0x01) { // enable Gate A20
            a20_enable();
            regs->ax = 0x0000;
            regs->flags &= ~ISR_FLAGS_CF; // Success
        } else if (al == 0x02) { // request if Gate A20 is opened
            regs->ax = a20_is_enabled() ? 0x0001 : 0x0000;
            regs->flags &= ~ISR_FLAGS_CF;
        } else {
            regs->ax = 0x8600; // unknown AL-Subcode
            regs->flags |= ISR_FLAGS_CF;
        }
    }else if (ah == 0x41) {
        // Wait for External Event

        if (al == 0x01) {
            // wait for keypress
            uint16_t head = readFarWord(0x0000, BDA_KBD_HEAD_PTR);
            uint16_t tail = readFarWord(0x0000, BDA_KBD_TAIL_PTR);

            // wait until a key is within the buffer
            while (tail == head) {
                __asm__ volatile ("sti; hlt; cli" : : : "memory"); // halt CPU until next interrupt
                
                // read buffers again and check if a key has been pressed
                head = readFarWord(0x0000, BDA_KBD_HEAD_PTR);
                tail = readFarWord(0x0000, BDA_KBD_TAIL_PTR);
            }
            
            // a new char is in buffer
            regs->ax &= 0x00FF;
            regs->flags &= ~ISR_FLAGS_CF;
        }else{
            regs->ax &= 0x00FF; // clear AH, but keep AL
            regs->flags &= ~ISR_FLAGS_CF;
        }
    }else if (ah == 0x83) {
        // Set Event Wait Interval
        if (al == 0x00) {
            // set interval
            uint16_t microsec_low = regs->cx;
            uint16_t microsec_high = regs->dx;
            
            // store address of User-Flag (will be submitted in ES:BX)
            writeFarWord(0x0000, BDA_WAIT_COMPLETE, regs->bx);       // Offset
            writeFarWord(0x0000, BDA_WAIT_SEGMENT, regs->es);       // Segment
            
            // convert microseconds to timer-ticks (value / 55000)
            uint32_t total_micro = ((uint32_t)microsec_high << 16) | microsec_low;
            uint32_t ticks = total_micro / 55000; // 1 Sekunde / 18.207 Ticks ~ 0.055000 Sekunden
            if (ticks == 0) ticks = 1;
            
            writeFarWord(0x0000, BDA_WAIT_COUNT_LOW, (uint16_t)(ticks & 0xFFFF));
            writeFarWord(0x0000, BDA_WAIT_COUNT_HIGH, (uint16_t)(ticks >> 16));
            
            // set Wait Active Flag
            writeFarByte(0x0000, BDA_WAIT_ACTIVE_FLAG, 0x01);
            
            regs->flags &= ~ISR_FLAGS_CF; // success
        } else if (al == 0x01) { // delete interval
            writeFarByte(0x0000, BDA_WAIT_ACTIVE_FLAG, 0x00); // disable
            regs->flags &= ~ISR_FLAGS_CF;
        }
    }else if (ah == 0x86) {
        // BIOS Wait / Delay (synchroneous waiting)
        uint32_t total_micro = ((uint32_t)regs->dx << 16) | regs->cx;
        uint32_t ticks_to_wait = total_micro / 55000; // 1 Sekunde / 18.207 Ticks ~ 0.055000 Sekunden

        uint32_t start_ticks = readFarLong(0x0000, BDA_TIMER_COUNTER_LOW);
        while ((readFarLong(0x0000, BDA_TIMER_COUNTER_LOW) - start_ticks) < ticks_to_wait) {
            __asm__ volatile ("hlt" : : : "memory"); // halt CPU until next tick
        }

        regs->flags &= ~ISR_FLAGS_CF;
    }else if (ah == 0x87) {
        // Move block to/from extended memory

        // read GDT-descriptors from ES:SI
        uint16_t gdt_seg = regs->es;
        uint16_t gdt_off = regs->si;
        
        // source-descriptor: Offset 0x10 in GDT-table
        uint32_t* src_base = (uint32_t*)(
            ((uint32_t)readFarByte(gdt_seg, gdt_off + 0x12))       |
            ((uint32_t)readFarByte(gdt_seg, gdt_off + 0x13) << 8)  |
            ((uint32_t)readFarByte(gdt_seg, gdt_off + 0x14) << 16) |
            ((uint32_t)readFarByte(gdt_seg, gdt_off + 0x17) << 24));
        
        // destination-descriptor: Offset 0x18 in GDT-table
        uint32_t* dst_base = (uint32_t*)(
            ((uint32_t)readFarByte(gdt_seg, gdt_off + 0x1A))       |
            ((uint32_t)readFarByte(gdt_seg, gdt_off + 0x1B) << 8)  |
            ((uint32_t)readFarByte(gdt_seg, gdt_off + 0x1C) << 16) |
            ((uint32_t)readFarByte(gdt_seg, gdt_off + 0x1F) << 24));
        
        uint32_t byte_count = (uint32_t)regs->cx * 2; // CX contains number of words
        
        // copy data in extended memory using protected mode
        memcpy(dst_base, src_base, byte_count);

        regs->ax = 0x0000;
        regs->flags &= ~ISR_FLAGS_CF;
    }else if (ah == 0x88) {
        // Request Extended Memory Size
        regs->ax = ((BIOS_TOTAL_MEMORY_MB - 1) * 1024); // remove the lower 1MB
        regs->flags &= ~ISR_FLAGS_CF; // Clear Carry Flag (Success)
    }else if ((ah == 0x90) || (ah == 0x91)) {
        // AH=90h: Device busy
        // AH=91h: Interrupt complete
        regs->ax = 0x0000;
        regs->flags &= ~ISR_FLAGS_CF;
    }else if (ah == 0xC0) {
        // Get Configuration Table
        regs->es = BIOS_SEG;
        regs->bx = (uint16_t)(uintptr_t)&sysconfig; // pointer to config

        regs->ax = 0x0000;
        regs->flags &= ~ISR_FLAGS_CF;  // clear Carry Flag

        /*
        // we do not support this for now
        regs->ax = 0x8600;
        regs->flags |= ISR_FLAGS_CF;  // Set Carry Flag
        */
    }else if (ah == 0xC1) {
        // Get EBDA segment
        regs->es = BIOS_SEG;
        regs->ax = 0x0000;
        regs->flags &= ~ISR_FLAGS_CF;
    }else if (ah == 0xDA) {
        // E820 / alternative memory-APIs
        // we do not support this for now
        regs->ax = 0x8600;
        regs->flags |= ISR_FLAGS_CF;
    }else if (ah == 0xE8) {
        if (al == 0x20) {
            // memmap
            // unsupported
            regs->ax = 0x8600;
            regs->flags |= ISR_FLAGS_CF; // Set Carry Flag
        }else if (al == 0x01) {
            // memsize

            // AX/CX = Memory-size in kB between 1MB and 16MB
            // BX/DX = 64KB-blocks above 16MB
            regs->ax = (BIOS_TOTAL_MEMORY_MB - 1) * 1024;
            regs->bx = 0;
            regs->cx = (BIOS_TOTAL_MEMORY_MB - 1) * 1024;
            regs->dx = 0;
            regs->flags &= ~ISR_FLAGS_CF;
        }else{
            // unsupported
            regs->ax = 0x8600;
            regs->flags |= ISR_FLAGS_CF; // Set Carry Flag
        }
    }else if (ah == 0x8A) {
        // memsize3
        // unsupported
        regs->flags |= ISR_FLAGS_CF; // Set Carry Flag
    }else{
        // Unbekannte Funktion -> Fehler (AH = 0x86, Carry Flag = 1)
        regs->ax = 0x8600;
        regs->flags |= ISR_FLAGS_CF; // Set Carry Flag
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

    if ((ah == 0x00) || (ah == 0x10)) {
        // Read key press (0x00) or Read expanded keyboard character (0x10)

        // this function has to block if buffer is empty
        while (1) {
            uint16_t head = readFarWord(0x0000, BDA_KBD_HEAD_PTR);
            uint16_t tail = readFarWord(0x0000, BDA_KBD_TAIL_PTR);
            
            if (head != tail) {
                // read char from ring-buffer
                regs->ax = readFarWord(0x0040, head); // head is stored at offset within segment 0x0040!

                // read start/end of ringbuffer from BDA
                uint16_t buf_start = readFarWord(0x0000, BDA_KBD_BUF_START_PTR);
                uint16_t buf_end = readFarWord(0x0000, BDA_KBD_BUF_END_PTR);

                // move head forwards
                uint16_t next_head = head + sizeof(uint16_t);
                if (next_head >= buf_end) {
                    next_head = buf_start;
                }

                // write new head
                writeFarWord(0x0000, BDA_KBD_HEAD_PTR, next_head);
                break;
            }else{
                // buffer is empty -> enable interrupts and wait for new key-stroke
                __asm__ volatile ("sti; hlt; cli" : : : "memory");
            }
        }
    }else if ((ah == 0x01) || (ah == 0x11)) {
        // Get the State of the keyboard buffer (0x01) or Obtain status of the expanded keyboard buffer (0x11)
        uint16_t head = readFarWord(0x0000, BDA_KBD_HEAD_PTR);
        uint16_t tail = readFarWord(0x0000, BDA_KBD_TAIL_PTR);    
        
        if (head == tail) {
            // keyboard-buffer is empty -> set zero-flag
            regs->flags |= ISR_FLAGS_ZF;
        } else {
            // keyboard-buffer has some new data -> copy char to AX without changing the buffer
            regs->ax = readFarWord(0x0040, head); 

            // delete zero-flag to show that character is present
            regs->flags &= ~ISR_FLAGS_ZF;
        }
    }else if (ah == 0x02) {
        // Get the State of the keyboard
        regs->ax = (uint16_t)readFarByte(0x0000, BDA_KBD_STATUS_FLAG0);
    }else if (ah == 0x05) {
        // Simulate a keystroke

        // get current keyboard-buffer tail
        uint16_t tail = readFarWord(0x0000, BDA_KBD_TAIL_PTR);

        // read start/end of ringbuffer from BDA
        uint16_t buf_start = readFarWord(0x0000, BDA_KBD_BUF_START_PTR);
        uint16_t buf_end = readFarWord(0x0000, BDA_KBD_BUF_END_PTR);

        // calc next tail with wraparound
        uint16_t next_tail = tail + sizeof(uint16_t);
        if (next_tail >= buf_end) {
            next_tail = buf_start;
        }

        if (next_tail == readFarWord(0x0000, BDA_KBD_HEAD_PTR)) {
            // buffer overflow -> ignore new key-press
            // regular BIOS would beep here, but we have no speaker
            regs->ax = 0x0001;
            regs->flags |= ISR_FLAGS_CF;
        }else{
            // get the simulated key-data (CH = ScanCode, CL = ASCII Character)
            uint16_t key_data = regs->cx;

            // write current keydata to keyboard-buffer
            writeFarWord(0x0040, tail, key_data); // tail is stored as offset in segment 0x0040!

            // update tail-pointer
            writeFarWord(0x0000, BDA_KBD_TAIL_PTR, next_tail);

            regs->ax = 0x0000;
            regs->flags &= ~ISR_FLAGS_CF;
        }
    }else if (ah == 0x0A) {
        // Get the ID of the keyboard

        // if we would have a modern keyboard, we could return 0xAB41 in register BX
        //regs->bx = 0xAB41; // extended keyboard MF-II

        // but the AMD ELAN SC300 only supports XT-keyboards, so we must not alter register BX
        regs->ax &= 0x00FF; // clear AH, but leave AL unchanged
    }else if (ah == 0x12) {
        // Get expanded keyboard status
    }else{
        regs->flags |= ISR_FLAGS_ZF;
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

    if (ah == 0x00) {
        // Get system time
        uint16_t timer_lo = readFarWord(0x0000, BDA_TIMER_COUNTER_LOW);
        uint16_t timer_hi = readFarWord(0x0000, BDA_TIMER_COUNTER_HIGH);
        uint8_t midnight = readFarByte(0x0000, BDA_MIDNIGHT_FLAG);

        regs->cx = timer_hi;
        regs->dx = timer_lo;
        regs->ax = 0x0000 | midnight;    // AL = midnight flag, AH = 0
        regs->flags &= ~ISR_FLAGS_CF;    // CF=0

        // Optional: clear IBM-compatible midnight-flag after reading
        writeFarByte(0x0000, BDA_MIDNIGHT_FLAG, 0);
    }else if (ah == 0x01) {
        // Set system time CX:DX
        writeFarWord(0x0000, BDA_TIMER_COUNTER_LOW, regs->dx);
        writeFarWord(0x0000, BDA_TIMER_COUNTER_HIGH, regs->cx);
        writeFarByte(0x0000, BDA_MIDNIGHT_FLAG, 0);
        regs->ax = 0x0000;
        regs->flags &= ~ISR_FLAGS_CF;
    }else if (ah == 0x02) {
        // Read RTC
        uint16_t hour = 0;
        uint8_t min = 0;
        uint16_t sec = 0;

        regs->cx = (hour << 8) | min;
        regs->dx = (sec << 8);
        regs->flags &= ~ISR_FLAGS_CF;
    }else{
        regs->ax = 0x8600;
        regs->flags |= ISR_FLAGS_CF;
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

    char c = (char)(regs->ax & 0xFF);
    lcd_putc(c, 0x07);
    regs->flags &= ~ISR_FLAGS_CF;
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
