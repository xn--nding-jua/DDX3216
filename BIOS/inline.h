#ifndef HELPER_H_
#define HELPER_H_

#include "bios.h"

#define CFG_ADDR    0x22
#define CFG_DATA    0x23


// **********************************************************
// General inline-functions
// **********************************************************

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

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

#endif