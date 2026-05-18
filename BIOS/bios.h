/*
	BIOS for the AMD SC300 within the Behringer DDX3216
	(c) 2026 Chris Noeding, christian@noeding-online.de
	https://chrisdevblog.com
*/

#ifndef BIOS_H_
#define BIOS_H_

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include "inline.h"
#include "uart.h"
#include "lcd.h"
#include "disk.h"
#include "timer.h"
#include "isr.h"
#include "helper.h"

#define CFG_ADDR    0x22
#define CFG_DATA    0x23
#define ROM_SEG     0xF000

// BIOS Data Area (see https://www.lowlevel.eu/wiki/BIOS_Data_Area)
#define BDA_COM1_BASE			0x0400
#define BDA_EQUIPMENT_WORD		0x0410
#define BDA_MEM_SIZE			0x0413
#define BDA_VIDEO_INFO			0x0449
#define BDA_VIDEO_COLUMS		0x044A
#define BDA_TIMER_COUNTER 		((volatile uint32_t*)0x046C)

#define BDA_KBD_HEAD        	((volatile uint16_t*)0x041A) // Kopf (nächstes zu lesendes Zeichen)
#define BDA_KBD_TAIL        	((volatile uint16_t*)0x041C) // Ende (nächstes zu schreibendes Zeichen)
#define BDA_KBD_BUF_START   	0x041E                       // Offset des Puffers in Segment 0x40
#define BDA_KBD_BUF_END     	0x043E                       // Ende des Puffers

#define UART_BASE				0x1000
#define UART_CLK				14336000        // external UART is connected to CLK14OUT of SC300
#define UART_THR				(UART_BASE + 0) // Transmit Holding Register
#define UART_DLL				(UART_BASE + 0) // Divisor Latch Low
#define UART_DLM				(UART_BASE + 1) // Divisor Latch High
#define UART_IER				(UART_BASE + 1) // Interrupt Enable Register
#define UART_FCR				(UART_BASE + 2) // FIFO Control Register
#define UART_IIR				(UART_BASE + 2) // Interrupt
#define UART_LCR				(UART_BASE + 3) // Line Control Register
#define UART_MCR				(UART_BASE + 4) // Modem Control Register
#define UART_LSR				(UART_BASE + 5) // Line Status Register
#define UART_MSR				(UART_BASE + 6) // Modem Status Register
#define UART_SCR				(UART_BASE + 7) // Scratchpad Register

#define MIDI_BASE				0x1008
#define MIDI_CLK				14336000        // external UART is connected to CLK14OUT of SC300
#define MIDI_THR				(MIDI_BASE + 0) // Transmit Holding Register
#define MIDI_DLL				(MIDI_BASE + 0) // Divisor Latch Low
#define MIDI_DLM				(MIDI_BASE + 1) // Divisor Latch High
#define MIDI_IER				(MIDI_BASE + 1) // Interrupt Enable Register
#define MIDI_FCR				(MIDI_BASE + 2) // FIFO Control Register
#define MIDI_IIR				(MIDI_BASE + 2) // Interrupt
#define MIDI_LCR				(MIDI_BASE + 3) // Line Control Register
#define MIDI_MCR				(MIDI_BASE + 4) // Modem Control Register
#define MIDI_LSR				(MIDI_BASE + 5) // Line Status Register
#define MIDI_MSR				(MIDI_BASE + 6) // Modem Status Register
#define MIDI_SCR				(MIDI_BASE + 7) // Scratchpad Register

#define IIR_PENDING 0x01 // 0 = Interrupt steht an, 1 = kein Interrupt
#define IIR_REASON  0x0E // Bits 1-3 enthalten den Grund
#define IIR_MS      0x00 // Modem Status
#define IIR_THRE    0x02 // Transmitter Holding Register Empty (Senden bereit)
#define IIR_RDA     0x04 // Receiver Data Available (Daten empfangen)
#define IIR_RLS     0x06 // Receiver Line Status (Fehler/Break)
#define IIR_TIMEOUT 0x0C // Character Timeout (Daten im FIFO, aber keine neuen kommen)

#define PCMCIA_MEM_WINDOW		0x3000
#define IDE_BASE				0x01F0

// IDE Status Register Bits
#define IDE_STATUS_BSY  		0x80
#define IDE_STATUS_DRDY 		0x40

// Timer 0 Ports (8254 kompatibel)
#define TIMER0_DATA    0x40
#define TIMER_CTRL     0x43

#define KBD_DATA_PORT    0x60
#define KBD_STATUS_PORT  0x64
#define KBD_STAT_OBF     0x01 // Output Buffer Full

#define VRAM_BASE           0x7E00 // directly after boot-sector
#define LCD_COLUMNS_BYTES   30
#define LCD_ROWS            64
#define VRAM_SIZE           (LCD_COLUMNS_BYTES * LCD_ROWS)  // 1920

extern void activate_unreal_mode(void);

#endif