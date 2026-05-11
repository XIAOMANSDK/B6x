/**
 ****************************************************************************************
 *
 * @file cfg.h
 *
 * @brief App Configure MACRO, --preinclude
 *
 ****************************************************************************************
 */

#ifndef _APP_CFG_H_
#define _APP_CFG_H_

/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK             (0)

/// Flash Address of Loader's Information, *KEEP UP with startup_ld.s
#define LDR_RUN_ADDR        0x18004000
#define LDR_INFO_ADDR       0x18020000 // INFO_ADDR + 0x100 = otaAddr(0x18020100)

#endif  //_APP_CFG_H_
