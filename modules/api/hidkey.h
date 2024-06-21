/**
 ****************************************************************************************
 *
 * @file hidkey.h
 *
 * @brief HID KeyCode - USB usages
 *        https://www.usb.org/sites/default/files/hut1_5.pdf
 ****************************************************************************************
 */

#ifndef _HIDKEY_H_
#define _HIDKEY_H_

/// Ordinary keys
enum generic_key 
{
    KEY_A               = 0x04,
    KEY_B               = 0x05,
    KEY_C               = 0x06,
    KEY_D               = 0x07,
    KEY_E               = 0x08,
    KEY_F               = 0x09,
    KEY_G               = 0x0A,
    KEY_H               = 0x0B,
    KEY_I               = 0x0C,
    KEY_J               = 0x0D,
    KEY_K               = 0x0E,
    KEY_L               = 0x0F,
    KEY_M               = 0x10,
    KEY_N               = 0x11,
    KEY_O               = 0x12,
    KEY_P               = 0x13,
    KEY_Q               = 0x14,
    KEY_R               = 0x15,
    KEY_S               = 0x16,
    KEY_T               = 0x17,
    KEY_U               = 0x18,
    KEY_V               = 0x19,
    KEY_W               = 0x1A,
    KEY_X               = 0x1B,
    KEY_Y               = 0x1C,
    KEY_Z               = 0x1D,

    KEY_1               = 0x1E, // 1!
    KEY_2               = 0x1F, // 2@
    KEY_3               = 0x20, // 3#
    KEY_4               = 0x21, // 4$
    KEY_5               = 0x22, // 5%
    KEY_6               = 0x23, // 6^
    KEY_7               = 0x24, // 7&
    KEY_8               = 0x25, // 8*
    KEY_9               = 0x26, // 9(
    KEY_0               = 0x27, // 0)

    KEY_ENTER           = 0x28,
    KEY_ESC             = 0x29,
    KEY_BACKSPACE       = 0x2A,
    KEY_TAB             = 0x2B,
    KEY_SPACEBAR        = 0x2C,
    KEY_MINUS           = 0x2D, // -_
    KEY_EQUAL           = 0x2E, // =+
    KEY_LBRACKET        = 0x2F, // [{
    KEY_RBRACKET        = 0x30, // ]}
    KEY_BACKSLASH       = 0x31, // \|
    KEY_NUMBER          = 0x32,
    KEY_SEMICOLON       = 0x33, // ;: 
    KEY_QUOTE           = 0x34, // '"
    KEY_ACCENT          = 0x35, // `~ 
    KEY_COMMA           = 0x36, // ,< 
    KEY_PERIOD          = 0x37, // .> 
    KEY_SLASH           = 0x38, // /?
    KEY_CAPS_LOCK       = 0x39,

    KEY_F1              = 0x3A,
    KEY_F2              = 0x3B,
    KEY_F3              = 0x3C,
    KEY_F4              = 0x3D,
    KEY_F5              = 0x3E,
    KEY_F6              = 0x3F,
    KEY_F7              = 0x40,
    KEY_F8              = 0x41,
    KEY_F9              = 0x42,
    KEY_F10             = 0x43,
    KEY_F11             = 0x44,
    KEY_F12             = 0x45,

    KEY_PRINT_SCREEN    = 0x46,
    KEY_SCROLL_LOCK     = 0x47,
    KEY_PAUSE           = 0x48,
    KEY_INSERT          = 0x49,
    KEY_HOME            = 0x4A,
    KEY_PAGE_UP         = 0x4B,
    KEY_DELETE          = 0x4C,
    KEY_END             = 0x4D,
    KEY_PAGE_DOWN       = 0x4E,

    KEY_RIGHT           = 0x4F,
    KEY_LEFT            = 0x50,
    KEY_DOWN            = 0x51,
    KEY_UP              = 0x52,

    KEY_NUM_LOCK        = 0x53,
    KEY_PAD_SLASH       = 0x54, // / 
    KEY_PAD_ASTERIX     = 0x55, // * 
    KEY_PAD_MINUS       = 0x56, // - 
    KEY_PAD_PLUS        = 0x57, // + 
    KEY_PAD_ENTER       = 0x58,
    KEY_PAD_1           = 0x59,
    KEY_PAD_2           = 0x5A,
    KEY_PAD_3           = 0x5B,
    KEY_PAD_4           = 0x5C,
    KEY_PAD_5           = 0x5D,
    KEY_PAD_6           = 0x5E,
    KEY_PAD_7           = 0x5F,
    KEY_PAD_8           = 0x60,
    KEY_PAD_9           = 0x61,
    KEY_PAD_0           = 0x62,
    KEY_PAD_DOT         = 0x63, // . 

    KEY_K45             = 0x64,
    KEY_APP             = 0x65,
    KEY_POWER           = 0x66,
    KEY_PAD_EQUAL       = 0x67,
    KEY_F13             = 0x68,
    KEY_F14             = 0x69,
    KEY_F15             = 0x6A,
    KEY_F16             = 0x6B,
    KEY_F17             = 0x6C,
    KEY_F18             = 0x6D,
    KEY_F19             = 0x6E,
    KEY_F20             = 0x6F,
    KEY_F21             = 0x70,
    KEY_F22             = 0x71,
    KEY_F23             = 0x72,
    KEY_F24             = 0x73,
    KEY_EXECUTE         = 0x74,
    KEY_HELP            = 0x75,
    KEY_MENU            = 0x76,
    KEY_SELECT          = 0x77,
    
    KEY_STOP            = 0x78,
    KEY_AGAIN           = 0x79,
    KEY_UNDO            = 0x7A,
    KEY_CUT             = 0x7B,
    KEY_COPY            = 0x7C,
    KEY_PASTE           = 0x7D,
    KEY_FIND            = 0x7E,
    KEY_MUTE            = 0x7F,
    KEY_VOL_UP          = 0x80,
    KEY_VOL_DN          = 0x81,
    
    KEY_LOCKING_CAPS    = 0x82,
    KEY_LOCKING_NUM     = 0x83,
    KEY_LOCKING_SCROLL  = 0x84,
    KEY_PAD_COMMA       = 0x85,
    KEY_PAD_EQUAL_AS400 = 0x86,
    KEY_INTL_1          = 0x87,
    KEY_INTL_2          = 0x88,
    KEY_INTL_3          = 0x89,
    KEY_INTL_4          = 0x8A,
    KEY_INTL_5          = 0x8B,
    KEY_INTL_6          = 0x8C,
    KEY_INTL_7          = 0x8D,
    KEY_INTL_8          = 0x8E,
    KEY_INTL_9          = 0x8F,
    KEY_LANG_1          = 0x90,
    KEY_LANG_2          = 0x91,
    KEY_LANG_3          = 0x92,
    KEY_LANG_4          = 0x93,
    KEY_LANG_5          = 0x94,
    KEY_LANG_6          = 0x95,
    KEY_LANG_7          = 0x96,
    KEY_LANG_8          = 0x97,
    KEY_LANG_9          = 0x98,
    KEY_ALT_ERASE       = 0x99,
    KEY_SYS_REQ         = 0x9A,
    KEY_CANCEL          = 0x9B,
    KEY_CLEAR           = 0x9C,
    KEY_PRIOR           = 0x9D,
    KEY_RETURN          = 0x9E,
    KEY_SEPARATOR       = 0x9F,
    
    KEY_OUT             = 0xA0,
    KEY_OPER            = 0xA1,
    KEY_CLEAR_AGAIN     = 0xA2,
    KEY_CRSEL           = 0xA3,
    KEY_EXSEL           = 0xA4,
    
    // Reserved 0xA5-0xAF(165-175)
    
    KEY_PAD_00                  = 0xB0,
    KEY_PAD_000                 = 0xB1,
    KEY_THOUSANDS_SEPERATOR     = 0xB2,
    KEY_DECIMAL_SEPERATOR       = 0xB3,
    
    KEY_CURRENCY_UNIT           = 0xB4,
    KEY_CURRENCY_SUB_UNIT       = 0xB5,
    KEY_PAD_LEFT_PAREN          = 0xB6,
    KEY_PAD_RIGHT_PAREN         = 0xB7,
    KEY_PAD_LEFT_CURLY_BRACE    = 0xB8,    
    KEY_PAD_RIGHT_CURLY_BRACE   = 0xB9,    
    KEY_PAD_TAB                 = 0xBA,
    KEY_PAD_BACKSPACE           = 0xBB,
    KEY_PAD_A                   = 0xBC,
    KEY_PAD_B                   = 0xBD,
    
    KEY_PAD_C                   = 0xBE,
    KEY_PAD_D                   = 0xBF,
    KEY_PAD_E                   = 0xC0,
    KEY_PAD_F                   = 0xC1,
    KEY_PAD_XOR                 = 0xC2,
    KEY_PAD_CARET               = 0xC3,
    KEY_PAD_PERCENT             = 0xC4,
    KEY_PAD_LESS_THAN           = 0xC5,
    KEY_PAD_GREATER_THAN        = 0xC6,
    KEY_PAD_AMPERSAND           = 0xC7,
    
    KEY_PAD_DOUBLE_AMPERSAND    = 0xC8,
    KEY_PAD_VERTICAL_BAR        = 0xC9,
    KEY_PAD_DOUBLE_VERTICAL_BAR = 0xCA,
    KEY_PAD_COLON               = 0xCB,
    KEY_PAD_HASH                = 0xCC,
    KEY_PAD_SPACE               = 0xCD,
    KEY_PAD_AT                  = 0xCE,
    KEY_PAD_EXCLAMATION         = 0xCF,
    KEY_PAD_MEM_STORE           = 0xD0,
    KEY_PAD_MEM_RECALL          = 0xD1,
    
    KEY_PAD_MEM_CLEAR           = 0xD2,
    KEY_PAD_MEM_ADD             = 0xD3,
    KEY_PAD_MEM_SUBTRACT        = 0xD4,
    KEY_PAD_MEM_MULTIPLY        = 0xD5,
    KEY_PAD_MEM_DIVIDE          = 0xD6,
    KEY_PAD_PLUS_MINUS          = 0xD7,
    KEY_PAD_CLEAR               = 0xD8,
    KEY_PAD_CLEAR_ENTRY         = 0xD9,
    KEY_PAD_BINARY              = 0xDA,
    KEY_PAD_OCTAL               = 0xDB,
    
    KEY_PAD_DECIMAL             = 0xDC,
    KEY_PAD_HEX                 = 0xDD,
    
    // Reserved 0xDE-0xDF(222-223)
    
    KEY_LCTRL                   = 0xE0,
    KEY_LSHIFT                  = 0xE1,
    KEY_LALT                    = 0xE2,
    KEY_LGUI                    = 0xE3,
    KEY_RCTRL                   = 0xE4,
    KEY_RSHIFT                  = 0xE5,
    KEY_RALT                    = 0xE6,
    KEY_RGUI                    = 0xE7,

    KEY_FN                      = 0xF0,
    KEY_LED                     = 0xF1,
    KEY_RGB                     = 0xF2,
};

/// Multi-Fn keys, Reference: <<HID Usage Tables>> Consumer Page(0x0C)
enum multi_key
{
    MKEY_POWER          = 0x0030, // Lock screen
    MKEY_RESET          = 0x0031,
    MKEY_SLEEP          = 0x0032,
    
    MKEY_MENU           = 0x0040,
    MKEY_MENU_PICK      = 0x0041,
    MKEY_MENU_UP        = 0x0042,
    MKEY_MENU_DN        = 0x0043,
    MKEY_MENU_LEFT      = 0x0044,
    MKEY_MENU_RIGHT     = 0x0045,
    
    MKEY_LIGHT_UP       = 0x006F,
    MKEY_LIGHT_DN       = 0x0070,
    
    MKEY_CHN_UP         = 0x009C,
    MKEY_CHN_DN         = 0x009D,
    
    MKEY_PLAY           = 0x00B0,
    MKEY_PAUSE          = 0x00B1,
    MKEY_RECORD         = 0x00B2,
    MKEY_FAST_FORWARD   = 0x00B3,
    MKEY_REWIND         = 0x00B4,
    MKEY_NEXT_TRK       = 0x00B5,
    MKEY_PREV_TRK       = 0x00B6,
    MKEY_STOP           = 0x00B7,
    MKEY_EJECT          = 0x00B8,
    
    MKEY_START_PAUSE    = 0x00CD,
    MKEY_START_SKIP     = 0x00CE,
    
    MKEY_VOLUME         = 0x00E0,
    MKEY_BALANCE        = 0x00E1,
    MKEY_MUTE           = 0x00E2,
    MKEY_VOL_UP         = 0x00E9,
    MKEY_VOL_DN         = 0x00EA,
    
    MKEY_MUSIC          = 0x0183,
    MKEY_EMAIL          = 0x018A,
    MKEY_CALCAULATOL    = 0x0192,
    MKEY_COMPUTER       = 0x0194,
    
    MKEY_VIRKB          = 0x01AE, // Soft keyboard
    
    MKEY_WWW_SEARCH     = 0x0221,
    MKEY_WWW_HOME       = 0x0223,
    MKEY_WWW_BACK       = 0x0224,
    MKEY_WWW_FORWARD    = 0x0225,
    MKEY_WWW_STOP       = 0x0226,
    MKEY_WWW_REFRESH    = 0x0227,
    MKEY_WWW_FAVORITES  = 0x022A,
};

enum multi_key_pos
{
    MKEY_BIT0_POS       = 0,
    MKEY_BIT1_POS       = 1,
    MKEY_BIT2_POS       = 2,
    MKEY_BIT3_POS       = 3,
    MKEY_BIT4_POS       = 4,
    MKEY_BIT5_POS       = 5,
    MKEY_BIT6_POS       = 6,
    MKEY_BIT7_POS       = 7,
};

/// Bitmap of Keys
enum key_bit
{
    // byte0: E0~E7(Modifier Keys)
    KEY_BIT_LCTRL       = 0x01,
    KEY_BIT_LSHIFT      = 0x02,
    KEY_BIT_LALT        = 0x04,
    KEY_BIT_LGUI        = 0x08,

    KEY_BIT_RCTRL       = 0x10,
    KEY_BIT_RSHIFT      = 0x20,
    KEY_BIT_RALT        = 0x40,
    KEY_BIT_RGUI        = 0x80,
    
    // byte1: F0~F2(Fn Keys)
    KEY_BIT_FN          = 0x01,
    KEY_BIT_LED         = 0x02,
    KEY_BIT_RGB         = 0x04,
};

enum key_sys
{
    ANDROID             = 0,
    WINDOWS             = 1,
    SYS_IOS             = 2, //iPad
    SYS_MAC             = 3,

    SYS_MAX             = SYS_MAC,
};

#endif  //_HIDKEY_H_
