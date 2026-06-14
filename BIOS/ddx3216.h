#ifndef DDX3216_H_
#define DDX3216_H_

// pins on parallel-output of external TLC16C551
#define DDX3216_IRDA_CS			(1 << 1) // AFD#
#define DDX3216_IRDA_EN			(1 << 0) // STB#
#define DDX3216_IRDA_ON			(1 << 2) // INIT#
#define DDX3216_RS232_EN		(1 << 3) // SLIN#
#define DDX3216_SPDI			(1 << 6) // ACK#

void ddx3216_setLEDs();

#endif