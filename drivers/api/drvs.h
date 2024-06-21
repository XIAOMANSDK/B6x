/**
 ****************************************************************************************
 *
 * @file drvs.h
 *
 * @brief Header file - B6x drivers wrapper
 *
 ****************************************************************************************
 */
 
#ifndef _DRVS_H_
#define _DRVS_H_

/*
 *  This macro is for use by other macros to form a fully valid C statement.
 *  Without this, the if/else conditionals could show unexpected behavior.
 * (The while condition below evaluates false without generating a
 *  constant-controlling-loop type of warning on most compilers.)
 */
#if !defined(dowl)
    #define dowl(x)         do { x } while(__LINE__ == -1)
#endif

#if !defined(BIT)
    #define BIT(n)          (1UL << (n))
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "string.h"

#include "b6x.h"
#include "rom.h"

#include "core.h"
#include "dma.h"
#include "exti.h"
#include "fshc.h"
#include "flash.h"
#include "gpio.h"
#include "i2c.h"
#include "iopad.h"
#include "iwdt.h"
#include "pwm.h"
#include "rcc.h"
#include "rtc.h"
#include "rco.h"
#include "sadc.h"
#include "spi.h"
#include "timer.h"
#include "trim.h"
#include "uart.h"

// Alter system clock(@see sys_clk_sel 0:16M 1:32M 2:48M 3:64M)
#ifndef SYS_CLK
#define SYS_CLK             (0)
#endif

#if (SYS_CLK)
#define SYS_CLK_ALTER()     rcc_sysclk_set(SYS_CLK)
#if (SYS_CLK == 1)
#define SYS_GET_CLOCK()     (32000000)
#elif (SYS_CLK == 2)
#define SYS_GET_CLOCK()     (48000000)
#else
#define SYS_GET_CLOCK()     (64000000)
#endif
#else
#define SYS_CLK_ALTER()
#define SYS_GET_CLOCK()     (16000000)
#endif

// RC32K Init with Calibration
//   rc32k_conf(RCLK_HSE, 0x1F);  // 14ms
//   rc32k_conf(RCLK_DPLL, 7);    // 3.7ms
//   rc32k_conf(RCLK_DPLL128, 3); // 1.8ms
#if !defined(rc32k_init)
#define rc32k_init()        dowl(rc32k_conf(RCLK_DPLL, 7); rc32k_calib();)
#endif

#ifndef __SRAMFN
#define __SRAMFN            __attribute__((section("ram_func")))
#endif

#ifndef __ATTR_SRAM
#define __ATTR_SRAM         __attribute__((section("ram_func")))
#endif

#ifndef __DATA_ALIGNED
#define __DATA_ALIGNED(n)   __attribute__((aligned (n)))
#endif

#ifndef __RETENTION
#define __RETENTION         __attribute__((section("user_retention"), zero_init))
#endif

#endif // _DRVS_H_
