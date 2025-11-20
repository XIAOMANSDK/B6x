/**
 ****************************************************************************************
 *
 * @file iopad.h
 *
 * @brief Header file - IOPAD with CSC Driver - IO引脚与交叉开关连接驱动
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
 * @brief 基础IO模式控制
 * @param[in] pad   IO引脚编号 @see enum pad_idx
 * @param[in] ctrl  模式控制值 @see enum iom_bfs
 * @note 配置指定IO引脚的电气特性和功能选择
 */
#define iom_ctrl(pad, ctrl)           CSC->CSC_PIO[pad].Word = ctrl

/**
 * @brief 基础CSC输入连接配置
 * @param[in] pad   IO引脚编号 @see enum pad_idx
 * @param[in] fsel  功能选择编号 @see enum csc_fsel
 * @note 配置指定功能的外设输入连接到指定的IO引脚
 * 
 * 根据CSC寄存器说明：
 * CSC_INPUT配置：CSC->CSC_INPUT[func].CSC_FSEL = pad
 * 例如：配置PA01引脚为UART2的RXD输入功能，则(pad=1, func=3): CSC->CSC_INPUT[3].CSC_FSEL = 1
 */
#define csc_input(pad, fsel)          CSC->CSC_INPUT[fsel].Word = CSC_EN(pad)

/**
 * @brief 基础CSC输出连接配置
 * @param[in] pad   IO引脚编号 @see enum pad_idx
 * @param[in] fsel  功能选择编号 @see enum csc_fsel
 * @note 配置指定IO引脚的输出功能为指定的外设功能
 * 
 * 根据CSC寄存器说明：
 * CSC_OUTPUT配置：CSC->CSC_OUTPUT[pad].CSC_FSEL = func
 * 例如：配置PA00引脚为UART2的TXD输出功能，则(pad=0, func=2): CSC->CSC_OUTPUT[0].CSC_FSEL = 2
 */
#define csc_output(pad, fsel)         CSC->CSC_OUTPUT[pad].Word = CSC_EN(fsel)


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 * @brief 复合CSC配置 - UART TXD和RXD功能
 * @param[in] port   UART端口号(0-UART1, 1-UART2)
 * @param[in] pad_tx 用于UART TXD的IO引脚 @see enum pad_idx
 * @param[in] pad_rx 用于UART RXD的IO引脚 @see enum pad_idx
 * @note 配置UART的发送和接收引脚，包括CSC连接和IO模式设置
 */
void iocsc_uart(uint8_t port, uint8_t pad_tx, uint8_t pad_rx);

/**
 * @brief 复合CSC配置 - UART硬件流控制功能
 * @param[in] port    UART端口号(0-UART1, 1-UART2)
 * @param[in] pad_rts 用于UART RTS的IO引脚 @see enum pad_idx
 * @param[in] pad_cts 用于UART CTS的IO引脚 @see enum pad_idx
 * @note 配置UART的硬件流控制引脚(RTS/CTS)，包括CSC连接和IO模式设置
 */
void iocsc_uart_hwfc(uint8_t port, uint8_t pad_rts, uint8_t pad_cts);

/**
 * @brief 复合CSC配置 - I2C功能
 * @param[in] pad_scl 用于I2C SCL的IO引脚 @see enum pad_idx
 * @param[in] pad_sda 用于I2C SDA的IO引脚 @see enum pad_idx
 * @note 配置I2C的时钟和数据引脚，支持双向开漏通信
 */
void iocsc_i2c(uint8_t pad_scl, uint8_t pad_sda);

/**
 * @brief 复合CSC配置 - SPI主设备功能
 * @param[in] pad_clk  用于SPI CLK的IO引脚 @see enum pad_idx
 * @param[in] pad_miso 用于SPI MISO的IO引脚 @see enum pad_idx
 * @param[in] pad_mosi 用于SPI MOSI的IO引脚 @see enum pad_idx
 * @note 配置SPI主设备的时钟、数据输入和数据输出引脚
 */
void iocsc_spim(uint8_t pad_clk, uint8_t pad_miso, uint8_t pad_mosi);

/**
 * @brief 复合CSC配置 - SPI从设备功能
 * @param[in] pad_cs   用于SPI CS的IO引脚 @see enum pad_idx
 * @param[in] pad_clk  用于SPI CLK的IO引脚 @see enum pad_idx
 * @param[in] pad_miso 用于SPI MISO的IO引脚 @see enum pad_idx
 * @param[in] pad_mosi 用于SPI MOSI的IO引脚 @see enum pad_idx
 * @note 配置SPI从设备的片选、时钟、数据输入和数据输出引脚
 */
void iocsc_spis(uint8_t pad_cs, uint8_t pad_clk, uint8_t pad_miso, uint8_t pad_mosi);

/**
 * @brief 复合CSC配置 - CTMR定时器通道功能
 * @param[in] pad_ch1 用于CTMR CH1的IO引脚 @see enum pad_idx
 * @param[in] pad_ch2 用于CTMR CH2的IO引脚 @see enum pad_idx
 * @note 配置CTMR定时器的PWM/PWC通道引脚，支持输入捕获和输出比较
 */
void iocsc_ctmr_chnl(uint8_t pad_ch1, uint8_t pad_ch2);

/**
 * @brief 配置模拟观察功能
 * @param[in] pad   IO引脚编号
 * @param[in] fsel  模拟功能选择 @see enum csc_fsel (17~24)
 * @note 配置指定IO引脚为模拟观察输出功能
 */
void iocsc_observe(uint8_t pad, uint8_t fsel); 

/**
 * @brief 配置时钟输出功能（固定PA05引脚）
 * @param[in] clk  选择的时钟类型 @see enum clk_out
 * @note 配置PA05引脚为时钟输出功能，支持多种时钟源选择
 */
void iospc_clkout(uint8_t clk);

/**
 * @brief 配置USB DP/DM引脚（固定PA06 PA07，高阻模式）
 * @note 配置USB数据引脚为高阻模式，用于USB通信
 */
void iospc_usbpin(void);

/**
 * @brief 配置SWD调试引脚（固定PA00 PA01，默认配置）
 * @note 配置SWD调试接口的时钟和数据引脚
 */
void iospc_swdpin(void);

/**
 * @brief 配置复位引脚功能（固定PA19）
 * @param[in] as_gpio  True-作为GPIO，False-作为nRESET
 * @note 配置PA19引脚功能，可选择作为普通GPIO或系统复位引脚
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
