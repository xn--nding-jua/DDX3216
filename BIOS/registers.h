#ifndef REGISTERS_H_
#define REGISTERS_H_

#define CFG_ADDR                0x22
#define CFG_DATA                0x23
#define BASE_SEG                0x0000 // the bottom of the memory-map
#define ROM_SEG                 0xF000 // external ROM is mapped to this segment (/ROMCS)
#define VRAM_SEG                0xB800 // external SRAM is mapped to this segment

// BIOS Data Area (see https://www.lowlevel.eu/wiki/BIOS_Data_Area and https://github.com/sergev/tiltti/blob/main/docs/BIOS_Data_Area.md)
#define BDA_COM1_BASE			0x0400
#define BDA_LPT1_BASE			0x0408
#define BDA_EBDA_BASE           0x040E
#define BDA_EQUIPMENT_WORD		0x0410
#define BDA_MEM_SIZE			0x0413
#define BDA_KBD_STATUS_FLAGS	0x0417
#define BDA_VIDEO_MODE			0x0449
#define BDA_CURSOR_POS_COL      0x0450
#define BDA_CURSOR_POS_ROW      0x0451
#define BDA_VIDEO_IO_BASE       0x0463
#define BDA_SOFT_RESET_FLAGS    0x0472
#define BDA_VIDEO_COLUMS		0x044A
#define BDA_VIDEO_ROWS          0x0484
#define BDA_TIMER_COUNTER_LOW   0x046C
#define BDA_TIMER_COUNTER_HIGH  0x046E
#define BDA_MIDNIGHT_FLAG       0x0470

#define BDA_KBD_HEAD        	0x041A          // next readable scancode
#define BDA_KBD_TAIL        	0x041C          // next writable position for scancode
#define BDA_KBD_BUF_START   	0x001E          // begin of buffer within segment 0x0040
#define BDA_KBD_BUF_END     	0x003E          // end of buffer within segment 0x0040
#define BDA_KBD_BUF_START_PTR   0x0480          // offset in segment 0x0040 to begin of keyboardbuffer (hence 0x001E)
#define BDA_KBD_BUF_END_PTR     0x0482          // offset in segment 0x0040 to end of keyboardbuffer (hence 0x003E)

#define UART_BASE				0x1000
#define UART_CLK				14336000        // external UART is connected to CLK14OUT of SC300
#define UART_THR				(UART_BASE + 0) // Transmit Holding Register
#define UART_RBR				(UART_BASE + 0) // Receive Buffer Register
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

#define LPT_BASE				0x1010
#define LPT_READ_DATA			(LPT_BASE + 0)
#define LPT_READ_STATUS			(LPT_BASE + 1)
#define LPT_READ_CONTROL		(LPT_BASE + 2)
#define LPT_WRITE_DATA			(LPT_BASE + 0)
#define LPT_WRITE_CONTROL		(LPT_BASE + 2)

#define IIR_PENDING             0x01 // 0 = interrupt pending, 1 = no interrupt
#define IIR_REASON              0x0E // Bits 1-3 contain the reason for the interrupt
#define IIR_MS                  0x00 // Modem Status
#define IIR_THRE                0x02 // Transmitter Holding Register Empty (Senden bereit)
#define IIR_RDA                 0x04 // Receiver Data Available (Daten empfangen)
#define IIR_RLS                 0x06 // Receiver Line Status (Fehler/Break)
#define IIR_TIMEOUT             0x0C // Character Timeout (Daten im FIFO, aber keine neuen kommen)

#define LCD_HGA_IDX_ADDR        0x03B4
#define LCD_HGA_IDX_DATA        0x03B5
#define LCD_HGA_MODE_CTRL       0x03B8
#define LCD_HGA_STATUS          0x03BA
#define LCD_HGA_CONFIG          0x03BF

#define LCD_CGA_IDX_ADDR        0x03D4
#define LCD_CGA_IDX_DATA        0x03D5
#define LCD_CGA_MODE_CTRL       0x03D8
#define LCD_CGA_CLR_SEL         0x03D9
#define LCD_CGA_STATUS          0x03DA

#define LCD_VID_IDX_HOR_TOTAL            0x00
#define LCD_VID_IDX_HOR_DISP             0x01
#define LCD_VID_IDX_HOR_SYNC_POS         0x02
#define LCD_VID_IDX_HOR_SYNC_WIDTH       0x03
#define LCD_VID_IDX_VER_TOTAL            0x04
#define LCD_VID_IDX_VER_TOTAL_ADJ        0x05
#define LCD_VID_IDX_VER_DISP             0x06
#define LCD_VID_IDX_VER_SYNC_POS         0x07
#define LCD_VID_IDX_INTERLACED_MODE      0x08
#define LCD_VID_IDX_MAX_SCANLINE         0x09
#define LCD_VID_IDX_CURSOR_START         0x0A
#define LCD_VID_IDX_CURSOR_END           0x0B
#define LCD_VID_IDX_START_ADDR_UPPER     0x0C
#define LCD_VID_IDX_START_ADDR_LOWER     0x0D
#define LCD_VID_IDX_CURSOR_ADDR_UPPER    0x0E
#define LCD_VID_IDX_CURSOR_ADDR_LOWER    0x0F
#define LCD_VID_IDX_LIGHTPEN_REG1        0x10
#define LCD_VID_IDX_LIGHTPEN_REG2        0x11
#define LCD_ENH_IDX_SOFTW_SWITCH_EN      0x12
#define LCD_ENH_IDX_SOFTW_SWITCH_DIS     0x13
#define LCD_ENH_IDX_SCREEN_CTRL_RESTORE  0x18
#define LCD_ENH_IDX_SCREEN_CONTROL_2     0x19
#define LCD_ENH_IDX_SCREEN_ADJ_LOWER     0x1A
#define LCD_ENH_IDX_SCREEN_ADJ_UPPER     0x1B
#define LCD_ENH_IDX_SCREEN_CONTROL_1     0x20
#define LCD_ENH_IDX_TEXT_TRUNC_START     0x21
#define LCD_ENH_IDX_TEXT_TRUNC_STOP      0x22
#define LCD_ENH_IDX_GRAPH_TRUNC_START    0x23
#define LCD_ENH_IDX_GRAPH_TRUNC_STOP     0x24
#define LCD_ENH_IDX_LCD_SPECIAL          0x25

#define RTC_IDX_ADDR            0x70
#define RTC_IDX_DATA            0x71

#define RTC_REG_SECONDS         0x00
#define RTC_REG_MINUTES         0x02
#define RTC_REG_HOURS           0x04
#define RTC_REG_DAY_OF_WEEK     0x06
#define RTC_REG_DAY_OF_MONTH    0x07
#define RTC_REG_MONTH           0x08
#define RTC_REG_YEAR            0x09

// IDE Status Register Bits
#define IDE_STATUS_BSY  		0x80
#define IDE_STATUS_DRDY 		0x40

// Timer 0 Ports (8254 kompatibel)
#define TIMER0_DATA             0x40
#define TIMER1_DATA             0x41
#define TIMER2_DATA             0x42
#define TIMER_CTRL              0x43

#endif