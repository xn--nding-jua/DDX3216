/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef ISR_H_
#define ISR_H_

#include "bios.h"

#define ISR_FLAGS_CF    0x0001 // carry-flag
#define ISR_FLAGS_PF    0x0004 // parity-flag
#define ISR_FLAGS_ZF    0x0040 // zero-flag
#define ISR_FLAGS_SF    0x0080 // sign-flag

// caution: when changing size of this struct, also update the code in start.s that adjusts the stackpointer (INT_FRAME_WORDS)
struct interrupt_registers {
    uint16_t ds;
    uint16_t es;
    uint16_t bp;
    uint16_t di;
    uint16_t si;
    uint16_t dx;
    uint16_t cx;
    uint16_t bx;
    uint16_t ax;
    
    // Von der CPU automatisch bei "INT" gepusht:
    uint16_t ip;
    uint16_t cs;
    uint16_t flags;
};

void pirq_init();
// hardware-interrupts
__attribute__((externally_visible, regparm(1))) void c_int04_handler(struct interrupt_registers *regs);
__attribute__((externally_visible, regparm(1))) void c_int09_handler(struct interrupt_registers *regs);

// software-interrupts
__attribute__((externally_visible, regparm(1))) void c_int10_handler(struct interrupt_registers *regs);
__attribute__((externally_visible, regparm(1))) void c_int11_handler(struct interrupt_registers *regs);
__attribute__((externally_visible, regparm(1))) void c_int12_handler(struct interrupt_registers *regs);
__attribute__((externally_visible, regparm(1))) void c_int13_handler(struct interrupt_registers *regs);
__attribute__((externally_visible, regparm(1))) void c_int14_handler(struct interrupt_registers *regs);
__attribute__((externally_visible, regparm(1))) void c_int15_handler(struct interrupt_registers *regs);
__attribute__((externally_visible, regparm(1))) void c_int16_handler(struct interrupt_registers *regs);
__attribute__((externally_visible, regparm(1))) void c_int17_handler(struct interrupt_registers *regs);
__attribute__((externally_visible, regparm(1))) void c_int19_handler(struct interrupt_registers *regs);
__attribute__((externally_visible, regparm(1))) void c_int1a_handler(struct interrupt_registers *regs);
__attribute__((externally_visible, regparm(1))) void c_int1c_handler(struct interrupt_registers *regs);
__attribute__((externally_visible, regparm(1))) void c_int_dummy_handler(struct interrupt_registers *regs);

#endif