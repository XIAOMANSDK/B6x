/**
 ****************************************************************************************
 *
 * @file rom.h
 *
 * @brief Header file - Drivers defined in ROM
 *
 ****************************************************************************************
 */

#ifndef _ROM_H_
#define _ROM_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Address Pointer of APIs
enum ROM_POINTER {
    ROM_BOOTLOADER           = 0x000001b9, 
    ROM_PUART_CONF           = 0x0000032d, 
    ROM_PUART_GETC           = 0x00000379, 
    ROM_PUART_PUTC           = 0x00000387, 
    ROM_PUART_WAIT           = 0x00000393, 
    ROM_PUART_INIT           = 0x000003a3, 
    ROM_UARTSYNC             = 0x00000469, 
    ROM_UARTPARSE            = 0x000004a9, 
    ROM_SYSJUMPTO            = 0x00000731, 
    ROM_BTMR_DELAY           = 0x00000749, 
    ROM_XO16M_EN             = 0x00000777, 
    ROM_EXTCLK_EN            = 0x0000079b, 
    ROM_DPLL_EN              = 0x000007bd, 
    ROM_DEEPSLEEP            = 0x000007df, 
    ROM_RSTCLR               = 0x00000845, 
    ROM_RCC_FSHCLK_SET       = 0x00000877, 
    ROM_IWDT_CONF            = 0x000008d1, 
    ROM_RC16M_CALIB          = 0x000008fd, 
    ROM_TRIMVALLOAD          = 0x0000097d, 
    ROM_FSHC_XIP_CONF        = 0x00000abd, 
    ROM_FSHC_SUSPEND_CONF    = 0x00000ae1, 
    ROM_FSHC_HPM_CONF        = 0x00000b1f, 
    ROM_FSHC_EN_CMD          = 0x00000b59, 
    ROM_FSHC_WR_STA          = 0x00000b73, 
    ROM_FSHC_RD_STA          = 0x00000b9d, 
    ROM_FSHC_WR_CFG          = 0x00000bcd, 
    ROM_FSHC_WR_FIFO         = 0x00000beb, 
    ROM_FSHC_RD_CFG          = 0x00000c21, 
    ROM_FSHC_RD_FIFO         = 0x00000c41, 
    ROM_FSHC_ERASE           = 0x00000c63, 
    ROM_FSHC_WRITE           = 0x00000cb9, 
    ROM_FSHC_READ            = 0x00000d1d, 
    ROM_FLASHINIT            = 0x00000d6d, 
    ROM_FSHC_CAPDLY_CFG      = 0x00000dc9, 
    ROM_FSHC_QUAD_MODE       = 0x00000e25, 
    ROM_PUYA_QUADHPMXIPMODE  = 0x00000e41, 
    ROM_PUYA_ENTER_HPM       = 0x00000e61, 
    ROM_PUYA_EXIT_HPM        = 0x00000ead, 
    ROM_XMEMCPY              = 0x00000f19, 
    ROM_XMEMMOVE             = 0x00000f59, 
    ROM_XMEMSET              = 0x00000f75, 
    ROM_XMEMCMP              = 0x00000fa3, 
};

/// Flash Data length type
typedef uint32_t           flen_t;


/*
 * APIs : Memory Operation
 ****************************************************************************************
 */

/// Same as memcpy, direct copy uint32_t if aligned
#define xmemcpy            ((void (*)(void *dst, const void *src, uint32_t size))ROM_XMEMCPY)

/// Same as memmove, reverse copy or inner call xmemcpy
#define xmemmove           ((void (*)(void * dst, const void * src, uint32_t size))ROM_XMEMMOVE)

/// Same as xmemset, fill cccc to uint32_t when address aligned
#define xmemset            ((void (*)(void *m, uint8_t c, uint32_t size))ROM_XMEMSET)

/// Same as memcmp, direct compare uint32_t if aligned
#define xmemcmp            ((int (*)(const void *m1, const void *m2, uint32_t size))ROM_XMEMCMP)


/*
 * APIs : Core Utility
 ****************************************************************************************
 */

/// Core enter deep sleep mode, inner call __wfi
#define deepsleep          ((void (*)(void))ROM_DEEPSLEEP)

/// Clear states of Core reset and wakeup, return the reason and source
/// @see enum rst_src_bfs
#define rstrsn             ((uint16_t (*)(void))ROM_RSTCLR)

/// Jump to the running 'addr' of application, SP PC STACK reinited
#define sysJumpTo          ((void (*)(uint32_t addr))ROM_SYSJUMPTO)

/// Enable xosc16m Oscillator mode, wait 2ms for stable
#define xo16m_en           ((void (*)(void))ROM_XO16M_EN)

/// Enable extarnal clock mode, through XOSC16M_IN PAD into chip
#define extclk_en          ((void (*)(void))ROM_EXTCLK_EN)

/// Enable dpll power and clk, wait 50us for stable
#define dpll_en            ((void (*)(void))ROM_DPLL_EN)

/// Delay via basic timer, wait 'tcnt' count in 'tpsc' prescaler 
/// eg. btmr_delay(16, 1000) means delay 1Ms when sysclk is 16MHz
#define btmr_delay         ((void (*)(uint16_t tpsc, uint16_t tcnt))ROM_BTMR_DELAY)
/// time must great than 1
#if (SYS_CLK == 1)
    #define bootDelayMs(time)  btmr_delay(32000, time)
    #define bootDelayUs(time)  btmr_delay(32, time)
#elif (SYS_CLK == 2)
    #define bootDelayMs(time)  btmr_delay(48000, time)
    #define bootDelayUs(time)  btmr_delay(48, time)
#elif (SYS_CLK == 3)
    #define bootDelayMs(time)  btmr_delay(64000, time)
    #define bootDelayUs(time)  btmr_delay(64, time)
#else
    #define bootDelayMs(time)  btmr_delay(16000, time)
    #define bootDelayUs(time)  btmr_delay(16, time)
#endif //SYS_CLK

/// Config IWDT, disable if 'load' equl 0
#define iwdt_conf          ((uint32_t (*)(uint32_t load))ROM_IWDT_CONF)

/// Calib rc16m, return trim value(range 0~63)
#define rc16m_calib        ((uint8_t (*)(void))ROM_RC16M_CALIB)


/*
 * APIs : Boot Uart(UART1) 
 ****************************************************************************************
 */

/// Csc PA6 PA7 as pUart PIN
#define pUart_init         ((void (*)(void))ROM_PUART_INIT)

/// Config pUart params
#define pUart_conf         ((void (*)(uint16_t cfg_BRR, uint16_t cfg_LCR))ROM_PUART_CONF)

/// Get one data from pUart RXD(Blocking)
#define pUart_getc         ((uint8_t (*)(void))ROM_PUART_GETC)

/// Put one byte via pUart TXD(Blocking)
#define pUart_putc         ((void (*)(uint8_t ch))ROM_PUART_PUTC)

/// Wait pUart finished(not busy)
#define pUart_wait         ((void (*)(void))ROM_PUART_WAIT)


/*
 * APIs: Flash Controlor(FSHC) Driver
 ****************************************************************************************
 */

/// Set Fshc Clock Selection @see enum fsh_clk_sel
#define rcc_fshclk_set     ((void (*)(uint8_t fclk_sel))ROM_RCC_FSHCLK_SET)

/// Config XIP mode
#define fshc_xip_conf      ((void (*)(uint8_t rdCmd, uint8_t adrBits, uint32_t dlySet))ROM_FSHC_XIP_CONF)

/// Config HPM mode
#define fshc_hpm_conf      ((void (*)(bool en, uint8_t crIdx, uint8_t crCmd))ROM_FSHC_HPM_CONF)

/// Config Suspend/Resume
#define fshc_suspend_conf  ((void (*)(uint8_t susCmd, uint8_t rsmCmd, uint16_t susTime, uint32_t rsmTime))ROM_FSHC_SUSPEND_CONF)

/**
 ****************************************************************************************
 * @brief Send control/enable command without value
 *
 * @param[in] cmd  Command opcode(eg. FSH_CMD_RST_EN FSH_CMD_RESET FSH_CMD_EXIT_HMP 
 *                                    FSH_CMD_WR_EN FSH_CMD_WR_STA_EN)
 *
 ****************************************************************************************
 */
#define fshc_en_cmd        ((void (*)(uint8_t cmd))ROM_FSHC_EN_CMD)

/**
 ****************************************************************************************
 * @brief Write value to flash state register
 *
 * @param[in] cmd  Command opcode(eg. FSH_CMD_WR_STA)
 * @param[in] len  Length of value, range 1~4 Bytes
 * @param[in] val  State value, valid range 8/16/24/32bits by 'len'
 *
 ****************************************************************************************
 */
#define fshc_wr_sta        ((void (*)(uint8_t cmd, uint8_t len, uint32_t val))ROM_FSHC_WR_STA)

/**
 ****************************************************************************************
 * @brief Read value from flash state register
 *
 * @param[in] cmd  Command opcode(eg. FSH_CMD_RD_ID FSH_CMD_RD_STA0 FSH_CMD_RD_STA1)
 * @param[in] len  Length of value, range 1~4 Bytes
 *
 * @return value  State value, valid range 8/16/24/32bits by 'len'
 ****************************************************************************************
 */
#define fshc_rd_sta        ((uint32_t (*)(uint8_t cmd, uint8_t len))ROM_FSHC_RD_STA)

/**
 ****************************************************************************************
 * @brief Perpare write data to flash
 *
 * @param[in] cmd     fshc access cmd (example read cmd: 0x03)
 * @param[in] offset  access flash addr offset
 * @param[in] len     access flash data len, unit is byte
 * @param[in] sctrl   access flash mode config(example: spi mode, dual mode...)
 * @param[in] acbit   dummy cycle
 *
 ****************************************************************************************
 */
#define fshc_wr_cfg        ((void (*)(uint8_t cmd, uint32_t offset, flen_t len, uint16_t sctrl, uint16_t acbit))ROM_FSHC_WR_CFG)

/// Fill 'data'(unit in uint32_t, length='wlen') to Fshc Write-FiFo in 'fcmd' mode
#define fshc_wr_fifo       ((flen_t (*)(const uint32_t *data, flen_t wlen, uint16_t fcmd))fshc_wr_fifo)

/**
 ****************************************************************************************
 * @brief Perpare read data from flash
 *
 * @param[in] cmd     fshc access cmd (example read cmd: 0x03)
 * @param[in] offset  access flash addr offset
 * @param[in] len     access flash data len, unit is byte
 * @param[in] sctrl   access flash mode config(example: spi mode, dual mode...)
 * @param[in] acbit   dummy cycle
 *
 ****************************************************************************************
 */
#define fshc_rd_cfg        ((void (*)(uint8_t cmd, uint32_t offset, flen_t len, uint16_t sctrl, uint16_t acbit))ROM_FSHC_RD_CFG)

/// Get data from Fshc Read-FiFo to 'buff'(unit in uint32_t, length='wlen') in 'fcmd' mode
#define fshc_rd_fifo       ((flen_t (*)(uint32_t *buff, flen_t wlen, uint16_t fcmd))ROM_FSHC_RD_FIFO)

/// Erase Flash in 'fcmd' config, inner call fshc_en_cmd() fshc_wr_cfg()
#define fshc_erase         ((void (*)(uint32_t offset, uint16_t fcmd))ROM_FSHC_ERASE)

/// Read Flash in 'fcmd' config, inner call fshc_rd_cfg() fshc_rd_fifo()
#define fshc_read          ((flen_t (*)(uint32_t offset, uint32_t *buff, flen_t wlen, uint16_t fcmd))ROM_FSHC_READ)

/// Write Flash in 'fcmd' config, inner call fshc_en_cmd() fshc_wr_cfg()fshc_wr_fifo()
#define fshc_write         ((flen_t (*)(uint32_t offset, const uint32_t *data, flen_t wlen, uint16_t fcmd))ROM_FSHC_WRITE)

/// Config Flash Cap Dealy cell(0~7, default 5), auto search if 'dly' > 7
#define fshc_capdly_cfg    ((uint8_t (*)(uint8_t dly))ROM_FSHC_CAPDLY_CFG)

/// Config Flash Access Mode, enter/exit Quad mode with special 'val'
#define fshc_quad_mode     ((void (*)(uint16_t val))ROM_FSHC_QUAD_MODE)

/// BoYa Flash enter HPM mode
#define boya_enter_hpm     ((void (*)(void))ROM_PUYA_ENTER_HPM)

/// BoYa Flash exit HPM Mode
#define boya_exit_hpm      ((void (*)(void))ROM_PUYA_EXIT_HPM)


#endif // _ROM_H_
