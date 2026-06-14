#include "bios.h"

// this is a test-function to fill 8-Bit Shift Register IC73 to control some LEDs
bool g_even;
void ddx3216_setLEDs() {
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
    /*
	for (uint8_t i = 0; i < (8 * 5); i++) {
		// odd LEDs are connected to GND
		// even LEDs are connected to VCC
		// the value 0b00000000 will set the leds of 5 shift-registers to a pattern on/off/on/off/...
		// the other 7 shift-register lanes will be inverted
	    outb(0x3000, 0b00000000); // output data to databus D0...D7 (will set SRCLK=0 and RCLK=1)
    	inb(0x3000); // dummy-read from IO-bus (will set SRCLK=1 and RCLK=0)
	}
    */

    //bool even = false;
    for (uint8_t i = 0; i < (8 * 5); i++) {
        if (g_even) {
            outb(0x3000, 0b11111111);
        }else{
            outb(0x3000, 0b00000000);
        }
        inb(0x3000);

        g_even = !g_even;
    }

/*
    //      bits   76543210      BIT 0          BIT 1           BIT 2
    // DL30..37
    outb(0x3000, 0b00000000); // led 1, Left   led 2, Left     led 3, Left
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, Left   led 10, Left    led 11, Left
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, Right  led 2, Right    led 3, Right
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, Right  led 10, Right   led 11, Right
    inb(0x3000);
    outb(0x3000, 0b00000000); // segment
    inb(0x3000);
    outb(0x3000, 0b00000000); // segment
    inb(0x3000);
    outb(0x3000, 0b00000000); // segment
    inb(0x3000);
    outb(0x3000, 0b00000000); // free
    inb(0x3000);

    // DL20..27
    outb(0x3000, 0b00000000); // led 1, ch 13   led 2, ch 13    led 3, ch 13
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 13   led 10, ch 13   led 11, ch 13
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, ch 14   led 2, ch 14    led 3, ch 14
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 14   led 10, ch 14   led 11, ch 14
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, ch 15   led 2, ch 15    led 3, ch 15
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 15   led 10, ch 15   led 11, ch 15
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, ch 16   led 2, ch 16    led 3, ch 16
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 16   led 10, ch 16   led 11, ch 16
    inb(0x3000);

    // DL10..17
    outb(0x3000, 0b00000000); // led 1, ch 9    led 2, ch 9     led 3, ch 9
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 9    led 10, ch 9    led 11, ch 9
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, ch 10   led 2, ch 10    led 3, ch 10
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 10   led 10, ch 10   led 11, ch 10
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, ch 11   led 2, ch 11    led 3, ch 11
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 11   led 10, ch 11   led 11, ch 11
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, ch 12   led 2, ch 12    led 3, ch 12
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 12   led 10, ch 12   led 11, ch 12
    inb(0x3000);

    // DL00..07
    outb(0x3000, 0b00000000); // led 1, ch 5    led 2, ch 5     led 3, ch 5
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 5    led 10, ch 5    led 11, ch 5
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, ch 6    led 2, ch 6     led 3, ch 6
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 6    led 10, ch 6    led 11, ch 6
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, ch 7    led 2, ch 7     led 3, ch 7
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 7    led 10, ch 7    led 11, ch 7
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, ch 8    led 2, ch 8     led 3, ch 8
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 8    led 10, ch 8    led 11, ch 8
    inb(0x3000);

    // DL0..7
    outb(0x3000, 0b00000000); // led 1, ch 1    led 2, ch 1     led 3, ch 1
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 1    led 10, ch 1    led 11, ch 1
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, ch 2    led 2, ch 2     led 3, ch 2
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 2    led 10, ch 2    led 11, ch 2
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, ch 3    led 2, ch 3     led 3, ch 3
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 3    led 10, ch 3    led 11, ch 3
    inb(0x3000);
    outb(0x3000, 0b00000000); // led 1, ch 4    led 2, ch 4     led 3, ch 4
    inb(0x3000);
    outb(0x3000, 0b11111111); // led 9, ch 4    led 10, ch 4    led 11, ch 4
    inb(0x3000);
    */
}