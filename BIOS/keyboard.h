#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KBD_DATA_PORT           0x60
#define KBD_CTRL_PORT           0x61
#define KBD_CMD_RESET           0xFE

#define KBD_STATUS_OBF          0x01 // Output Buffer Full
#define KBD_STATUS_IBF          0x02 // Input Buffer Full

#define SC300_XTKBDEN           (1 << 3) // Bit 3: XT Keyboard Enable
#define KBD_CTRL_CLK_LOW        (1 << 6)
#define KBD_CTRL_CLEAR          (1 << 7)

#define KBD_FLAG_RSHIFT         0x01
#define KBD_FLAG_LSHIFT         0x02
#define KBD_FLAG_CTRL           0x04
#define KBD_FLAG_ALT            0x08
#define KBD_FLAG_SCROLLLOCK     0x10
#define KBD_FLAG_NUMLOCK        0x20
#define KBD_FLAG_CAPSLOCK       0x40
#define KBD_FLAG_INSERTMODE     0x80

static const char xt_to_ascii_normal[58] = {
  // index = 0x00
  0x00, // [NULL]
  0x1B, // [ESCAPE]
  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
  0x08, // [BACKSPACE]
  '\t', // [TAB]

  // next index = 0x10
  'q', 'w', 'e', 'r', 't', 'z', 'u', 'i', 'o', 'p', '[', ']',
  0x0D, // [CARRIAGE RETURN]
  0x00, // [CTRL]

  // next index = 0x1E
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
  0x00, // [SHIFT]

  // next index = 0x2B
  '\\', 'y', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0x00, 0x00, 0x00, ' '
};

/*
static const char xt_to_ascii_altgr[58] = {
};
*/

static const char xt_to_ascii_shift[58] = {
  // index = 0x00
  0x00, // [NULL]
  0x1B, // [ESCAPE]
   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
   0x08, // [BACKSPACE]
  '\t',  // [TAB]

  // next index = 0x10
  'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P', '{', '}',
  0x0D,  // [CARRIAGE RETURN]
  0x00,  // [CTRL]

  // next index = 0x1E
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
  0x00,  // [SHIFT]

  // next index = 0x2B
  '|', 'Y', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0x00, 0x00, 0x00, ' '
};

#endif