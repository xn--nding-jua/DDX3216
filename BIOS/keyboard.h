#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KBD_DATA_PORT           0x60
#define KBD_CTRL_PORT           0x61
#define KBD_STATUS_PORT         0x64

#define KBD_STATUS_OBF          0x01 // Output Buffer Full
#define KBD_STATUS_IBF          0x02 // Input Buffer Full
#define KBD_LED_SCROLLLOCK      0x01
#define KBD_LED_NUMLOCK         0x02
#define KBD_LED_CAPSLOCK        0x04

// BDA 0x0417: Keyboard Status Flags 1
#define KBD_FLAG_RSHIFT         0x01
#define KBD_FLAG_LSHIFT         0x02
#define KBD_FLAG_CTRL           0x04
#define KBD_FLAG_ALT            0x08
#define KBD_FLAG_SCROLLLOCK     0x10
#define KBD_FLAG_NUMLOCK        0x20
#define KBD_FLAG_CAPSLOCK       0x40
#define KBD_FLAG_INSERT         0x80

#define SC300_XTKBDEN           (1 << 3) // Bit 3: XT Keyboard Enable
#define KBD_CTRL_CLK_LOW        (1 << 6)
#define KBD_CTRL_CLEAR          (1 << 7)

#define KBD_CMD_RESET           0xFE

// set 2 Scancodes for special keys
#define SC2_EXTENDED    0xE0    // Extended Key Prefix
#define SC2_BREAK       0xF0    // Break Code Prefix
#define SC2_PAUSE_1     0xE1    // Pause Key (2-Byte Sequenz)

// AT Scancode Set 2 -> XT Scancode Set 1 Conversion table
// Index = Set-2 Scancode, Value = Set-1 Scancode
static const uint8_t set2_to_set1[256] = {
//  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x00, 0x43, 0x41, 0x3F, 0x3D, 0x3B, 0x3C, 0x58, 0x64, 0x44, 0x42, 0x40, 0x3E, 0x0F, 0x29, 0x59, // 0x00
    0x65, 0x38, 0x2A, 0x70, 0x1D, 0x10, 0x02, 0x5A, 0x66, 0x71, 0x2C, 0x1F, 0x1E, 0x11, 0x03, 0x5B, // 0x10
    0x67, 0x2E, 0x2D, 0x20, 0x12, 0x05, 0x04, 0x5C, 0x68, 0x39, 0x2F, 0x21, 0x14, 0x13, 0x06, 0x5D, // 0x20
    0x69, 0x31, 0x30, 0x23, 0x22, 0x15, 0x07, 0x5E, 0x6A, 0x72, 0x32, 0x24, 0x16, 0x08, 0x09, 0x5F, // 0x30
    0x6B, 0x33, 0x25, 0x17, 0x18, 0x0B, 0x0A, 0x60, 0x6C, 0x34, 0x35, 0x26, 0x27, 0x19, 0x0C, 0x61, // 0x40
    0x6D, 0x73, 0x28, 0x74, 0x1A, 0x0D, 0x62, 0x6E, 0x3A, 0x36, 0x1C, 0x1B, 0x75, 0x2B, 0x63, 0x76, // 0x50
    0x55, 0x56, 0x77, 0x78, 0x79, 0x7A, 0x0E, 0x7B, 0x7C, 0x4F, 0x7D, 0x4B, 0x47, 0x7E, 0x7F, 0x6F, // 0x60
    0x52, 0x53, 0x50, 0x4C, 0x4D, 0x48, 0x01, 0x45, 0x57, 0x4E, 0x51, 0x4A, 0x37, 0x49, 0x46, 0x54, // 0x70
    0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x80
};

typedef enum {
    KBD_STATE_IDLE,         // Regular State
    KBD_STATE_EXTENDED,     // after 0xE0
    KBD_STATE_BREAK,        // after 0xF0
    KBD_STATE_EXT_BREAK,    // after 0xE0 0xF0
} kbd_state_t;
static volatile kbd_state_t kbd_state = KBD_STATE_IDLE;

// Shift-Status
static bool shift_pressed  = false;
static bool caps_lock      = false;
static bool ctrl_pressed   = false;
static bool alt_pressed    = false;

// Set 2 Extended Scancodes -> ASCII
// (after 0xE0 Prefix)
static const uint8_t set2_ext_to_set1[256] = {
    [0x11] = 0x38,  // Right Alt    -> Alt
    [0x14] = 0x1D,  // Right Ctrl   -> Ctrl
    [0x4A] = 0x35,  // Numpad /
    [0x5A] = 0x1C,  // Numpad Enter
    [0x69] = 0x4F,  // End
    [0x6B] = 0x4B,  // Cursor Left
    [0x6C] = 0x47,  // Home
    [0x70] = 0x52,  // Insert
    [0x71] = 0x53,  // Delete
    [0x72] = 0x50,  // Cursor Down
    [0x74] = 0x4D,  // Cursor Right
    [0x75] = 0x48,  // Cursor Up
    [0x7A] = 0x51,  // Page Down
    [0x7D] = 0x49,  // Page Up
};

// ============================================================
// Set 1 -> ASCII mit Shift-Unterstützung
// ============================================================
static const char set1_ascii_normal[] = {
//  0    1    2    3    4    5    6    7
    0,   0,  '1', '2', '3', '4', '5', '6',   // 0x00
   '7', '8', '9', '0', '-', '=','\b','\t',   // 0x08
   'q', 'w', 'e', 'r', 't', 'z', 'u', 'i',   // 0x10  (QWERTZ!)
   'o', 'p', '[', ']','\n',  0,  'a', 's',   // 0x18
   'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',   // 0x20
   '\'','`',  0, '#', 'y', 'x', 'c', 'v',    // 0x28  (QWERTZ: y <-> z)
   'b', 'n', 'm', ',', '.', '-',  0,  '*',   // 0x30
    0,  ' ',                                 // 0x38
};

static const char set1_ascii_shift[] = {
    0,    0,   '!', '"', '#', '$', '%', '&', // 0x00
   '/', '(', ')', '=', '?', '`','\b','\t',   // 0x08
   'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I',   // 0x10
   'O', 'P', '{', '}','\n',  0,  'A', 'S',   // 0x18
   'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',   // 0x20
   '@', '~',  0,  '\'','Y', 'X', 'C', 'V',   // 0x28
   'B', 'N', 'M', '<', '>', '_',  0,  '*',   // 0x30
    0,  ' ',                                 // 0x38
};

static char set1_to_ascii(uint8_t set1_code, bool shift, bool caps) {
    bool use_shift = shift;

    // caps-lock invertes shift for letters
    if (caps && set1_code >= 0x10 && set1_code <= 0x32) {
        use_shift = !shift;
    }

    if (set1_code < sizeof(set1_ascii_normal)) {
        if (use_shift) {
            //return set1_ascii_shift[set1_code];
            return (char)readRomByte((uint16_t)(uintptr_t)&set1_ascii_shift[set1_code]);
        } else {
            //return set1_ascii_normal[set1_code];
            return (char)readRomByte((uint16_t)(uintptr_t)&set1_ascii_normal[set1_code]);
        }
    }
    return 0;
}

#endif