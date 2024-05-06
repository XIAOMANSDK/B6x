/**
 ****************************************************************************************
 *
 * @file gpio.h
 *
 * @brief Header file - GPIO Driver
 *
 ****************************************************************************************
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdint.h>
#include "iopad.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#if !defined(BIT)
    #define BIT(n)             (1UL << (n))
#endif

/// Index of gpio bits 
enum gpio_bits
{
    GPIO00                   = (1UL << PA00),
    GPIO01                   = (1UL << PA01),
    GPIO02                   = (1UL << PA02),
    GPIO03                   = (1UL << PA03),
    GPIO04                   = (1UL << PA04),
    GPIO05                   = (1UL << PA05),
    GPIO06                   = (1UL << PA06),
    GPIO07                   = (1UL << PA07),
    GPIO08                   = (1UL << PA08),
    GPIO09                   = (1UL << PA09),
    GPIO10                   = (1UL << PA10),
    GPIO11                   = (1UL << PA11),
    GPIO12                   = (1UL << PA12),
    GPIO13                   = (1UL << PA13),
    GPIO14                   = (1UL << PA14),
    GPIO15                   = (1UL << PA15),
    GPIO16                   = (1UL << PA16),
    GPIO17                   = (1UL << PA17),
    GPIO18                   = (1UL << PA18),
    GPIO19                   = (1UL << PA19),
};

/// Type of gpio output
enum gpio_out_typ
{
    OE_LOW                   = 0,
    OE_HIGH                  = 1,
};

/// Type of gpio input
enum gpio_in_typ
{
    IE_AIR                   = IOM_INPUT,
    IE_DOWN                  = IOM_INPUT | IOM_PULLDOWN,
    IE_UP                    = IOM_INPUT | IOM_PULLUP,
};


/*
 * MACROS DECLARATION
 ****************************************************************************************
 */

#include "reg_gpio.h"

/// Set gpio data to 1 (output High) - only write 1 effect
#define GPIO_DAT_SET(bits)     ( GPIO->DAT_SET = (uint32_t)(bits) )
/// Clear gpio data to 0 (output Low) - only write 1 effect
#define GPIO_DAT_CLR(bits)     ( GPIO->DAT_CLR = (uint32_t)(bits) )
/// Toggle gpio data (state overturn) - only write 1 effect
#define GPIO_DAT_TOG(bits)     ( GPIO->DAT_TOG = (uint32_t)(bits) )

/// Mask gpio data operation - write 1: can't set/clear/toggle, 0: normal
#define GPIO_DAT_MSK(bits)     ( GPIO->DAT_MSK = (uint32_t)(bits) )
/// Get gpio data mask - Read 1: mask, 0: not mask
#define GPIO_DAT_MSK_GET()     ( GPIO->DAT_MSK )

/// Set gpio data all config - write 1: High, 0: Low
#define GPIO_DAT_CFG(bits)     ( GPIO->DAT = (uint32_t)(bits) )
/// Get gpio data all config - Read 1: High, 0: Low
#define GPIO_DAT_GET()         ( GPIO->DAT )

/// Set gpio dir to 1 (enable output) - only write 1 effect
#define GPIO_DIR_SET(bits)     ( GPIO->DIR_SET = (uint32_t)(bits) )
/// Clear gpio dir to 0 (disable output) - only write 1 effect
#define GPIO_DIR_CLR(bits)     ( GPIO->DIR_CLR = (uint32_t)(bits) )
/// Toggle gpio dir (state overturn) - only write 1 effect
#define GPIO_DIR_TOG(bits)     ( GPIO->DIR_TOG = (uint32_t)(bits) )

/// Mask gpio dir operation - write 1: can't set/clear/toggle, 0: normal
#define GPIO_DIR_MSK(bits)     ( GPIO->DIR_MSK = (uint32_t)(bits) )
/// Get gpio dir mask - Read 1: mask, 0: not mask
#define GPIO_DIR_MSK_GET()     ( GPIO->DIR_MSK )

/// Set gpio dir all config - write 1: Enable, 0: Disable
#define GPIO_DIR_CFG(bits)     ( GPIO->DIR = (uint32_t)(bits) )
/// Get gpio dir all config - Read 1: Enable, 0: Disable
#define GPIO_DIR_GET()         ( GPIO->DIR )

/// Get gpio input state (1: High, 0: Low), enable via iom_ctrl()
#define GPIO_PIN_GET()         ( GPIO->PIN )

/// gpio output enable, low level
#define GPIO_DIR_SET_LO(bits)  dowl( GPIO_DAT_CLR(bits); GPIO_DIR_SET(bits); )
/// gpio output enable, high level
#define GPIO_DIR_SET_HI(bits)  dowl( GPIO_DAT_SET(bits); GPIO_DIR_SET(bits); )


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

static inline void gpio_set_hiz(uint8_t pad)
{
    // disable output & input
    GPIO_DIR_CLR(1UL << pad);
    iom_ctrl(pad, IOM_HIZ);
}

static inline void gpio_dir_output(uint8_t pad, uint8_t oe)
{
    uint32_t bits = 1UL << pad;
    
    if (oe == OE_HIGH)
    {
        GPIO_DAT_SET(bits);
    }
    else
    {
        GPIO_DAT_CLR(bits);
    }
    
    GPIO_DIR_SET(bits);
}

static inline void gpio_put(uint8_t pad, uint8_t val)
{
    if (val == OE_HIGH)
    {
        GPIO_DAT_SET(1UL << pad);
    }
    else
    {
        GPIO_DAT_CLR(1UL << pad);
    }
}

static inline void gpio_put_hi(uint8_t pad)
{
    GPIO_DAT_SET(1UL << pad);
}

static inline void gpio_put_lo(uint8_t pad)
{
    GPIO_DAT_CLR(1UL << pad);
}

static inline void gpio_dir_input(uint8_t pad, uint16_t ie)
{
    iom_ctrl(pad, ie);
}

static inline bool gpio_get(uint8_t pad)
{
    return ((GPIO_PIN_GET() >> pad) & 0x01);
}

static inline uint32_t gpio_get_all(void)
{
    return GPIO_PIN_GET();
}

#endif
