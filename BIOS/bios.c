/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
	
	mov, push, pop will control MEMR# and MEMW#
	in, out will control IOR# and IOW#
*/

__asm__(".code16gcc\n"); // we are using -m16 compiler-switch so this line is redundant

#include "bios.h"

// **********************************************************
// service functions
// **********************************************************

void setup_ivt() {
    volatile struct ivt_entry* ivt = (volatile struct ivt_entry*)0x0000;
    // set DS to segment 0x0000 temporarily to write to IVT with absolute addresses
    __asm__ __volatile__(
        "pushw %ds\n\t"
        "xorw %ax, %ax\n\t"
        "movw %ax, %ds\n\t"
    );

    // dummy_callbacks for all interrupts
    for (uint16_t i = 0; i < 256; i++) {
        ivt[i].offset = (uint16_t)(uintptr_t)c_int_dummy_handler;
        ivt[i].segment = ROM_SEG; // keep ROM-Segment
    }

  	// register callback-handlers for specific interrupts
	// the following code will produce warnings during compilation, but
	// the warning is the proof of using the 16-bit "iret" after the ISR
    ivt[0x08].offset = (uint16_t)(uintptr_t)c_int08_handler; // double-cast to mitigate warning about 32/16-bit pointer
    //ivt[0x08].offset = (uint16_t)(uintptr_t)isr_int08;
    ivt[0x09].offset = (uint16_t)(uintptr_t)c_int09_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x10].offset = (uint16_t)(uintptr_t)c_int10_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x11].offset = (uint16_t)(uintptr_t)c_int11_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x12].offset = (uint16_t)(uintptr_t)c_int12_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x13].offset = (uint16_t)(uintptr_t)c_int13_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x14].offset = (uint16_t)(uintptr_t)c_int14_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x15].offset = (uint16_t)(uintptr_t)c_int15_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x16].offset = (uint16_t)(uintptr_t)c_int16_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x19].offset = (uint16_t)(uintptr_t)c_int19_handler; // double-cast to mitigate warning about 32/16-bit pointer
    //ivt[0x0C].offset = (uint16_t)(uintptr_t)c_int0c_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x1A].offset = (uint16_t)(uintptr_t)c_int1a_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x1C].offset = (uint16_t)(uintptr_t)c_int1c_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x41].offset = (uint16_t)(uintptr_t)&hd0_params; // direct to disk-parameters in ROM for INT 41h

    // reset DS to original value
    __asm__ __volatile__(
        "popw %ds\n\t"
    );
}

void setup_bda() {
    // BDA is within our STACK_SEG so we can access it with 16-bit pointers directly

    volatile struct bios_data_area* bda = (volatile struct bios_data_area*)0x0400;
    // set DS to segment 0x0000 temporarily to write to BDA with absolute addresses
    __asm__ __volatile__(
        "pushw %ds\n\t"
        "xorw %ax, %ax\n\t"
        "movw %ax, %ds\n\t"
    );
 
    bda->com_ports[0] = UART_BASE; // COM1 = 0x1000 = external UART at I/O Port 0x1000
    bda->com_ports[1] = 0x0000; // COM2 = not available
    bda->com_ports[2] = 0x0000; // COM3 = not available
    bda->com_ports[3] = 0x0000; // COM4 = not available

    bda->lpt_ports[0] = 0x0000; // LPT1 = not available
    bda->lpt_ports[1] = 0x0000; // LPT2 = not available
    bda->lpt_ports[2] = 0x0000; // LPT3 = not available

    bda->ebda_base_address = 0x0000; // EBDA-Address (not used)

	// set Equipment Word within BDA to tell DOS that we have an UART and a CGA-graphic
	//                                     FEDCBA9876543210
    bda->equipment_word = 0b0000001000010000; // 1 COM-port / CGA-graphics
    bda->base_memory_kb = 640; // base memory size in kB (standard: 640 kB)
    bda->kbd_status_flags1 = 0x00; // Keyboard-Status-Flags (e.g. Numlock/Capslock active, etc.)
    bda->kbd_status_flags2 = 0x00; // Keyboard-Status-Flags (e.g. Numlock/Capslock active, etc.)

    // create DOS-compatibility by updating BDA
    bda->video_mode = 0x00; // current video mode (40x25 grayscale-text, but we are using only 30x8)
    bda->video_columns = 30; // number of columns
    bda->video_page_size = 512; // (LCD_WIDTH / 8) * (LCD_HEIGHT / 8) * 2 = 480 -> rounded to 512 // size of one video page in bytes (virtual CGA resolution)
    bda->video_page_start = 0x0000; // start offset of page relative to video memory
    bda->cursor_position[0].row = 0; // initial cursor position (row)
    bda->cursor_position[0].col = 0; // initial cursor position (column)
    bda->cursor_type = 0x0607; // cursor type (start/end scanline)
    bda->active_video_page = 0; // active video page index
    bda->crtc_port_address = LCD_CGA_IDX_ADDR; // I/O Port of Videochip (CGA = 0x3D4)
    bda->video_rows = (LCD_HEIGHT / 8) - 1; //  number of rows - 1
    bda->character_points = 8; // bytes per character (font height)
    bda->ext_memory_kb = 1024; // extended memory size in kB above 1MB (for INT15h)

    // reset DS to original value
    __asm__ __volatile__(
        "popw %ds\n\t"
    );
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
        lcd_print_string_ram(1, 11, textbuffer, 0x07);

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
        lcd_print_string_ram(1, 11, textbuffer, 0x07);

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
    if (test_ram_range(0x07C0, 0x9000)) {
        lcd_print_string(1, 11, "OK    ", 0x07); // delete the last 3 chars from memory-test as well
    } else {
        lcd_print_string(1, 11, "ERROR!", 0x07);
        while(1) { __asm__("hlt"); }
    }
}

static void kbd_wait_write() {
	// check if write-buffer is full
    while (inb(KBD_STATUS_PORT) & KBD_STATUS_IBF) {
        __asm__ __volatile__("nop");
    }
}

static void kbd_wait_ready(void) {
    // hold clock low for at least one clock-cycle
    uint8_t ctrl = inb(KBD_CTRL_PORT);
    outb(KBD_CTRL_PORT, ctrl | KBD_CTRL_CLK_LOW);
    // small delay
    for (volatile int i = 0; i < 100; i++);
    outb(KBD_CTRL_PORT, ctrl & ~KBD_CTRL_CLK_LOW);
}

void kbd_init() {
    // enable XT-Keyboard in SC300
    uint8_t pmu3 = read_sc300_cfg(0xAD);
    write_sc300_cfg(0xAD, pmu3 | SC300_XTKBDEN);

    // initialize keyboard-buffer
    // set DS to segment 0x0000 temporarily to write to IVT with absolute addresses
    __asm__ __volatile__(
        "pushw %ds\n\t"
        "xorw %ax, %ax\n\t"
        "movw %ax, %ds\n\t"
    );
    *BDA_KBD_HEAD = BDA_KBD_BUF_START;
    *BDA_KBD_TAIL = BDA_KBD_BUF_START;

    // reset DS to original value
    __asm__ __volatile__(
        "popw %ds\n\t"
    );

    // clear keyboard-register and IRQ
    uint8_t ctrl = inb(KBD_CTRL_PORT);
    outb(KBD_CTRL_PORT, ctrl | KBD_CTRL_CLEAR);   // set Bit 7
    outb(KBD_CTRL_PORT, ctrl & ~KBD_CTRL_CLEAR);  // clear Bit 7

    // enable IRQ1 in PIC
    uint8_t imr = inb(0x21);
    outb(0x21, imr & ~(1 << 1));
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
    outb(0x21, 0xFC);   // 1111 1100: nur IRQ0+IRQ1 erlaubt
    outb(0xA1, 0xFF);   // Slave komplett sperren
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
    uint8_t status = 0xFF;
    uint8_t retries = 3;
    
    // load MBR via INT 13h (AH=02)
    while (retries-- > 0) {
        if (ide_read_bootsector()) {
            status = 0; // success
            break;
        }

        /*
        __asm__ volatile (
            "pushw %%es\n"
            "xorw %%ax, %%ax\n"
            "movw %%ax, %%es\n"
            "movb $0x02, %%ah\n"
            "movb $0x01, %%al\n"
            "movb $0x00, %%ch\n"
            "movb $0x01, %%cl\n"
            "movb $0x00, %%dh\n"
            "movb $0x80, %%dl\n"
            "movw $0x7C00, %%bx\n"
            "int $0x13\n"
            "movb %%ah, %0\n"
            "popw %%es\n"
            : "=rm"(status)
            :
            : "ax", "bx", "cx", "dx", "memory"
        );

        if (status == 0) break;

        uart_print("Disk read error, retrying...\n");

        // INT 13h AH=00: Reset drive before retry
        __asm__ volatile (
            "movb $0x00, %%ah\n"
            "movb $0x80, %%dl\n"
            "int $0x13\n"
            ::: "ax", "dx"
        );
        */
    }

    if (status != 0) {
        uart_print("Disk read failed after 3 retries!\n");
        return;
    }

    // check boot-signature at the end of the MBR
    uint16_t signature = readFarWord(BASE_SEG, 0x7C00 + 510);
    if (signature != 0xAA55) {
        uart_print("No Boot Signature found!\n");
        return;
    }
    
    uart_print("Booting from CF-Card...\n");
    
    // jump to the loaded MBR at 0x7C00
    __asm__ volatile (
        "movb $0x80, %%dl\n"    // Boot-Drive in DL (DOS-convention)
        "ljmpw $0x0000, $0x7C00\n"
        ::: "dl"
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
    if (*(volatile uint16_t*)BDA_SOFT_RESET_FLAGS == 0x1234) {
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
	lcd_print_string(0, 0, "AMD Elan SC300 BIOS v0.01", 0x07);

    //lcd_print_string(1, 0, "RAM-Test...", 0x07);
	//ram_test_and_setup(); // this function halts the CPU on any RAM-error

    lcd_print_string(2, 0, "Init PIC and IVT...", 0x07);
    pic_init();
	setup_ivt();
	setup_bda();

    lcd_print_string(3, 0, "Init UART...", 0x07);
    uart_init(9600);
    //pirq_init();
	//uart_interrupt_enable();
	uart_print("AMD Elan SC300 BIOS v0.01\n");

	lcd_print_string(4, 0, "Init timer...", 0x07);
	timer_init();

    lcd_print_string(5, 0, "Init keyboard...", 0x07);
	kbd_init();
	
	//lcd_print_string(6, 0, "Init PCMCIA / CF-Card...", 0x07);
	//pcmcia_init();

    // enable interrupts
	__asm__ volatile ("sti");
    setLEDs();

    // try to load DOS from CF-Card and boot it
    lcd_print_string(7, 0, "Ready.", 0x07);
    //lcd_print_string(7, 0, "Booting from CF-Card...", 0x07);
	//boot_dos();

    uint8_t c = 'A';
    while(1) {
        // show a sign of life
        lcd_putc_pos(0, 0, c, 0x07);
        c++;
        if (c > 'z') c = 'A';

        // check BIOS Keyboard-Ringbuffer

        char textbuffer[3];
        uint8_to_hex(g_kbd_scancode, textbuffer);
        lcd_putc_pos(6, 0, textbuffer[0], 0x07);
        lcd_putc_pos(6, 1, textbuffer[1], 0x07);


        /*
        if (*BDA_KBD_HEAD != *BDA_KBD_TAIL) {
            uint16_t* ptr = (uint16_t*)(uintptr_t)(*BDA_KBD_HEAD);
            uint8_t scancode = (*ptr) >> 8;
            char ascii = (*ptr) & 0xFF;

            // display scancode and ASCII-character on LCD
            lcd_print_string(7, 0, "Scancode: 0x", 0x07);
            char textbuffer[3];
            uint8_to_hex(scancode, textbuffer);
            lcd_putc_pos(7, 15, textbuffer[0], 0x07);
            lcd_putc_pos(7, 16, textbuffer[1], 0x07);

            lcd_print_string(7, 18, "ASCII: ", 0x07);
            lcd_putc_pos(7, 25, ascii ? ascii : '.', 0x07);

            // move head forward
            uint16_t next_head = *BDA_KBD_HEAD + 2;
            if (next_head >= BDA_KBD_BUF_END) next_head = BDA_KBD_BUF_START;
            *BDA_KBD_HEAD = next_head;
        }
        */

        delay_1ms();
    }
}
