#define CFG_ADDR                0x22
#define CFG_DATA                0x23
#define ROM_SEG                 0xF000

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

#define IIR_PENDING             0x01 // 0 = Interrupt steht an, 1 = kein Interrupt
#define IIR_REASON              0x0E // Bits 1-3 enthalten den Grund
#define IIR_MS                  0x00 // Modem Status
#define IIR_THRE                0x02 // Transmitter Holding Register Empty (Senden bereit)
#define IIR_RDA                 0x04 // Receiver Data Available (Daten empfangen)
#define IIR_RLS                 0x06 // Receiver Line Status (Fehler/Break)
#define IIR_TIMEOUT             0x0C // Character Timeout (Daten im FIFO, aber keine neuen kommen)

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
#define LCD_VID_IDX_MAX_SCANLINE         0x09
#define LCD_VID_IDX_CURSOR_START         0x0A
#define LCD_VID_IDX_CURSOR_END           0x0B
#define LCD_VID_IDX_START_ADDR_UPPER     0x0C
#define LCD_VID_IDX_START_ADDR_LOWER     0x0D
#define LCD_VID_IDX_CURSOR_ADDR_UPPER    0x0E
#define LCD_VID_IDX_CURSOR_ADDR_LOWER    0x0F

#define LCD_ENH_IDX_SOFTW_SWITCH_EN      0x12
#define LCD_ENH_IDX_SOFTW_SWITCH_DIS     0x13
#define LCD_ENH_IDX_SCREEN_CTRL_REST     0x18
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

#define PCMCIA_MEM_WINDOW		0x3000
#define IDE_BASE				0x01F0

// IDE Status Register Bits
#define IDE_STATUS_BSY  		0x80
#define IDE_STATUS_DRDY 		0x40

// Timer 0 Ports (8254 kompatibel)
#define TIMER0_DATA             0x40
#define TIMER_CTRL              0x43

#define KBD_DATA_PORT           0x60
#define KBD_STATUS_PORT         0x64
#define KBD_STATUS_OBF          0x01 // Output Buffer Full
#define KBD_STATUS_IBF          0x02 // Input Buffer Full
#define KBD_LED_SCROLLLOCK      0x01
#define KBD_LED_NUMLOCK         0x02
#define KBD_LED_CAPSLOCK        0x04

#define VRAM_SEG                0xB800 // directly after boot-sector
#define LCD_COLUMNS_BYTES       30
#define LCD_ROWS                64
#define VRAM_SIZE               (LCD_COLUMNS_BYTES * LCD_ROWS * 2)  // 1920 x 2
