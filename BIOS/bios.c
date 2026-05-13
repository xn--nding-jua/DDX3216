/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
	
	mov, push, pop will control MEMR# and MEMW#
	in, out will control IOR# and IOW#
*/

#include "bios.h"

// **********************************************************
// service functions
// **********************************************************

void io_delay() {
	// a simple I/O-access to a "safe" port takes around 1 microsecond
    outb(0x80, 0x00);
}

void setup_ivt() {
    // fill IVT (Interrupt Vector Table)
    uint16_t *ivt = (uint16_t *)0;
    uint16_t code_seg;

    __asm__ volatile ("movw %%cs, %0" : "=r"(code_seg));

	// dummy_callbacks for all interrupts
	for (uint16_t i = 0; i < 256; i++) {
		ivt[i * 2]     = (uint16_t)(uintptr_t)c_int_dummy_handler;
		ivt[i * 2 + 1] = code_seg; // 0xF000
	}

	// register callback-handlers for specific interrupts
	// ivt is a 16-bit pointer so "* 2" increases by 2 bytes!
    ivt[0x08 * 2]     = (uint16_t)(uintptr_t)c_int08_handler;
    ivt[0x09 * 2]     = (uint16_t)(uintptr_t)c_int09_handler;
    ivt[0x10 * 2]     = (uint16_t)(uintptr_t)c_int10_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x11 * 2]     = (uint16_t)(uintptr_t)c_int11_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x12 * 2]     = (uint16_t)(uintptr_t)c_int12_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x13 * 2]     = (uint16_t)(uintptr_t)c_int13_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x14 * 2]     = (uint16_t)(uintptr_t)c_int14_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x16 * 2]     = (uint16_t)(uintptr_t)c_int16_handler; // double-cast to mitigate warning about 32/16-bit pointer
    //ivt[0x0C * 2]     = (uint16_t)(uintptr_t)c_int0c_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x1A * 2]     = (uint16_t)(uintptr_t)c_int1a_handler; // double-cast to mitigate warning about 32/16-bit pointer
    ivt[0x1C * 2]     = (uint16_t)(uintptr_t)c_int1c_handler; // double-cast to mitigate warning about 32/16-bit pointer
}

void setup_bda() {
	// create DOS-compatibility by updating BDA

	*(uint8_t *)BDA_VIDEO_INFO = 0x03; // current video mode (80x25 Text)
	*(uint16_t *)BDA_VIDEO_COLUMS = 80;   // number of columns

	// set address of COM1 (external UART)
    *(uint16_t *)BDA_COM1_BASE = UART_BASE; // COM1 = 0x1000

	// set Equipment Word within BDA to tell DOS that we have an UART and a CGA-graphic
	//                                     FEDCBA9876543210
	*(uint16_t *)BDA_EQUIPMENT_WORD = 0b0000001000010000; // 1 COM-port / CGA-graphics

}

uint16_t ram_test_and_setup() {
    uint16_t count = 0;

    // uint32_t für die Adressberechnung verwenden!
    for (uint16_t kb = 32; kb < 640; kb++) {
        // Overflow-sichere Adressberechnung
        uint32_t addr = (uint32_t)kb * 1024;
        
        // Segmentgrenze beachten: max 0xFFFF im 16-Bit Segment
        if (addr > 0xFFFF) break;
        
        // VRAM-Bereich auslassen
        if (addr >= VRAM_BASE && addr < (VRAM_BASE + VRAM_SIZE)) {
            count = kb;
            continue;   // VRAM nicht testen
        }

        volatile uint16_t *ptr = (volatile uint16_t*)((uintptr_t)addr);
        *ptr = 0xAA55;
        if (*ptr != 0xAA55) break;
        *ptr = 0x55AA;
        if (*ptr != 0x55AA) break;
        count = kb;
    }

    *(uint16_t*)BDA_MEM_SIZE = count;
    return count;
}

bool kbd_init() {
    // selftest of the controller (optional)
	bool kbd_ok = false;
    outb(KBD_STATUS_PORT, 0xAA);
    while (!(inb(KBD_STATUS_PORT) & KBD_STAT_OBF));
    if (inb(KBD_DATA_PORT) == 0x55) {
        kbd_ok = true;
    }
    
	*BDA_KBD_HEAD = BDA_KBD_BUF_START;
    *BDA_KBD_TAIL = BDA_KBD_BUF_START;

    // Keyboard Interface aktivieren
    outb(KBD_STATUS_PORT, 0xAE);
	
	return kbd_ok;
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

void wdt_disable() {
	write_sc300_cfg(0xCB, 0x00);
}

void boot_dos() {
    uint8_t status = 0xFF;
    uint8_t retries = 3;
    
    // load MBR via INT 13h (AH=02)
    while (retries-- > 0) {
        __asm__ volatile (
            "pushw %%es\n"          // ← 'w' suffix erzwingt 16-Bit Operation
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
            "popw %%es\n"           // ← 'w' suffix erzwingt 16-Bit Operation
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
    }

    if (status != 0) {
        uart_print("Disk read failed after 3 retries!\n");
        return;
    }

    // check boot-signature at the end of the MBR
    uint16_t *sig = (uint16_t *)0x7DFE;
    if (*sig != 0xAA55) {
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

// **********************************************************
// main function
// **********************************************************

void bios_main() {
	// disable the watchdog
	wdt_disable();
	
    uart_init(38400);
	//pirq_init();
	//uart_interrupt_enable();
    uart_print("DDX3216 BIOS Booting...\n");

    uart_print("Initializing PIC...\n");
    pic_init();
    
    uart_print("Setting IVT and BDA...\n");
	setup_ivt();
	setup_bda();

    uart_print("Initializing LCD...\n");
    lcd_init();
    lcd_clear_test();
	
	uart_print("Initializing keyboard...");
	if (kbd_init()) {
		uart_print(" OK\n");
	}else{
		uart_print(" missing...\n");
	}
	
    uart_print("Initializing PCMCIA / CF-Card...\n");
	pcmcia_init();
	
	uart_print("Initializing timer...\n");
	timer_init();

    uart_print("Starting RAM-Test...\n");
	ram_test_and_setup();

    uart_print("Enabling Gate A20 via Port 0x92...\n");
	a20_enable();

    uart_print("Enabling interrupts...\n");
	__asm__ volatile ("sti");

    uart_print("Booting from CF-Card...\n");
	boot_dos();

    while(1) {
	}; 
}
