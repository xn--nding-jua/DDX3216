/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
	
	mov, push, pop will control MEMR# and MEMW#
	in, out will control IOR# and IOW#

    Some interesting connections
    - PGP0 is connected to the external WatchDog-Input WDI of the ADM699
    - external UART INT0 and INT1 are connected to a single PIRQ0 (mapped to INT0Ch) and can trigger interrupts there

*/

__asm__(".code16gcc\n"); // we are using -m16 compiler-switch so this line is redundant but keeping is safe

#include "bios.h"

// **********************************************************
// service functions
// **********************************************************

void setup_ivt() {
    for (uint16_t i = 0; i < 8; i++) {
        set_ivt_entry((uint8_t)i, (uint16_t)(uintptr_t)isr_int_error, ROM_SEG);
    }

    for (uint16_t i = 8; i < 256; i++) {
        set_ivt_entry((uint8_t)i, (uint16_t)(uintptr_t)isr_int_dummy, ROM_SEG);
    }

    set_ivt_entry(0x04, (uint16_t)(uintptr_t)isr_int04, ROM_SEG);
    set_ivt_entry(0x08, (uint16_t)(uintptr_t)isr_int08, ROM_SEG);
    set_ivt_entry(0x09, (uint16_t)(uintptr_t)isr_int09, ROM_SEG);
    set_ivt_entry(0x10, (uint16_t)(uintptr_t)isr_int10, ROM_SEG);
    set_ivt_entry(0x11, (uint16_t)(uintptr_t)isr_int11, ROM_SEG);
    set_ivt_entry(0x12, (uint16_t)(uintptr_t)isr_int12, ROM_SEG);
    set_ivt_entry(0x13, (uint16_t)(uintptr_t)isr_int13, ROM_SEG);
    set_ivt_entry(0x14, (uint16_t)(uintptr_t)isr_int14, ROM_SEG);
    set_ivt_entry(0x15, (uint16_t)(uintptr_t)isr_int15, ROM_SEG);
    set_ivt_entry(0x16, (uint16_t)(uintptr_t)isr_int16, ROM_SEG);
    set_ivt_entry(0x17, (uint16_t)(uintptr_t)isr_int17, ROM_SEG);
    set_ivt_entry(0x19, (uint16_t)(uintptr_t)isr_int19, ROM_SEG);
    set_ivt_entry(0x1a, (uint16_t)(uintptr_t)isr_int1a, ROM_SEG);
    set_ivt_entry(0x1c, (uint16_t)(uintptr_t)isr_int1c, ROM_SEG);
    set_ivt_entry(0x29, (uint16_t)(uintptr_t)isr_int29, ROM_SEG);
    set_ivt_entry(0x41, (uint16_t)(uintptr_t)&hd0_params, ROM_SEG);


    // spurious interrupts
    set_ivt_entry(0x0F, (uint16_t)(uintptr_t)isr_spurious_irq7, ROM_SEG);
    set_ivt_entry(0x77, (uint16_t)(uintptr_t)isr_spurious_irq15, ROM_SEG);
}

void setup_bda() {
    // set UART-ports
    writeFarWord(0x0000, BDA_COM1_BASE, UART_BASE);
    writeFarWord(0x0000, BDA_COM1_BASE + 2, 0x0000);
    writeFarWord(0x0000, BDA_COM1_BASE + 4, 0x0000);
    writeFarWord(0x0000, BDA_COM1_BASE + 6, 0x0000);

    // set LPT-ports
    writeFarWord(0x0000, BDA_LPT1_BASE, 0x0000);
    writeFarWord(0x0000, BDA_LPT1_BASE + 2, 0x0000);
    writeFarWord(0x0000, BDA_LPT1_BASE + 4, 0x0000);

    // EBDA Base
	writeFarWord(0x0000, BDA_EBDA_BASE, BIOS_SEG);
	writeFarWord(BIOS_SEG, 0x0000, BIOS_RESERVED_KB);

	// set Equipment Word within BDA to tell DOS that we have an UART and a CGA-graphic
    writeFarWord(0x0000, BDA_EQUIPMENT_WORD, 0b0000001000010000); // 1 COM-port / CGA-graphics

    // set conventional memory
	writeFarWord(0x0000, BDA_MEM_SIZE, BIOS_CONVENTIONAL_KB);
	
    // set keyboard state flag
	writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS, 0x0000);
	writeFarByte(0x0000, BDA_KBD_STATUS_FLAGS + 1, 0x0000);
	
	// set video information
    writeFarByte(0x0000, BDA_VIDEO_MODE, 0x00);    // video-mode = text
	writeFarWord(0x0000, BDA_VIDEO_COLUMS, 40);    // number of columns. We have only 30 rows, but we tell DOS that we have 40 columns, because otherwise some software would not work at all
	writeFarWord(0x0000, 0x044C, 512);     // size of Video-Page in bytes: (LCD_WIDTH / 8) * (LCD_HEIGHT / 8) * 2 = 480 -> rounded to 512 // size of one video page in bytes (virtual CGA resolution)
    writeFarWord(0x0000, 0x044E, 0x0000);  //  Start-Offset of VRAM for current page (in Bytes, relative to 0xB8000)

    // cursor position (8x Col/Row)
    //writeFarWord(0x0000, BDA_CURSOR_POS_COL, 0x0000);     // cursor-position is already be used
    //writeFarWord(0x0000, BDA_CURSOR_POS_COL + 2, 0x0000); // cursor-position is already be used
    writeFarWord(0x0000, BDA_CURSOR_POS_COL + 4, 0x0000);
    writeFarWord(0x0000, BDA_CURSOR_POS_COL + 6, 0x0000);
    writeFarWord(0x0000, BDA_CURSOR_POS_COL + 8, 0x0000);
    writeFarWord(0x0000, BDA_CURSOR_POS_COL + 10, 0x0000);
    writeFarWord(0x0000, BDA_CURSOR_POS_COL + 12, 0x0000);
    writeFarWord(0x0000, BDA_CURSOR_POS_COL + 14, 0x0000);
    writeFarWord(0x0000, 0x0460, 0x0607);
	
    // set Active Video-Page index
    writeFarByte(0x0000, 0x0462, 0);

    // set I/O Port of Videochip (z.B. 0x3D4 für CGA)
    writeFarWord(0x0000, BDA_VIDEO_IO_BASE, LCD_CGA_IDX_ADDR); // I/O Port of Videochip (CGA = 0x3D4)

    // Number of screen rows MINUS 1 (Important for your 8-row LCD!)
    writeFarByte(0x0000, BDA_VIDEO_ROWS, (LCD_HEIGHT / 8) - 1); //  number of rows - 1

    // set Bytes per character (Font height, e.g. 8)
    writeFarWord(0x0000, 0x0485, 8);

    // extended memory size in kB above 1MB (for INT15h)
    writeFarWord(0x0000, 0x49B, 1024);
}

// in 16-bit RealMode we can only access RAM below 1MB
int test_ram_range(uint16_t start_seg, uint16_t end_seg) {
    uint16_t test_pattern1 = 0x55AA;
    uint16_t test_pattern2 = 0xAA55;
    char textbuffer[6];
    uint32_t tested_kb = 0;

    // write pattern #1 and check it
    for (uint32_t seg = start_seg; seg <= end_seg; seg += 0x1000) { 
        tested_kb += 64;
        uint16_to_dec(tested_kb, textbuffer); 
        lcd_print_string_ram_pos(1, 11, textbuffer, 0x07);

        // the full loop would take quite long, so we are testing only
        // every 1024th byte per segment
        for (uint32_t off = 0; off < 0xFFFF; off += 64) {
            writeFarWord(seg, off, test_pattern1);
            if (readFarWord(seg, off) != test_pattern1) {
                return 0; // error in RAM
            }
        }
    }

    // write pattern #2 and check it again
    tested_kb = 0; // reset counter
    for (uint32_t seg = start_seg; seg <= end_seg; seg += 0x1000) {
        tested_kb += 64;
        uint16_to_dec(tested_kb, textbuffer);
        lcd_print_string_ram_pos(1, 11, textbuffer, 0x07);

        // the full loop would take quite long, so we are testing only
        // every 1024th byte per segment
        for (uint32_t off = 0; off < 0xFFFF; off += 64) {
            writeFarWord(seg, off, test_pattern2);
            if (readFarWord(seg, off) != test_pattern2) {
                return 0; // error in RAM
            }
        }
    }

    return 1; // RAM OK
}

void ram_test_and_setup() {
	// we have 2MB of RAM installed, but can test only the lower 640kB due to 16-bit
    // RealMode limitations, but this is sufficient for DOS and our LCD framebuffer

    // this will test RAM-segments 0x07C0 (bootsector) to 0x9FFF = the end of conventional memory (640 kB)
    if (test_ram_range(0x07C0, 0x87C0)) {
        lcd_print_string_pos(1, 11, "OK    ", 0x07); // delete the last 3 chars from memory-test as well
    } else {
        lcd_print_string_pos(1, 11, "ERROR!", 0x07);
        while(1) { __asm__("hlt"); }
    }
}

void kbd_init() {
    // enable XT-Keyboard in SC300
    uint8_t pmu3 = read_sc300_cfg(0xAD);
    write_sc300_cfg(0xAD, pmu3 | SC300_XTKBDEN);

    // check for 0xAA from keyboard
    /*    uint32_t timeout = 500;
    uint8_t scancode = inb(KBD_DATA_PORT);
    while ((timeout > 0) || (scancode != 0xAA)) {
        timeout--;
        if (timeout == 0) {
            // an error occured
            break;
        }
        scancode = inb(KBD_DATA_PORT);

        delay_1ms();
    }
    */
    uint8_t scancode = 0xAA; // DEBUG
    if (scancode != 0xAA) {
        // keyboard did not respond correctly
        lcd_print_string("ERROR\n", 0x07);
    }else{
        // keyboard sent the expected 0xAA
        lcd_print_string("OK\n", 0x07);

        // initialize keyboard-buffer (set both pointers to begin of buffer)
        writeFarWord(0x0000, BDA_KBD_HEAD, BDA_KBD_BUF_START);
        writeFarWord(0x0000, BDA_KBD_TAIL, BDA_KBD_BUF_START);

        // clear keyboard-register and IRQ
        uint8_t ctrl = inb(KBD_CTRL_PORT);
        outb(KBD_CTRL_PORT, ctrl | KBD_CTRL_CLEAR);   // set Bit 7
        outb(KBD_CTRL_PORT, ctrl & ~KBD_CTRL_CLEAR);  // clear Bit 7

        // enable IRQ1 in PIC
        uint8_t imr = inb(0x21);
        outb(0x21, imr & ~(1 << 1));
    }
}

void pic_init() {
    // ICW1: Start initialization, edge-triggered, cascade
    outb(0x20, 0x11);   // Master PIC
    outb(0xA0, 0x11);   // Slave PIC

    // ICW2: Interrupt vector offset
    outb(0x21, 0x08);   // Master: IRQ0-7 → INT 08h-0Fh
    outb(0xA1, 0x70);   // Slave:  IRQ8-15 → INT 70h-77h

    // ICW3: Master/Slave Kaskadierung
    outb(0x21, 0x04);   // Master: Slave an IRQ2
    outb(0xA1, 0x02);   // Slave:  Kaskaden-ID = 2

    // ICW4: 8086-Modus
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // OCW1: IRQ-Maske - alle außer Timer(0) und Keyboard(1) sperren
    outb(0x21, 0b11111100);   // demask IRQ0 and IRQ1
    outb(0xA1, 0xFF);   // Slave komplett sperren
}

void cpu_reset() {
    __asm__ volatile ("cli");

    // enable fast-reset by setting bit 0 of port 0x92
    outb(0x92, 0x01);

    // fallback: trigger a triple fault by loading an empty IDT and executing an invalid instruction
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) idtr = { 0, 0 };

    __asm__ volatile (
        "lidt %0    \n\t"
        "int $0     \n\t"
        "hlt        \n\t"
        : : "m"(idtr)
    );

    // this should never be reached
    for(;;) {}
}

bool a20_is_enabled() {
    uint8_t val = inb(0x92);
    return (val & 0x02);
}

void a20_enable() {
	// enable gate A20
    uint8_t val = inb(0x92);
    
    if (!(val & 0x02)) {
        val |= 0x02;     // enable A20
        val &= ~0x01;    // bit 0 is responsible for fast-reset - keep it 0
        outb(0x92, val);
    }
}

void a20_disable() {
    // disable gate A20
    uint8_t val = inb(0x92);
    
    if (val & 0x02) {
        val &= ~0x02;    // disable A20
        val &= ~0x01;    // bit 0 is responsible for fast-reset - keep it 0
        outb(0x92, val);
    }
}

void wdt_disable() {
	write_sc300_cfg(0xCB, 0x00);
}

void boot_dos() {
    /*
    Phase 1: Loading MBR into RAM at 0x7C00 (512 Bytes)
    Phase 2: MBR reads partitiontable, selects the active partition and load first sector into RAM at 0x7C00
             IO.SYS must be the first file in Root-directory to be loaded into RAM when using DOS MBR
    Phase 3: First 3 Sectors of IO.SYS are loaded to RAM and system jumps to IO.SYS
    Phase 4: IO.SYS requests conventional memory via INT12h and loads more sectors to Segment 0x0000:0x0500 and later last segment at end of conv. memory
    Phase 5: IO.SYS loads MSDOS.SYS (60 - 80 sectors)
    Phase 6: MSDOS.SYS loads COMMAND.COM and starts the shell -> DONE
    */
    uint8_t status = 0xFF;
    uint8_t retries = 3;
    
    uart_print("Reading Bootsector...\n");
    while (retries-- > 0) {
        if (ide_read_bootsector() == 0x00) {
            status = 0; // success
            break;
        }
        uart_print("Disk read failed, retrying...\n");
        delay_1ms();
    }

    if (status != 0) {
        uart_print("Disk read failed after 3 retries!\n");
        return;
    }

    // check boot-signature at the end of the MBR
    uart_print("Checking Boot Signature...");
    uint16_t signature = readFarWord(BASE_SEG, 0x7C00 + 510);
    if (signature != 0xAA55) {
        uart_print("ERROR!\n");
        return;
    }else{
        uart_print("OK\n");
    }
    
    uart_print("Booting from CF-Card...\n");
    launch_bootsector();

    __asm__ __volatile__ (
        /* set DataSegment to 0x0000 */
        "xorw %%ax, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        /* set first drive (0x80) to DL */
        "movb $0x80, %%dl\n\t"
        /* far-jump to 0x7C00 */
        "ljmp $0x0000, $0x7C00\n\t"
        : /* no output-register */
        : /* no input-register */
        : "ax", "dx" /* we will overwrite AX and DX */
    );
}

// this is a test-function to fill 8-Bit Shift Register IC73 to control some LEDs
void setLEDs() {
	// LEDs are controlled through a bunch of logic ICs
	// first the control-signals are fed into IC5A and IC6A
	// IC5A controls signals VULTCH, LSSELR, UCSELR and SPTESR
	// IC6A controls signals VUSELW, LSSELW, UCSELW, LSLTCH, FLSET1, FLSET0
	//
	// The Addressbits SA12..15 are used on the IO-bus, resulting in an address-space
	// between 0x1000 and 0xF000. 0x3000 will enable VUSELW on writing and VULTCH on reading the IO bus

	// LEDs 1 and  9 of Channel 1-4 are controlled at address 0x3000 bit 0
	// ...
	// LEDs 8 and 16 of Channel 1-4 are controlled at address 0x3000 bit 7
	//
	// four more shift-registers are connected to the 9th bit of the previous shift-register

	// turn on/off all 5 stages of shift-register
	for (uint8_t i = 0; i < (8 * 5); i++) {
		// odd LEDs are connected to GND
		// even LEDs are connected to VCC
		// the value 0b00000001 will set the leds of 5 shift-registers to a pattern on/off/on/off/...
		// the other 7 shift-register lanes will be inverted
		outb(0x3000, 0b00001111); // output data to databus D0...D7 (will set SRCLK=0 and RCLK=1)
		inb(0x3000); // dummy-read from IO-bus (will set SRCLK=1 and RCLK=0)
	}
	// final dummy-write to set SRCLK=0 and RCLK=1
	outb(0x3000, 0b00000000);
}

// **********************************************************
// main function
// **********************************************************

__attribute__((noreturn)) void bios_main() {
    if (readFarWord(0x0000, BDA_SOFT_RESET_FLAGS) == 0x1234) {
        // warm-boot
    }else{
        // cold-boot
    }

	// disable the watchdog
	wdt_disable();
    delay_1ms();
	a20_enable();

    lcd_init();
    lcd_clear();
    lcd_print_string_pos(0, 0, "AMD Elan SC300 BIOS v0.01   . ", 0x07);

    uint8_t version = read_sc300_cfg(0x64);
    lcd_putc_pos(0, 27, 'A' + (version & 0b00000111) - 1, 0x07);
    lcd_putc_pos(0, 29, '0' + ((version & 0b01111000) >> 3), 0x07);

    //lcd_print_string_pos(1, 0, "RAM-Test...", 0x07);
	//ram_test_and_setup(); // this function halts the CPU on any RAM-error

    lcd_print_string("Init PIC and IVT...\n", 0x07);
    pic_init();
	setup_ivt();
	setup_bda();

    //lcd_print_string("Init UART...\n", 0x07);
    uart_init(9600);
    pirq_init();
	uart_interrupt_enable();
	uart_print("AMD Elan SC300 BIOS v0.01\n");

    lcd_print_string("Init keyboard...", 0x07); // no linefeed here
	kbd_init();
	
	lcd_print_string("Init timer...\n", 0x07);
	timer_init();

    __asm__ volatile ("sti");
    setLEDs();

    lcd_print_string("Init PCMCIA / CF-Card...", 0x07); // no linefeed here
	mms_init();
	if (cfcard_init()) {
        // try to load DOS from CF-Card and boot it
        lcd_print_string("Booting from CF-Card...\n", 0x07);
        boot_dos();
	}else{
        // no CF-card found, launch BASIC instead
        lcd_print_string("No CF-Card. Booting BASIC...\n", 0x07);
        launch_basic();
    }

    uint8_t c = 'A';
    while(1) {
        // show a sign of life
        lcd_putc_pos(0, 0, c, 0x07);
        c++;
        if (c > 'z') c = 'A';

        // DEBUG: check BIOS Keyboard-Ringbuffer
        if (readFarWord(0x0000, BDA_KBD_HEAD) != readFarWord(0x0000, BDA_KBD_TAIL)) {
            uint16_t keyboard_data = readFarWord(0x0000, BDA_KBD_HEAD);
            char ascii = (keyboard_data >> 8) & 0xFF;
            uint8_t scancode = keyboard_data & 0xFF;

            // display scancode and ASCII-character on LCD
            lcd_print_string_pos(7, 0, "Scancode: 0x", 0x07);
            char textbuffer[3];
            uint8_to_hex(scancode, textbuffer);
            lcd_putc_pos(7, 15, textbuffer[0], 0x07);
            lcd_putc_pos(7, 16, textbuffer[1], 0x07);

            lcd_print_string_pos(7, 18, "ASCII: ", 0x07);
            lcd_putc_pos(7, 25, ascii ? ascii : '.', 0x07);

            // move head forward
            uint16_t next_head = readFarWord(0x0000, BDA_KBD_HEAD) + sizeof(uint16_t);
            if (next_head >= readFarWord(0x0000, BDA_KBD_BUF_END)) next_head = readFarWord(0x0000, BDA_KBD_BUF_START);
            writeFarWord(0x0000, BDA_KBD_HEAD, next_head);
        }

        /*
        // DEBUG: poll UART for new characters
        if (inb(UART_LSR) & 0x01) {
            char uart_c = inb(UART_RBR);
            uart_putc(uart_c); // echo back to sender
        }
        */

        delay_1ms();
    }
}
