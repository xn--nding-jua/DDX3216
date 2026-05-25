// **********************************************************
// Helper functions
// **********************************************************

#include "helper.h"

void delay_us(uint32_t microseconds) {
    while (microseconds--) {
        delay_1us();
    }
}

char* uint16_to_hex(uint16_t val, char* buf) {
    buf[4] = '\0';
    for (int i = 3; i >= 0; i--) {
        buf[i] = readRomByte((uint16_t)(uintptr_t)&hex_chars[val & 0x0F]);
        val >>= 4;
    }
    return buf;
}

char* uint16_to_dec(uint16_t val, char* buf) {
    buf[5] = '\0';
    for (int i = 4; i >= 0; i--) {
        buf[i] = '0' + (val % 10);
        val /= 10;
    }
    return buf;
}

char* uint8_to_hex(uint8_t val, char* buf) {
    buf[2] = '\0';
    for (int i = 1; i >= 0; i--) {
        buf[i] = readRomByte((uint16_t)(uintptr_t)&hex_chars[val & 0x0F]);
        val >>= 4;
    }
    return buf;
}

char* uint8_to_dec(uint8_t val, char* buf) {
    buf[3] = '\0';
    for (int i = 2; i >= 0; i--) {
        buf[i] = '0' + (val % 10);
        val /= 10;
    }
    return buf;
}
