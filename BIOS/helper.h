#ifndef HELPER_H_
#define HELPER_H_

#include "bios.h"

void delay_us(uint32_t microseconds);
char* uint16_to_hex(uint16_t val, char* buf);
char* uint16_to_dec(uint16_t val, char* buf);

#endif