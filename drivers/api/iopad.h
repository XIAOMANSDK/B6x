/**
 ****************************************************************************************
 *
 * @file iopad.h
 *
 * @brief Header file - IOPAD with CSC Driver
 *
 ****************************************************************************************
 */

#ifndef _IOPAD_H_
#define _IOPAD_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Indexs of pad
enum pad_idx
{
    // gpio pad
    PA00               = 0,
    PA01               = 1,
    PA02               = 2,
    PA03               = 3,
    PA04               = 4,
    PA05               = 5,
    PA06               = 6,
    PA07               = 7,
    PA08               = 8,
    PA09               = 9,
    PA10               = 10,
    PA11               = 11,
    PA12               = 12,
    PA13               = 13,
    PA14               = 14,
    PA15               = 15,
    PA16               = 16,
    PA17               = 17,
    PA18               = 18,
    PA19               = 19,
    PA_MAX,
    
    // pad of special func
    PA_SWCLK           = PA00,
    PA_SWDIO           = PA01,
    PA_CLKOUT          = PA05,
    PA_RSTPIN          = PA19,
    
    // pad of USB signal
    PA_USB_DP          = PA06,
    PA_USB_DM          = PA07,
    PA_SOF_SG          = PA15,
    
    // pad of analog func
    PA_ADCIN0          = PA00,
    PA_ADCIN1          = PA01,
    PA_ADCIN2          = PA02,
    PA_MICIN           = PA03,
    PA_ADCIN3          = PA04,
    PA_ADCIN4          = PA05,
    PA_ADCIN5          = PA06,
    PA_ADCIN6          = PA07,
    PA_ADCIN7          = PA08,
    PA_ADCIN8          = PA09,
    PA_ADCIN9          = PA10,
    PA_ADCIN0_B        = PA11,
    PA_ADCIN1_B        = PA12,
    PA_ADCIN2_B        = PA13,
    PA_ADCIN4_B        = PA14,
    PA_ADCIN5_B        = PA15,
    PA_ADCIN6_B        = PA16,
    PA_LDOTESTA        = PA17,
    PA_AT0             = PA18,
    PA_AT1             = PA19,
};

/// Bits Field of IO Mode @see CSC_IO_CTRL_TypeDef
enum iom_bfs
{
    // set 0 to Hi-Z
    IOM_HIZ            = 0x00,
    
    // bit[2:0]  -- IO Function Select
    IOM_SEL_GPIO       = (0 << 0),  // default function
    IOM_SEL_CSC        = (1 << 0),  // cross switch connect
    IOM_SEL_SPECL      = (2 << 0),  // swd/clk_out
    IOM_SEL_TIMER      = (3 << 0),  // timer function
    IOM_SEL_DEBUG      = (4 << 0),
    IOM_SEL_RFTEST     = (5 << 0),
    IOM_SEL_ADC_DBG    = (6 << 0),
    IOM_SEL_USB        = (7 << 0),
    
    // bit[5]  -- IO Drive Strength
    IOM_DRV_LVL0       = (0 << 5),  // default Driver Strength
    IOM_DRV_LVL1       = (1 << 5),  // Max Driver Strength
    
    // bit[11:6] -- IO Enable Mode
    IOM_PULLDOWN       = (1 << 6),  // Pull Down Control
    IOM_PULLUP         = (1 << 7),  // IO Pull Up
    IOM_INPUT          = (1 << 8),  // IO Input Enable
    IOM_CURSRC         = (1 << 9),  // IO 1ma Current Source Enable
    IOM_ANALOG         = (1 << 10), // IO Analog Enable
    IOM_OPENDRAIN      = (1 << 11), // IO Open Drain Enable
};

/// Selected functions of CSC (I:Input, O:Output)
enum csc_fsel
{
    // Both Output and Input func
    CSC_UART1_TXD      = 0,  // O/I can Swap
    CSC_UART1_RXD      = 1,  // I/O can Swap
    CSC_UART2_TXD      = 2,  // O/I can Swap
    CSC_UART2_RXD      = 3,  // I/O can Swap
    CSC_I2C_SCL        = 4,  // I&O OpenDrain
    CSC_I2C_SDA        = 5,  // I&O OpenDrain
    CSC_CTMR_CH1       = 6,  // I/O pwc/pwm
    CSC_CTMR_CH2       = 7,  // I/O pwc/pwm
    
    // Output func
    CSC_UART1_RTS      = 8,  // O
    CSC_UART2_RTS      = 9,  // O
    CSC_UART1_SCK      = 10, // O
    CSC_UART2_SCK      = 11, // O
    CSC_SPIM_CLK       = 12, // O
    CSC_SPIM_MOSI      = 13, // O
    CSC_SPIS_MISO      = 14, // O
    
    // Input func
    CSC_UART1_CTS      = 8,  // I
    CSC_UART2_CTS      = 9,  // I
    CSC_SPIM_MISO      = 10, // I
    CSC_SPIS_CLK       = 11, // I
    CSC_SPIS_MOSI      = 12, // I
    CSC_SPIS_CSN       = 13, // I

    // Output global bist indicate
    CSC_GLB_BIST_DONE  = 15, // O
    CSC_GLB_BIST_FAIL  = 16, // O
    
    // Output analog observe func
    CSC_OSCEN_FLAG     = 17, // O
    CSC_RFEN_FLAG      = 18, // O
    CSC_BLE_SLEEP_FLAG = 19, // O
    CSC_CM0P_DEEPSLEEP = 20, // O
    CSC_BB_TX_EN       = 21, // O
    CSC_BB_RX_EN       = 22, // O
    CSC_MDM_TX_EN      = 23, // O
    CSC_MDM_RX_EN      = 24, // O
};

enum clk_out
{
    CLK_OUT_NONE       = 0,
    CLK_OUT_HSI        = 1,
    CLK_OUT_XO16M      = 2,
    CLK_OUT_DPLL       = 3,
    CLK_OUT_LSI        = 4,
    CLK_OUT_SYSCLK     = 5,
    CLK_OUT_AHB        = 6,
    CLK_OUT_APB1       = 7,
    CLK_OUT_APB2       = 8,
    CLK_OUT_FSHC       = 9,
    CLK_OUT_USB        = 10,
};


/*
 * MACROS DECLARATION
 ****************************************************************************************
 */

#include "reg_csc.h"

/// CSC peripheral function enable @see CSC_OUTPUT_CTRL_TypeDef & CSC_INPUT_CTRL_TypeDef
#define CSC_FEN_BIT                   (1UL << 7) /* Note Diff*/
#define CSC_EN(fsel)                  ((fsel) | CSC_FEN_BIT)

/**
 ****************************************************************************************
 * @brief Base IO Mode Control
 *
 * @param[in] pad   Index of pad used @see enum pad_idx
 * @param[in] ctrl  value of mode @see enum iom_bfs
 *
 ****************************************************************************************
 */
#define iom_ctrl(pad, ctrl)           CSC->CSC_PIO[pad].Word = ctrl

/**
 ****************************************************************************************
 * @brief Base CSC Input Connect
 *
 * @param[in] pad   Index of pad used @see enum pad_idx
 * @param[in] fsel  value of func @see enum csc_fsel -I
 *
 ****************************************************************************************
 */
#define csc_input(pad, fsel)          CSC->CSC_INPUT[fsel].Word = CSC_EN(pad)

/**
 ****************************************************************************************
 * @brief Base CSC Output Connect
 *
 * @param[in] pad   Index of pad used @see enum pad_idx
 * @param[in] fsel  value of func @see enum csc_fsel -O
 *
 ****************************************************************************************
 */
#define csc_output(pad, fsel)         CSC->CSC_OUTPUT[pad].Word = CSC_EN(fsel)


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Composite CSC for UART Txd and Rxd
 *
 * @param[in] port    uart port(0-UART1, 1-UART2)
 * @param[in] pad_tx  pad used for uart txd @see enum pad_idx
 * @param[in] pad_rx  pad used for uart rxd @see enum pad_idx
 *
 ****************************************************************************************
 */
void iocsc_uart(uint8_t port, uint8_t pad_tx, uint8_t pad_rx);

/**
 ****************************************************************************************
 * @brief Composite CSC for UART HW Flow-Control
 *
 * @param[in] port     uart port(0-UART1, 1-UART2)
 * @param[in] pad_rts  pad used for uart rts @see enum pad_idx
 * @param[in] pad_cts  pad used for uart cts @see enum pad_idx
 *
 ****************************************************************************************
 */
void iocsc_uart_hwfc(uint8_t port, uint8_t pad_rts, uint8_t pad_cts);

/**
 ****************************************************************************************
 * @brief Composite CSC for I2C
 *
 * @param[in] pad_scl  pad used for I2C scl @see enum pad_idx
 * @param[in] pad_sda  pad used for I2C sda @see enum pad_idx
 *
 ****************************************************************************************
 */
void iocsc_i2c(uint8_t pad_scl, uint8_t pad_sda);

/**
 ****************************************************************************************
 * @brief Composite CSC for SPI Master Role
 *
 * @param[in] pad_clk   pad used for SPI clk @see enum pad_idx
 * @param[in] pad_miso  pad used for SPI miso @see enum pad_idx
 * @param[in] pad_mosi  pad used for SPI mosi @see enum pad_idx
 *
 ****************************************************************************************
 */
void iocsc_spim(uint8_t pad_clk, uint8_t pad_miso, uint8_t pad_mosi);

/**
 ****************************************************************************************
 * @brief Composite CSC for SPI Slave Role
 *
 * @param[in] pad_cs    pad used for SPI cs @see enum pad_idx
 * @param[in] pad_clk   pad used for SPI clk @see enum pad_idx
 * @param[in] pad_miso  pad used for SPI miso @see enum pad_idx
 * @param[in] pad_mosi  pad used for SPI mosi @see enum pad_idx
 *
 ****************************************************************************************
 */
void iocsc_spis(uint8_t pad_cs, uint8_t pad_clk, uint8_t pad_miso, uint8_t pad_mosi);

/**
 ****************************************************************************************
 * @brief Composite CSC for CTMR PWM/PWC channel
 *
 * @param[in] pad_ch1  pad used for CTMR CH1 @see enum pad_idx
 * @param[in] pad_ch2  pad used for CTMR CH2 @see enum pad_idx
 *
 ****************************************************************************************
 */
void iocsc_ctmr_chnl(uint8_t pad_ch1, uint8_t pad_ch2);

/**
 ****************************************************************************************
 * @brief Analog observe via pad
 *
 * @param[in] pad   pad used for observe @see enum pad_idx
 * @param[in] fsel  Func of analog @see enum csc_fsel (17~24)
 *
 ****************************************************************************************
 */
void iocsc_observe(uint8_t pad, uint8_t fsel); 

/**
 ****************************************************************************************
 * @brief Special Clock output via PA05(Fixed)
 *
 * @param[in] clk  Type of Clock Out @see enum clk_out
 *
 ****************************************************************************************
 */
void iospc_clkout(uint8_t clk);

/**
 ****************************************************************************************
 * @brief Special USB DP/DM via PA06 PA07(Fixed, HiZ mode)
 *
 ****************************************************************************************
 */
void iospc_usbpin(void);

/**
 ****************************************************************************************
 * @brief Special SWCLK SWDIO via PA00 PA01(Fixed, default)
 *
 ****************************************************************************************
 */
void iospc_swdpin(void);

/**
 ****************************************************************************************
 * @brief Special nRESET via PA19(Fixed, default)
 *
 * @param[in] as_gpio  true as GPIO, false as nRESET
 *
 ****************************************************************************************
 */
void iospc_rstpin(bool as_gpio);

// Macro for Deprecated Functions {
#define ioSelUartTxRx                iocsc_uart
#define ioSelUartRtsCts              iocsc_uart_hwfc
#define ioSelI2c                     iocsc_i2c
#define ioSelSpiMaster               iocsc_spim
#define ioSelSpiSlave                iocsc_spis
#define ioClkOut(clk)                iospc_clkout(clk)
#define ioOscen(pad)                 iocsc_observe(pad, CSC_OSCEN_FLAG)
#define ioRFen(pad)                  iocsc_observe(pad, CSC_RFEN_FLAG)
#define ioCoreSleep(pad)             iocsc_observe(pad, CSC_CM0P_DEEPSLEEP)
#define ioBleSleep(pad)              iocsc_observe(pad, CSC_BLE_SLEEP_FLAG)
#define ioBleTxRx(pad_tx, pad_rx)    do{iocsc_observe(pad_tx, CSC_BB_TX_EN);iocsc_observe(pad_rx, CSC_BB_RX_EN);}while(0)
#define ioModemTxRx(pad_tx, pad_rx)  do{iocsc_observe(pad_tx, CSC_MDM_TX_EN);iocsc_observe(pad_rx, CSC_MDM_RX_EN);}while(0)
// }

#endif // _IOPAD_H_
