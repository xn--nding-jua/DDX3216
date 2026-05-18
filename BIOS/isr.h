/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef ISR_H_
#define ISR_H_

#include "bios.h"

struct interrupt_frame {
    uint16_t ip;
    uint16_t cs;
    uint16_t flags;
};

void pirq_init();
__attribute__((interrupt)) void c_int08_handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void c_int09_handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void c_int10_handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void c_int11_handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void c_int12_handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void c_int13_handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void c_int14_handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void c_int15_handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void c_int16_handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void c_int0c_handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void c_int1a_handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void c_int1c_handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void c_int_dummy_handler(struct interrupt_frame *frame);

#endif