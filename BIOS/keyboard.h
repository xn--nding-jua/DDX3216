#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KBD_DATA_PORT           0x60
#define KBD_CTRL_PORT           0x61
#define KBD_STATUS_PORT         0x64
#define KBD_CMD_RESET           0xFE

#define KBD_STATUS_OBF          0x01 // Output Buffer Full
#define KBD_STATUS_IBF          0x02 // Input Buffer Full

#define SC300_XTKBDEN           (1 << 3) // Bit 3: XT Keyboard Enable
#define KBD_CTRL_CLK_LOW        (1 << 6)
#define KBD_CTRL_CLEAR          (1 << 7)

#define KBD_FLAG_LSHIFT         0x01
#define KBD_FLAG_RSHIFT         0x02
#define KBD_FLAG_CTRL           0x04
#define KBD_FLAG_ALT            0x08

static const char xt_to_ascii_normal[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  8, // 0x00 - 0x0E
  '\t', 'q', 'w', 'e', 'r', 't', 'z', 'u', 'i', 'o', 'p', '[', ']',  13,    // 0x0F - 0x1C (13 = Enter)
     0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0,    // 0x1D - 0x2A (0x1D = Ctrl)
   '\\', 'y', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,   0,  0,    // 0x2B - 0x38 (0x2A = LShift)
   ' ' // 0x39 = space
};

static const char xt_to_ascii_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  8,
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P', '{', '}',  13,
     0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '^',  0,
   '\'', 'Y', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0,   0,  0,
   ' '
};



#endif