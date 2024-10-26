/**
 ****************************************************************************************
 *
 * @file iopad.c
 *
 * @brief IOPAD with Cross Switch Connect(CSC) Driver
 *
 ****************************************************************************************
 */

#include "iopad.h"
#include "reg_csc.h"
#include "reg_rcc.h" // for clkout
#include "reg_aon.h"
#include "reg_gpio.h"
#include "reg_apbmisc.h"


/*
 * DEFINES
 ****************************************************************************************
 */



/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if (OPTM_SIZE)

/// CSC Ctrl: peripheral function enable
#define CSC_FEN_BIT                   (1UL << 7) /* Note Diff*/
#define CSC_EN(fsel)                  ((fsel) | CSC_FEN_BIT)

/// CSC Input func select : 'pad' @see enum pad_idx, 'fsel' @see enum csc_fsel -I
#define CSC_INPUT(pad, fsel)          CSC->CSC_INPUT[fsel].Word = CSC_EN(pad)
/// CSC Output func select: 'pad' @see enum pad_idx, 'fsel' @see enum csc_fsel -O
#define CSC_OUTPUT(pad, fsel)         CSC->CSC_OUTPUT[pad].Word = CSC_EN(fsel)
/// Config IO mode: 'pad' @see enum pad_idx, 'ctrl' @see enum iom_bfs
#define IOM_CTRL(pad, ctrl)           CSC->CSC_PIO[pad].Word = ctrl

/**
 ****************************************************************************************
 * @brief Base IO Mode Control
 *
 * @param[in] pad   Index of pad used @see enum pad_idx
 * @param[in] ctrl  value of mode @see enum iom_bfs
 *
 ****************************************************************************************
 */
void iom_ctrl(uint8_t pad, uint16_t ctrl)
{
    IOM_CTRL(pad, ctrl);
}

/**
 ****************************************************************************************
 * @brief Base CSC Input Connect
 *
 * @param[in] pad   Index of pad used @see enum pad_idx
 * @param[in] fsel  value of func @see enum csc_fsel
 *
 ****************************************************************************************
 */
void csc_input(uint8_t pad, uint8_t fsel)
{
    CSC_INPUT(pad, fsel);
}

/**
 ****************************************************************************************
 * @brief Base CSC Output Connect
 *
 * @param[in] pad   Index of pad used @see enum pad_idx
 * @param[in] fsel  value of func @see enum csc_fsel
 *
 ****************************************************************************************
 */
void csc_output(uint8_t pad, uint8_t fsel)
{
    CSC_OUTPUT(pad, fsel);
}
#endif


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
void iocsc_uart(uint8_t port, uint8_t pad_tx, uint8_t pad_rx)
{
    uint8_t fsel_add = port * 2; //(port == 1) ? 2 : 0;

    // csc output and input
    csc_output(pad_tx, CSC_UART1_TXD + fsel_add);
    csc_input(pad_rx,  CSC_UART1_RXD + fsel_add);

    // iomode control
    iom_ctrl(pad_tx, IOM_SEL_CSC | IOM_PULLUP | IOM_DRV_LVL1);
    iom_ctrl(pad_rx, IOM_SEL_CSC | IOM_PULLUP | IOM_INPUT);
}

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
void iocsc_uart_hwfc(uint8_t port, uint8_t pad_rts, uint8_t pad_cts)
{
    uint8_t fsel_add = port; //(port == 1) ? 1 : 0;

    // csc output and input
    csc_output(pad_rts, CSC_UART1_RTS + fsel_add);
    csc_input(pad_cts,  CSC_UART1_CTS + fsel_add);

    // iomode control
    iom_ctrl(pad_rts, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);
    iom_ctrl(pad_cts, IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
}

/**
 ****************************************************************************************
 * @brief Composite CSC for I2C
 *
 * @param[in] pad_scl  pad used for I2C scl @see enum pad_idx
 * @param[in] pad_sda  pad used for I2C sda @see enum pad_idx
 *
 ****************************************************************************************
 */
void iocsc_i2c(uint8_t pad_scl, uint8_t pad_sda)
{
    // csc output and input
    csc_output(pad_scl, CSC_I2C_SCL);
    csc_input(pad_scl,  CSC_I2C_SCL);
    csc_output(pad_sda, CSC_I2C_SDA);
    csc_input(pad_sda,  CSC_I2C_SDA);

    // iomode control
    iom_ctrl(pad_scl, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP | IOM_INPUT | IOM_OPENDRAIN);
    iom_ctrl(pad_sda, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP | IOM_INPUT | IOM_OPENDRAIN);
}

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
void iocsc_spim(uint8_t pad_clk, uint8_t pad_miso, uint8_t pad_mosi)
{
    // csc connect
    csc_output(pad_clk,  CSC_SPIM_CLK);
    csc_input(pad_miso,  CSC_SPIM_MISO);
    csc_output(pad_mosi, CSC_SPIM_MOSI);

    // iomode control
    iom_ctrl(pad_clk,  IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);
    iom_ctrl(pad_miso, IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
    iom_ctrl(pad_mosi, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);
}

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
void iocsc_spis(uint8_t pad_cs, uint8_t pad_clk, uint8_t pad_miso, uint8_t pad_mosi)
{
    // csc connect
    csc_input(pad_cs,    CSC_SPIS_CSN);
    csc_input(pad_clk,   CSC_SPIS_CLK);
    csc_output(pad_miso, CSC_SPIS_MISO);
    csc_input(pad_mosi,  CSC_SPIS_MOSI);

    // iomode control
    iom_ctrl(pad_cs,   IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
    iom_ctrl(pad_clk,  IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
    iom_ctrl(pad_miso, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);
    iom_ctrl(pad_mosi, IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
}

/**
 ****************************************************************************************
 * @brief Composite CSC for CTMR PWM/PWC channel
 *
 * @param[in] pad_ch1  pad used for CTMR CH1 @see enum pad_idx
 * @param[in] pad_ch2  pad used for CTMR CH2 @see enum pad_idx
 *
 ****************************************************************************************
 */
void iocsc_ctmr_chnl(uint8_t pad_ch1, uint8_t pad_ch2)
{
    if (pad_ch1 < PA_MAX)
    {
        // csc output and input
        csc_output(pad_ch1, CSC_CTMR_CH1);
        csc_input(pad_ch1,  CSC_CTMR_CH1);
        // iomode control
        iom_ctrl(pad_ch1, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP | IOM_INPUT);
    }

    if (pad_ch2 < PA_MAX)
    {
        csc_output(pad_ch2, CSC_CTMR_CH2);
        csc_input(pad_ch2,  CSC_CTMR_CH2);
        iom_ctrl(pad_ch2, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP | IOM_INPUT);
    }
}

/**
 ****************************************************************************************
 * @brief Select analog observe
 *
 * @param[in] pad   Index of pad used
 * @param[in] fsel  Func of analog @see enum csc_fsel (17~24)
 *
 ****************************************************************************************
 */
void iocsc_observe(uint8_t pad, uint8_t fsel)
{
    csc_output(pad, fsel);
    iom_ctrl(pad, IOM_SEL_CSC | IOM_DRV_LVL1);
}

/**
 ****************************************************************************************
 * @brief Select Clock output via PA05(Fixed)
 *
 * @param[in] clk  Type of Selected-Clock @see enum clk_out
 *
 ****************************************************************************************
 */
void iospc_clkout(uint8_t clk)
{
    if (APBMISC->DPLL_CTRL.DPLL2_EN)
    {
        APBMISC->DPLL_CTRL.DPLL2_EN_CLK48M = 1;
    }

    RCC->CFG.MCO_SW = clk;
    iom_ctrl(PA_CLKOUT, IOM_SEL_SPECL | IOM_DRV_LVL1);
}

/**
 ****************************************************************************************
 * @brief Special USB DP/DM via PA06 PA07(Fixed, HiZ mode)
 *
 ****************************************************************************************
 */
void iospc_usbpin(void)
{
    GPIO->DIR_CLR = (1UL << PA_USB_DP) | (1UL << PA_USB_DM);

    iom_ctrl(PA_USB_DP, IOM_HIZ);
    iom_ctrl(PA_USB_DM, IOM_HIZ);
}

/**
 ****************************************************************************************
 * @brief Special SWCLK SWDIO via PA00 PA01(Fixed, default)
 *
 * @param[in] as_gpio  True as GPIO, Flase as SWD
 *
 ****************************************************************************************
 */
void iospc_swdpin(void)
{
    iom_ctrl(PA_SWCLK, IOM_SEL_SPECL | IOM_INPUT | IOM_PULLDOWN);
    iom_ctrl(PA_SWDIO, IOM_SEL_SPECL | IOM_INPUT | IOM_PULLUP);
}

/**
 ****************************************************************************************
 * @brief Special nRESET via PA19(Fixed, default)
 *
 * @param[in] as_gpio  True as GPIO, Flase as nRESET
 *
 ****************************************************************************************
 */
void iospc_rstpin(bool as_gpio)
{
    AON->BKHOLD_CTRL.PIOA19_FUNC_SEL = as_gpio;
}
