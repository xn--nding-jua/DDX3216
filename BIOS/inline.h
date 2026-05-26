#ifndef INLINE_H_
#define INLINE_H_

#include "registers.h"

// **********************************************************
// General inline-functions
// **********************************************************

// functions for accessing I/O space
// ==========================================================
// write 8-bit data in 16-bit I/O-space
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// write 16-bit data in 16-bit I/O-space
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

// read 8-bit data in 16-bit I/O-space
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

// read 16-bit data in 16-bit I/O-space
static inline uint16_t inw(uint16_t port) {
    uint16_t val;
	__asm__ volatile ("inw %1, %0" : "=a" (val) : "Nd" (port));
    return val;
}

// functions for accessing config spaces
// ==========================================================

static inline void write_sc300_cfg(uint16_t reg, uint8_t data) {
	outb(CFG_ADDR, reg);
	outb(CFG_DATA, data);
}

static inline uint8_t read_sc300_cfg(uint16_t reg) {
	outb(CFG_ADDR, reg);
	return inb(CFG_DATA);
}

static inline void write_sc300_cfgw(uint16_t reg, uint16_t data) {
	outb(CFG_ADDR, reg);
	outb(CFG_DATA, (uint8_t)(data & 0xFF));
	
	outb(CFG_ADDR, reg + 1);
	outb(CFG_DATA, (uint8_t)((data >> 8) & 0xFF));
}

static inline void write_sc300_lcd_cfg(uint16_t reg, uint8_t data) {
	outb(LCD_CGA_IDX_ADDR, reg);
	outb(LCD_CGA_IDX_DATA, data);
}

static inline void write_sc300_rtc_cfg(uint16_t reg, uint8_t data) {
	outb(RTC_IDX_ADDR, reg);
	outb(RTC_IDX_DATA, data);
}

static inline uint8_t read_sc300_rtc_cfg(uint16_t reg) {
	outb(RTC_IDX_ADDR, reg);
	return inb(RTC_IDX_DATA);
}

// functions for accessing ROM space
// ==========================================================
static inline uint8_t readRomByte(uint16_t rom_offset) {
    uint8_t val;
    __asm__ __volatile__(
        "movb %%cs:(%1), %0"  // Liest das Byte über CS:[SI]
        : "=q"(val)           // Ausgabe: val (in einem Byte-Register wie AL, BL, CL, DL)
        : "S"(rom_offset)     // Eingabe: "S" zwingt GCC hart zu SI!
    );
    return val;
}

static inline uint16_t readRomWord(uint16_t rom_offset) {
    uint16_t val;
    __asm__ __volatile__(
        "movw %%cs:(%1), %0"  // Liest das Word über CS:[SI]
        : "=r"(val)           // Ausgabe: val (16-Bit Register)
        : "S"(rom_offset)     // Eingabe: Zwingt GCC zu SI
    );
    return val;
}

// functions for accessing RAM space in other than the current segment
// ==========================================================
static inline void writeFarByte(uint16_t segment, uint16_t offset, uint8_t value) {
    __asm__ __volatile__(
        "pushw %%es\n\t"         // store current ES
        "movw %0, %%es\n\t"      // load desired segment into ES register
        "movb %2, %%es:(%1)\n\t" // write Byte to ES:[BX]
        "popw %%es"              // restore es
        :
        : "rm"(segment), "b"(offset), "qi"(value) // "b" zwingt GCC zu BX!
        : "memory"
    );
}

static inline void writeFarWord(uint16_t segment, uint16_t offset, uint16_t value) {
    __asm__ __volatile__(
        "pushw %%es\n\t"         // store current ES
        "movw %0, %%es\n\t"      // load segment into ES register
        "movw %2, %%es:(%1)\n\t" // write Word to ES:[BX]
        "popw %%es"              // restore es
        :
        : "rm"(segment), "b"(offset), "ri"(value) // "b" zwingt GCC zu BX!
        : "memory"
    );
}

static inline uint8_t readFarByte(uint16_t segment, uint16_t offset) {
    uint8_t value;
    
    __asm__ __volatile__(
        "movw %1, %%es\n\t"   // load desired segment into ES register
        "movb %%es:(%2), %0"  // read bytes from ES-segment at given offset into 'value'
        : "=q"(value)         // store output to 'value'
        : "rm"(segment), "b"(offset) // inputs are segment and offset
        : "memory"
    );
    
    return value;
}

static inline uint16_t readFarWord(uint16_t segment, uint16_t offset) {
    uint16_t value;
    
    __asm__ __volatile__(
        "movw %1, %%es\n\t"   // load desired segment into ES register
        "movw %%es:(%2), %0"  // read word from ES-segment at given offset into 'value'
        : "=r"(value)         // store output to 'value'
        : "rm"(segment), "b"(offset) // inputs are segment and offset
        : "memory"
    );
    
    return value;
}

static inline void copyFarBlock(uint16_t srcSegment, uint16_t srcOffset, void* dest, uint32_t len) {
    __asm__ __volatile__(
        "push %%ds\n\t"
        "movw %0, %%ds\n\t"    // source-segment (DS)
        "rep movsb\n\t"        // copies from DS:ESI to ES:EDI
        "pop %%ds"
        :
        : "r"(srcSegment), "S"(srcOffset), "D"(dest), "c"(len)
        : "memory"
    );
}


// additional functions
// ==========================================================
static inline void delay_1us(void) {
    outb(0x80, 0x00);   // write 0x00 to port 0x80, which takes about 1 microsecond
}

static inline void delay_1ms(void) {
    for (int i = 0; i < 1000; i++) {
        delay_1us();
    }
}

#endif
