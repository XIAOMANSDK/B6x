#ifndef LCD_H_
#define LCD_H_

/**
 ****************************************************************************************
 *
 * @file lcd.h
 *
 * @brief LCD Driver, Support two LCDs in Mirror.
 *       [KAC-1.33' IPS_LCD, Pixel:240*240, Driver IC:ST7789, Interface:SPI] or other
 *
 ****************************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// LCD Driver IC
#define IC_NULL                 0x00
#define IC_ST7789               0x01
#define IC_GC9A01               0x02

/// LCD Pixel Size
#define LCD_WIDTH               240
#define LCD_HEIGHT              240

/// LCD control commands
#define CMD_SWRESET             0x01 // Software Reset
#define CMD_SLPIN               0x10 // Sleep In
#define CMD_SLPOUT              0x11 // Sleep Out

#define CMD_INVOFF              0x20 // Inversion Off
#define CMD_INVON               0x21 // Inversion On

#define CMD_DISPOFF             0x28 // Display Off
#define CMD_DISPON              0x29 // Display On
#define CMD_CASET               0x2A // Set Column Addr
#define CMD_RASET               0x2B // Set Row Addr
#define CMD_RAMWR               0x2C // Memory Write

#define CMD_MADCTL              0x36 // Memory Data Access Control
#define CMD_VSCRSADD            0x37 // Vertical Scroll Start Addr

/// LCD memory data access control(MADCTL)
#define MADCTL_MY_BIT           0x80
#define MADCTL_MX_BIT           0x40
#define MADCTL_MV_BIT           0x20 // 0:Portrait, 1:Landscape
#define MADCTL_ML_BIT           0x10 // 0
#define MADCTL_RGB_BIT          0x08 // 0:RGB, 1:BGR
#define MADCTL_MH_BIT           0x04 // 0

// Portrait Display general mode
#define PORTRAIT_NORMAL         (0x00) // (Normal)
#define PORTRAIT_FLIP           (MADCTL_MY_BIT) //0x80 (flip vertically)
#define PORTRAIT_MIORR          (MADCTL_MX_BIT) //0X40 (Left-Right Mirror)
#define PORTRAIT_ROT180         (MADCTL_MY_BIT | MADCTL_MX_BIT) //0xC0 (180 degree rotation)

// Landscape Display general mode
#define LANDSCAPE_NORMAL        (MADCTL_MV_BIT | MADCTL_MX_BIT) //0x60 (Normal)
#define LANDSCAPE_FLIP          (MADCTL_MV_BIT | MADCTL_MY_BIT | MADCTL_MX_BIT) //0xE0 (flip vertically)
#define LANDSCAPE_MIRROR        (MADCTL_MV_BIT) //0X20 (Left-Right Mirror)
#define LANDSCAPE_ROT180        (MADCTL_MV_BIT | MADCTL_MY_BIT) //0XA0 (180 degree rotation)

/// Color RGB565
#define RGB565_BLACK            0x0000
#define RGB565_BLUE             0x001F
#define RGB565_RED              0xF800
#define RGB565_GREEN            0x07E0
#define RGB565_CYAN             0x07FF
#define RGB565_MAGENTA          0xF81F
#define RGB565_YELLOW           0xFFE0
#define RGB565_ORANGE           0xFBE0
#define RGB565_GREY             0x84B5
#define RGB565_BORDEAUX         0xA000
#define RGB565_DINOGREEN        0x2C86
#define RGB565_WHITE            0xFFFF

/// LCDs id
enum lcd_id
{
    ID_LCD1     = 0x01,
    ID_LCD2     = 0x02,

    // All LCDs
    ID_LCDS     = (ID_LCD1 | ID_LCD2),
};

/// LCD state
enum lcd_state
{
    LS_OFF,
    LS_IDLE,
    LS_BUSY,
};


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

void lcd_init(void);
void lcd_reset(void);

bool lcd_select(int id);

uint8_t lcd_get_state(void);
void lcd_wait_done(void);

bool lcd_fill_color(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rgb565);
bool lcd_fill_pixel(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *pixel);
bool lcd_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const void *pic);

#endif // LCD_H_
