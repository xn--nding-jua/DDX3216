#ifndef HELPER_H_
#define HELPER_H_

#include "bios.h"

#define CFG_ADDR    0x22
#define CFG_DATA    0x23


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

static inline void write_sc300_cfg(uint8_t reg, uint8_t data) {
	outb(CFG_ADDR, reg);
	outb(CFG_DATA, data);
}

static inline void write_sc300_cfgw(uint8_t reg, uint16_t data) {
	outb(CFG_ADDR, reg);
	outb(CFG_DATA, (uint8_t)(data & 0xFF));
	
	outb(CFG_ADDR, reg + 1); 
	outb(CFG_DATA, (uint8_t)((data >> 8) & 0xFF));
}

// functions for accessing RAM space in other than the current segment
// ==========================================================
static inline void writeFarByte(uint16_t segment, uint16_t offset, uint8_t value) {
    __asm__ __volatile__(
        "movw %0, %%es\n\t"
        "movb %2, %%es:(%1)"
        :
        : "r"(segment), "b"(offset), "r"(value)
        : "memory"
    );
}

static inline void writeFarWord(uint16_t segment, uint16_t offset, uint16_t value) {
	__asm__ __volatile__(
        "movw %0, %%es\n\t"
        "movw %2, %%es:(%1)"
        :
        : "r"(segment), "b"(offset), "r"(value) // Auch hier "b" für BX verwenden
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


// functions for accessing RAM space using flat-model of "Un"real mode
// ==========================================================
static inline void writeFlatByte(uint32_t linear_address, uint8_t val) {
    __asm__ __volatile__(
        "movb %1, %%fs:(%0)"  // 'movb' steht für Move Byte (8-Bit)
        :
        : "r"(linear_address), "q"(val) // "q" zwingt GCC, ein byte-adressierbares Register (AL, BL, CL, DL) zu wählen
        : "memory"
    );
}

static inline void writeFlatWord(uint32_t linear_address, uint16_t val) {
    // Schreibt ein 16-Bit Word an eine echte, flache 32-Bit-Adresse via FS
    __asm__ __volatile__(
        "movw %1, %%fs:(%0)"
        :
        : "r"(linear_address), "r"(val)
        : "memory"
    );
}

static inline void writeFlatDword(uint32_t linear_address, uint32_t val) {
    __asm__ __volatile__(
        "movl %1, %%fs:(%0)"  // 'movl' steht für Move Long (32-Bit)
        :
        : "r"(linear_address), "r"(val)
        : "memory"
    );
}

static inline uint8_t readFlatByte(uint32_t linear_address) {
    uint8_t val;
    __asm__ __volatile__(
        "movb %%fs:(%1), %0"  // Holt 8 Bit in das Zielregister
        : "=q"(val)           // "q" sorgt dafür, dass das Ziel ein Byte-Register ist
        : "r"(linear_address)
        : "memory"
    );
    return val;
}

static inline uint16_t readFlatWord(uint32_t linear_address) {
    uint16_t val;
    // Liest ein 16-Bit Word von einer echten, flachen 32-Bit-Adresse via FS
    __asm__ __volatile__(
        "movw %%fs:(%1), %0"
        : "=r"(val)
        : "r"(linear_address)
        : "memory"
    );
    return val;
}

static inline uint32_t readFlatDword(uint32_t linear_address) {
    uint32_t val;
    __asm__ __volatile__(
        "movl %%fs:(%1), %0"  // Holt 32 Bit auf einmal in ein Register
        : "=r"(val)
        : "r"(linear_address)
        : "memory"
    );
    return val;
}

#endif