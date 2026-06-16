/*
  Converter to connect a modern AT-keyboard to XT-based systems like the AMD ELAN SC300
  This software reads regular PS/2 codes and converts everything into scancode 1

  Informations
  ==================================================
  More about scancode 1: https://www.marjorie.de/ps2/scancode-set1.htm
  Detailed Schematics: https://github.com/AndersBNielsen/pcxtkbd/blob/master/PS2XTadapter/PS2XTadapter.pdf


  Designated Hardware
  ==================================================
  Arduino Nano or similar (others are possible)

  Used libraries
  ==================================================
  - PS2KeyAdvanced v1.0.9 by Paul Carpenter


  Hardware-Connection:

    Connector to XT-System (view to pins in plug of keyboard):
          **********
       *       2      *        1 = XT_CLK  -> Pin D2
     *                  *      2 = XT_DATA -> Pin D5
    *      5       4     *     3 = not connected
    *                    *     4 = GND
     *    3         1   *      5 = VCC (5V)
       *      ***      *
         ****     ****

    Connector to AT-System (view to pins in plug of keyboard):
    Picture see here: https://www.vogons.org/viewtopic.php?t=103839
          **********
       *       2      *        1 = KBDCLK  -> Pin D3
     *                  *      2 = KBDDATA -> Pin D4
    *      5       4     *     3 = KBDRST (not used / unconnected)
    *                    *     4 = GND
     *    3         1   *      5 = VCC (5V)
       *      ***      *
         ****     ****

    In case you want to connect an USB-keyboard with PS/2-support:
    ***********************************   1 = VCC (5V)
    *                                 *   2 = D- = PS2DATA -> Pin D4
    *                                 *   3 = D+ = PS2CLK  -> Pin D3
    *      1      2     3     4       *   4 = GND
    ***********************************
*/

#include <PS2KeyAdvanced.h>

#define DEBUG     0   // enables debug-output via RS232 at 115200 baud

#define ps_clk    3   // PS CLOCK DATA
#define ps_data   4   // PS DATA PIN
#define xt_clk    2   // XT CLOCK PIN
#define xt_data   5   // XT DATA PIN
#define LED       13  // same as LED_BUILTIN

PS2KeyAdvanced keyboard;

unsigned char translationTable[256];
uint32_t ledCounter;

void setupTable() {
  // we can send 256 scancodes via XT-protocol. Set unused scancodes to 0xFF
  for (int i = 0; i < 256; i++) {
    translationTable[i] = 0xff;  
  }

  // now fill the used keys with designeted scancodes
  // according to this table: https://www.marjorie.de/ps2/scancode-set1.htm

  // =============================================================================================================
  //   KEY  |  MAKE  |  BREAK |   KEY      |  MAKE          |  BREAK         |  KEY      |   MAKE   |  BREAK     |
  // ============================================================================================================|
  //    A   |   1E   |   9E   |   9        |   0A           |   8A           |  [        |  1A      |  9A        |
  //    B   |   30   |   B0   |   `        |   29           |   89           |  INSERT   |  E0,52   |  E0,D2     |
  //    C   |   2E   |   AE   |   -        |   0C           |   8C           |  HOME     |  E0,47   |  E0,97     |
  //    D   |   20   |   A0   |   =        |   0D           |   8D           |  PG UP    |  E0,49   |  E0,C9     |
  //    E   |   12   |   92   |   \        |   2B           |   AB           |  DELETE   |  E0,53   |  E0,D3     |
  //    F   |   21   |   A1   |   BKSP     |   0E           |   8E           |  END      |  E0,4F   |  E0,CF     |
  //    G   |   22   |   A2   |   SPACE    |   39           |   B9           |  PG DN    |  E0,51   |  E0,D1     |
  //    H   |   23   |   A3   |   TAB      |   0F           |   8F           |  U ARROW  |  E0,48   |  E0,C8     |
  //    I   |   17   |   97   |   CAPS     |   3A           |   BA           |  L ARROW  |  E0,4B   |  E0,CB     |
  //    J   |   24   |   A4   |   L SHFT   |   2A           |   AA           |  D ARROW  |  E0,50   |  E0,D0     |
  //    K   |   25   |   A5   |   L CTRL   |   1D           |   9D           |  R ARROW  |  E0,4D   |  E0,CD     |
  //    L   |   26   |   A6   |   L GUI    |   E0,5B        |   E0,DB        |  NUM      |  45      |  C5        |
  //    M   |   32   |   B2   |   L ALT    |   38           |   B8           |  KP /     |  E0,35   |  E0,B5     |
  //    N   |   31   |   B1   |   R SHFT   |   36           |   B6           |  KP *     |  37      |  B7        |
  //    O   |   18   |   98   |   R CTRL   |   E0,1D        |   E0,9D        |  KP -     |  4A      |  CA        |
  //    P   |   19   |   99   |   R GUI    |   E0,5C        |   E0,DC        |  KP +     |  4E      |  CE        |
  //    Q   |   10   |   90   |   R ALT    |   E0,38        |   E0,B8        |  KP EN    |  E0,1C   |  E0,9C     |
  //    R   |   13   |   93   |   APPS     |   E0,5D        |   E0,DD        |  KP .     |  53      |  D3        |
  //    S   |   1F   |   9F   |   ENTER    |   1C           |   9C           |  KP 0     |  52      |  D2        |
  //    T   |   14   |   94   |   ESC      |   01           |   81           |  KP 1     |  4F      |  CF        |
  //    U   |   16   |   96   |   F1       |   3B           |   BB           |  KP 2     |  50      |  D0        |
  //    V   |   2F   |   AF   |   F2       |   3C           |   BC           |  KP 3     |  51      |  D1        |
  //    W   |   11   |   91   |   F3       |   3D           |   BD           |  KP 4     |  4B      |  CB        |
  //    X   |   2D   |   AD   |   F4       |   3E           |   BE           |  KP 5     |  4C      |  CC        |
  //    Y   |   15   |   95   |   F5       |   3F           |   BF           |  KP 6     |  4D      |  CD        |
  //    Z   |   2C   |   AC   |   F6       |   40           |   C0           |  KP 7     |  47      |  C7        |
  //    0   |   0B   |   8B   |   F7       |   41           |   C1           |  KP 8     |  48      |  C8        |
  //    1   |   02   |   82   |   F8       |   42           |   C2           |  KP 9     |  49      |  C9        |
  //    2   |   03   |   83   |   F9       |   43           |   C3           |  ]        |  1B      |  9B        |
  //    3   |   04   |   84   |   F10      |   44           |   C4           |  ;        |  27      |  A7        |
  //    4   |   05   |   85   |   F11      |   57           |   D7           |  '        |  28      |  A8        |
  //    5   |   06   |   86   |   F12      |   58           |   D8           |  ,        |  33      |  B3        |
  //    6   |   07   |   87   |   PRNT     |   E0,2A,E0,37  |   E0,B7,E0,AA  |  .        |  34      |  B4        |
  //    7   |   08   |   88   |   SCROLL   |   46           |   C6           |  /        |  35      |  B5        |
  //    8   |   09   |   89   |   PAUSE    |   E1,1D,45     |   NONE         |           |          |            |
  // ============================================================================================================|
    
  // page 1
  translationTable[PS2_KEY_A]         = 0x1E;
  translationTable[PS2_KEY_B]         = 0x30;
  translationTable[PS2_KEY_C]         = 0x2E;
  translationTable[PS2_KEY_D]         = 0x20;
  translationTable[PS2_KEY_E]         = 0x12;
  translationTable[PS2_KEY_F]         = 0x21;
  translationTable[PS2_KEY_G]         = 0x22;
  translationTable[PS2_KEY_H]         = 0x23;
  translationTable[PS2_KEY_I]         = 0x17;
  translationTable[PS2_KEY_J]         = 0x24;
  translationTable[PS2_KEY_K]         = 0x25;
  translationTable[PS2_KEY_L]         = 0x26;
  translationTable[PS2_KEY_M]         = 0x32;
  translationTable[PS2_KEY_N]         = 0x31;
  translationTable[PS2_KEY_O]         = 0x18;
  translationTable[PS2_KEY_P]         = 0x19;
  translationTable[PS2_KEY_Q]         = 0x10;
  translationTable[PS2_KEY_R]         = 0x13;
  translationTable[PS2_KEY_S]         = 0x1F;
  translationTable[PS2_KEY_T]         = 0x14;
  translationTable[PS2_KEY_U]         = 0x16;
  translationTable[PS2_KEY_V]         = 0x2F;
  translationTable[PS2_KEY_W]         = 0x11;
  translationTable[PS2_KEY_X]         = 0x2D;
  translationTable[PS2_KEY_Y]         = 0x15;
  translationTable[PS2_KEY_Z]         = 0x2c;
  translationTable[PS2_KEY_0]         = 0x0B;
  translationTable[PS2_KEY_1]         = 0x02;
  translationTable[PS2_KEY_2]         = 0x03;
  translationTable[PS2_KEY_3]         = 0x04;
  translationTable[PS2_KEY_4]         = 0x05;
  translationTable[PS2_KEY_5]         = 0x06;
  translationTable[PS2_KEY_6]         = 0x07;
  translationTable[PS2_KEY_7]         = 0x08;
  translationTable[PS2_KEY_8]         = 0x09;

  // page 2
  translationTable[PS2_KEY_9]         = 0x0A;
  translationTable[PS2_KEY_SINGLE]    = 0x29; // high-comma
  translationTable[PS2_KEY_MINUS]     = 0x0C;
  translationTable[PS2_KEY_EQUAL]     = 0x0D;  
  translationTable[PS2_KEY_BACK]      = 0x2B; // backslash
  translationTable[PS2_KEY_BS]        = 0x0E; // backspace
  translationTable[PS2_KEY_SPACE]     = 0x39; // spacebar
  translationTable[PS2_KEY_TAB]       = 0x0F;
  translationTable[PS2_KEY_CAPS]      = 0x3A;
  translationTable[PS2_KEY_L_SHIFT]   = 0x2A;
  translationTable[PS2_KEY_L_CTRL]    = 0x1D;
  translationTable[PS2_KEY_L_GUI]     = 0x5C; // windows-key
  translationTable[PS2_KEY_L_ALT]     = 0x38; 
  translationTable[PS2_KEY_R_SHIFT]   = 0x36;
  translationTable[PS2_KEY_R_CTRL]    = 0x1D; // original 0xE0 -> 0x1D
  translationTable[PS2_KEY_R_GUI]     = 0x5B; // windows-key
  translationTable[PS2_KEY_R_ALT]     = 0x38; // original 0xE0 -> 0x38
  translationTable[PS2_KEY_MENU]      = 0x5D; // APPS-Key
  translationTable[PS2_KEY_ENTER]     = 0x1C;
  translationTable[PS2_KEY_ESC]       = 0x01;
  translationTable[PS2_KEY_F1]        = 0x3B;
  translationTable[PS2_KEY_F2]        = 0x3C;
  translationTable[PS2_KEY_F3]        = 0x3D;
  translationTable[PS2_KEY_F4]        = 0x3E;
  translationTable[PS2_KEY_F5]        = 0x3F;
  translationTable[PS2_KEY_F6]        = 0x40;
  translationTable[PS2_KEY_F7]        = 0x41;
  translationTable[PS2_KEY_F8]        = 0x42;
  translationTable[PS2_KEY_F9]        = 0x43;
  translationTable[PS2_KEY_F10]       = 0x44;
  translationTable[PS2_KEY_F11]       = 0x57;
  translationTable[PS2_KEY_F12]       = 0x58;
  translationTable[PS2_KEY_SCROLL]    = 0x46;

  // page 3
  translationTable[PS2_KEY_OPEN_SQ]   = 0x1A;
  translationTable[PS2_KEY_INSERT]    = 0x52;
  translationTable[PS2_KEY_HOME]      = 0x47;
  translationTable[PS2_KEY_PGUP]      = 0x49;
  translationTable[PS2_KEY_DELETE]    = 0x53;
  translationTable[PS2_KEY_END]       = 0x4F;
  translationTable[PS2_KEY_PGDN]      = 0x51;
  translationTable[PS2_KEY_UP_ARROW]  = 0x48;
  translationTable[PS2_KEY_L_ARROW]   = 0x4B;
  translationTable[PS2_KEY_DN_ARROW]  = 0x50;
  translationTable[PS2_KEY_R_ARROW]   = 0x4D;
  translationTable[PS2_KEY_NUM]       = 0x45;
  translationTable[PS2_KEY_KP_DIV]    = 0x35;
  translationTable[PS2_KEY_KP_TIMES]  = 0x37;
  translationTable[PS2_KEY_KP_MINUS]  = 0x4A;
  translationTable[PS2_KEY_KP_PLUS]   = 0x4E;
  translationTable[PS2_KEY_KP_ENTER]  = 0x1C;
  translationTable[PS2_KEY_KP_DOT]    = 0x33;
  translationTable[PS2_KEY_KP0]       = 0x0B;
  translationTable[PS2_KEY_KP1]       = 0x02;
  translationTable[PS2_KEY_KP2]       = 0x03;
  translationTable[PS2_KEY_KP3]       = 0x04;
  translationTable[PS2_KEY_KP4]       = 0x05;
  translationTable[PS2_KEY_KP5]       = 0x06;
  translationTable[PS2_KEY_KP6]       = 0x07;
  translationTable[PS2_KEY_KP7]       = 0x08;
  translationTable[PS2_KEY_KP8]       = 0x09;
  translationTable[PS2_KEY_KP9]       = 0x0A;
  translationTable[PS2_KEY_CLOSE_SQ]  = 0x1B;
  translationTable[PS2_KEY_SEMI]      = 0x27;
  translationTable[PS2_KEY_APOS]      = 0x28; 
  translationTable[PS2_KEY_COMMA]     = 0x33;
  translationTable[PS2_KEY_DOT]       = 0x34;
  translationTable[PS2_KEY_DIV]       = 0x35;

  // own scancodes
  // we are using scancodes 0x01 to 0x5D above
  // scancodes 0x5E to 0x7F are still available
  translationTable[PS2_KEY_PRTSCR]    = 0x5E;
  translationTable[PS2_KEY_PAUSE]     = 0x5F;
  translationTable[PS2_KEY_BREAK]     = 0x60; // break is generated by keyboard when Ctrl + Pause is pressed

  // scancodes 0x61 to 0x7F are still available, so we add the media-keys here
  translationTable[PS2_KEY_NEXT_TR]   = 0x61;
  translationTable[PS2_KEY_PREV_TR]   = 0x62;
  translationTable[PS2_KEY_STOP]      = 0x63;
  translationTable[PS2_KEY_PLAY]      = 0x64;
  translationTable[PS2_KEY_MUTE]      = 0x65;
  translationTable[PS2_KEY_VOL_UP]    = 0x66;
  translationTable[PS2_KEY_VOL_DN]    = 0x67;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // we are receiving new data from keyboard -> turn on LED
  ledCounter = 100000; // preload the ledCounter to 10000 gives a reasonable turnOn-length of the LED

  setupTable() ; 

  keyboard.begin(ps_data, ps_clk);
  #if DEBUG == 1
    Serial.begin(115200);
  #endif  

  // enable numlock on bootup
  keyboard.setLock(PS2_LOCK_NUM);

  pinMode(xt_clk, INPUT_PULLUP) ; 
  digitalWrite(xt_data, LOW); // turns out the inactive state of the KB DATA line should always be LOW, as HIGH will enter "Manufacturing mode" on an IBM XT, continuously toggling the CLK line at 9.1Hz.
  pinMode(xt_data, OUTPUT);
}

void sendXtKey(unsigned char value) {
  // check if this is a supported scancode. if not -> return
  if (value == 0xFF) {
    return;
  }

  #if DEBUG == 1
    Serial.println((uint8_t)value, HEX);
  #endif

  while (digitalRead(xt_clk) != HIGH) {};  // Wait here until host releases CLK

  digitalWrite(xt_clk, LOW) ; // pull clock low before switching to output
  pinMode(xt_clk, OUTPUT); // RTS

  pinMode(xt_data, INPUT_PULLUP); // Let data go high
  delayMicroseconds(120);

  // while (digitalRead(xt_data) != HIGH); //We could wait here, we could cancel the data, we could do many things if we don't get a CTS but.. nah.
  pinMode(xt_clk, INPUT_PULLUP); // let clk go high for start bit
  delayMicroseconds(66);

  digitalWrite(xt_clk, LOW);
  pinMode(xt_clk, OUTPUT);
  delayMicroseconds(30);

  // send all 8 databits
  byte i = 0;
  for (i=0; i < 8; i++) {
    if (value & 1) {
      pinMode(xt_data, INPUT_PULLUP); // Let data go high
    }else{
      digitalWrite(xt_data, LOW);
      pinMode(xt_data, OUTPUT);
    }
    value = value >> 1;

    // Spec says to have data available 2.5us before and after clock pulse.
    pinMode(xt_clk, INPUT_PULLUP); // Let clk go high 
    delayMicroseconds(95);
    digitalWrite(xt_clk, LOW); 
    pinMode(xt_clk, OUTPUT);
  } 
  delayMicroseconds(20);

  // Go to passive clk high, data low
  pinMode(xt_clk, INPUT_PULLUP); // Let clk go high 
  digitalWrite(xt_data, LOW);  // Keep data inactive low
  pinMode(xt_data, OUTPUT);
  delay(1) ;
}

void loop() {
  if (keyboard.available()) {
    digitalWrite(LED_BUILTIN, HIGH); // we are receiving new data from keyboard -> turn on LED
    ledCounter = 1000; // preload the ledCounter to 1000 gives a reasonable turnOn-length of the LED

    uint16_t c = keyboard.read();
    #if DEBUG == 1
      if (c > 0) {
        Serial.print("Code: 0x");
        Serial.print(c & 0xFF, HEX);
        Serial.print(" | Status: 0x");
        Serial.print(c >> 8, HEX);
        Serial.print(" -> XT Code 0x");
      }
    #endif

    // check for make- or break-code
    if( !(c & PS2_BREAK) ) {
      // make-code (key pressed)
      sendXtKey(translationTable[c & 0xff]);
    }else{
      // break-code (key is released)
      sendXtKey(translationTable[c & 0xff] | 0x80);
    }
  }

  // power-on self test
  if (digitalRead(xt_clk) == LOW) {
    delay(10); // wait 10ms
    sendXtKey(0xAA) ; 
    while (keyboard.available()) {
      keyboard.read(); // Discard buffer on reset
    }
  }

  // check if LED is on -> decrement ledCounter
  if (ledCounter > 0) {
    ledCounter--;

    if (ledCounter == 1) {
      // turn LED off
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}
