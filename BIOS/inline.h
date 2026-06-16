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

static inline void write_sc300_hga_cfg(uint16_t reg, uint8_t data) {
	outb(LCD_HGA_IDX_ADDR, reg);
	outb(LCD_HGA_IDX_DATA, data);
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
        "pushw %%es\n\t"
        "movw  %w0, %%es\n\t"
        "movb  %b2, %%es:(%%bx)\n\t"
        "popw  %%es\n\t"
        :
        : "r"(segment), "b"(offset), "q"(value)
        : "memory"
    );
    /*
    __asm__ __volatile__(
        "pushw %%es\n\t"         // store current ES
        "movw %0, %%es\n\t"      // load desired segment into ES register
        "movb %2, %%es:(%1)\n\t" // write Byte to ES:[BX]
        "popw %%es"              // restore es
        :
        : "rm"(segment), "b"(offset), "qi"(value) // "b" zwingt GCC zu BX!
        : "memory"
    );
    */
    /*
    __asm__ __volatile__(
        "movw %0, %%fs\n\t"        // Lade das gewünschte Segment direkt nach FS
        "movb %2, %%fs:(%1)\n\t"   // Schreibe das Byte mittels FS:[BX]
        :
        : "rm"(segment), "b"(offset), "qi"(value)
        : "memory"
    );
    */
	/*
    __asm__ __volatile__(
        "movw %w0, %%fs\n\t"       
        "movb %b2, %%fs:(%w1)\n\t" 
        :
        : "r"(segment), "b"(offset), "r"(value)
        : "memory"
    );
	*/
    /*
    __asm__ __volatile__(
        "pushw %%fs\n\t"       // Altes FS sichern
        "movw %0, %%fs\n\t"    // Segment sauber als 16-Bit laden
        "movw %1, %%bx\n\t"    // Offset explizit in ein echtes 16-Bit-Basisregister
        "movb %2, %%fs:(%%bx)\n\t" // Absolut sichere 16-Bit-Adressierung im Real Mode
        "popw %%fs\n\t"        // FS wiederherstellen
        :
        : "ri"(segment), "ri"(offset), "ri"(value)
        : "bx", "memory"       // "bx" muss in die Clobber-Liste!
    );
    */
}

static inline void writeFarWord(uint16_t segment, uint16_t offset, uint16_t value) {
    __asm__ __volatile__(
        "pushw %%es\n\t"
        "movw  %w0, %%es\n\t"
        "movw  %w2, %%es:(%%bx)\n\t"
        "popw  %%es\n\t"
        :
        : "r"(segment), "b"(offset), "r"(value)
        : "memory"
    );
/*
    __asm__ __volatile__(
        "pushw %%es\n\t"         // store current ES
        "movw %0, %%es\n\t"      // load segment into ES register
        "movw %2, %%es:(%1)\n\t" // write Word to ES:[BX]
        "popw %%es"              // restore es
        :
        : "rm"(segment), "b"(offset), "ri"(value) // "b" zwingt GCC zu BX!
        : "memory"
    );
*/
}

static inline uint8_t readFarByte(uint16_t segment, uint16_t offset) {
    uint8_t value;

    __asm__ __volatile__(
        "pushw %%es\n\t"
        "movw  %w1, %%es\n\t"
        "movb  %%es:(%%bx), %b0\n\t"
        "popw  %%es\n\t"
        : "=q"(value)
        : "r"(segment), "b"(offset)
        : "memory"
    );

    return value;
/*
    uint8_t value;
    
    __asm__ __volatile__(
        "movw %1, %%es\n\t"   // load desired segment into ES register
        "movb %%es:(%2), %0"  // read bytes from ES-segment at given offset into 'value'
        : "=q"(value)         // store output to 'value'
        : "rm"(segment), "b"(offset) // inputs are segment and offset
        : "memory"
    );
    
    return value;
*/
}

static inline uint16_t readFarWord(uint16_t segment, uint16_t offset) {
    uint16_t value;

    __asm__ __volatile__(
        "pushw %%es\n\t"
        "movw  %w1, %%es\n\t"
        "movw  %%es:(%%bx), %w0\n\t"
        "popw  %%es\n\t"
        : "=r"(value)
        : "r"(segment), "b"(offset)
        : "memory"
    );

    return value;
/*
    uint16_t value;
    
    __asm__ __volatile__(
        "movw %1, %%es\n\t"   // load desired segment into ES register
        "movw %%es:(%2), %0"  // read word from ES-segment at given offset into 'value'
        : "=r"(value)         // store output to 'value'
        : "rm"(segment), "b"(offset) // inputs are segment and offset
        : "memory"
    );
    
    return value;
*/
}

static inline void copyFarBlock(uint16_t segment, uint16_t srcOffset, void* destOffset, uint32_t len) {
    __asm__ __volatile__(
        "push %%ds\n\t"
        "movw %0, %%ds\n\t"    // source-segment (DS)
        "rep movsb\n\t"        // copies from DS:ESI to ES:EDI
        "pop %%ds"
        :
        : "r"(segment), "S"(srcOffset), "D"(destOffset), "c"(len)
        : "memory"
    );
}

static inline void copyFarByte(uint16_t segment, uint16_t srcOffset, uint16_t destOffset) {
    __asm__ __volatile__(
        "pushw %%es\n\t"
        "movw  %w0, %%es\n\t"         // load segment to ES
        "movb  %%es:(%%bx), %%al\n\t" // read source (BX) to AL
        "movb  %%al, %%es:(%%di)\n\t" // write AL to destination (DI)
        "popw  %%es\n\t"
        :
        : "r"(segment), "b"(srcOffset), "D"(destOffset)
        : "ax", "memory"
    );
}

static inline void copyFarByteBetweenSegments(uint16_t srcSegment, uint16_t srcOffset, uint16_t destSegment, uint16_t destOffset) {
    __asm__ __volatile__(
        "pushw %%ds\n\t"              // store DS
        "pushw %%es\n\t"              // store ES
        
        "movw  %w0, %%ds\n\t"         // set source-segment to DS
        "movw  %w1, %%es\n\t"         // set destination-segment to ES
        
        "movsb\n\t"                   // copy byte DS:SI to ES:DI
        
        "popw  %%es\n\t"              // restore ES
        "popw  %%ds\n\t"              // restore DS
        :
        : "r"(srcSegment), "r"(destSegment), "S"(srcOffset), "D"(destOffset)
        : "memory"
    );
}

static inline void copyFarBlockBetweenSegments(uint16_t srcSegment, uint16_t srcOffset, uint16_t destSegment, uint16_t destOffset, uint16_t length) {
    if (length == 0) return;

    __asm__ __volatile__(
        "pushw %%ds\n\t"              // store current DS
        "pushw %%es\n\t"              // store current ES
        
        "movw  %w0, %%ds\n\t"         // set source-segment to DS
        "movw  %w1, %%es\n\t"         // set destination-segment to ES
        
        "cld\n\t"                     // clear direction-flag to make sure that SI/DI is counting up
        "rep movsb\n\t"               // repeat movsb CX times!
        
        "popw  %%es\n\t"              // restore ES
        "popw  %%ds\n\t"              // restore DS
        :
        : "r"(srcSegment), "r"(destSegment), "S"(srcOffset), "D"(destOffset), "c"(length)
        : "memory"
    );
}

static inline void readSector8BitFar(uint16_t ide_port, uint16_t dest_seg, uint16_t dest_offset) {
    __asm__ __volatile__(
        "pushw %%es\n\t"              // store current ES
        "movw  %w0, %%es\n\t"         // load destination-segment to ES
        
        "cld\n\t"                     // clear direction-flag to make sure that DI is counting up
        "rep insb\n\t"                // repeat 512 times: read from DX(IDE_PORT) to ES:DI
        
        "popw  %%es\n\t"              // restore ES
        :
        : "r"(dest_seg), "D"(dest_offset), "d"(ide_port), "c"(512)
        : "memory"
    );
}

static inline void writeSector8BitFar(uint16_t ide_port, uint16_t src_seg, uint16_t src_offset) {
    __asm__ __volatile__(
        "pushw %%ds\n\t"
        "movw  %w0, %%ds\n\t"         // load source-segment to DS
        
        "cld\n\t"
        "rep outsb\n\t"               // write 512 bytes from DS:SI to DX(ide_port)
        
        "popw  %%ds\n\t"
        :
        : "r"(src_seg), "S"(src_offset), "d"(ide_port), "c"(512)
        : "memory"
    );
}

// additional functions
// ==========================================================
static inline void delay_1us(void) {
    // writing to IO takes between 22 and 26 clock-cycles
    // at 33MHz this results in about 0.66 to 0.78 microseconds
    outb(0x80, 0x00);   // write 0x00 to port 0x80, which takes about 1 microsecond
}

static inline void delay_1ms(void) {
    for (int i = 0; i < 100; i++) {
        delay_1us();
    }
}

static inline void set_ivt_entry(uint8_t int_no, uint16_t offset, uint16_t segment) {
    uint16_t base = ((uint16_t)int_no) * 4;
    writeFarWord(0x0000, base + 0, offset);
    writeFarWord(0x0000, base + 2, segment);
}

#endif
