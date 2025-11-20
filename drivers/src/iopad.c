/**
 ****************************************************************************************
 *
 * @file iopad.c
 *
 * @brief IOPAD with Cross Switch Connect(CSC) Driver - IO引脚与交叉开关连接驱动
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

#if (OPTM_SIZE)

/// CSC功能使能位定义
#define CSC_FEN_BIT                   (1UL << 7) /* Note Diff*/  // CSC功能使能位
#define CSC_EN(fsel)                  ((fsel) | CSC_FEN_BIT)     // 组合功能编号和使能位

/// CSC输入功能选择：'pad' @see enum pad_idx, 'fsel' @see enum csc_fsel -I
#define CSC_INPUT(pad, fsel)          CSC->CSC_INPUT[fsel].Word = CSC_EN(pad)
/// CSC输出功能选择：'pad' @see enum pad_idx, 'fsel' @see enum csc_fsel -O
#define CSC_OUTPUT(pad, fsel)         CSC->CSC_OUTPUT[pad].Word = CSC_EN(fsel)
/// IO模式控制：'pad' @see enum pad_idx, 'ctrl' @see enum iom_bfs
#define IOM_CTRL(pad, ctrl)           CSC->CSC_PIO[pad].Word = ctrl

/**
 * @brief 基础IO模式控制
 * @param[in] pad   IO引脚编号 @see enum pad_idx
 * @param[in] ctrl  模式控制值 @see enum iom_bfs
 * @note 配置指定IO引脚的电气特性和功能选择
 */
void iom_ctrl(uint8_t pad, uint16_t ctrl)
{
    IOM_CTRL(pad, ctrl);  // CSC_PIO[pad]：配置IO引脚模式控制寄存器
}

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
void csc_input(uint8_t pad, uint8_t fsel)
{
    CSC_INPUT(pad, fsel);  // CSC_INPUT[fsel]：配置功能fsel的输入连接到pad引脚
}

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
void csc_output(uint8_t pad, uint8_t fsel)
{
    CSC_OUTPUT(pad, fsel);  // CSC_OUTPUT[pad]：配置pad引脚的输出功能为fsel
}
#endif

/**
 * @brief 复合CSC配置 - UART TXD和RXD功能
 * @param[in] port   UART端口号(0-UART1, 1-UART2)
 * @param[in] pad_tx 用于UART TXD的IO引脚 @see enum pad_idx
 * @param[in] pad_rx 用于UART RXD的IO引脚 @see enum pad_idx
 * @note 配置UART的发送和接收引脚，包括CSC连接和IO模式设置
 */
void iocsc_uart(uint8_t port, uint8_t pad_tx, uint8_t pad_rx)
{
    uint8_t fsel_add = port * 2; // 计算功能编号偏移量：UART1=0, UART2=2

    // CSC输出和输入配置
    csc_output(pad_tx, CSC_UART1_TXD + fsel_add);  // 配置TXD输出功能
    csc_input(pad_rx,  CSC_UART1_RXD + fsel_add);  // 配置RXD输入功能

    // IO模式控制：TXD配置为CSC功能、上拉、驱动等级1；RXD配置为CSC功能、上拉、输入模式
    iom_ctrl(pad_tx, IOM_SEL_CSC | IOM_PULLUP | IOM_DRV_LVL1);
    iom_ctrl(pad_rx, IOM_SEL_CSC | IOM_PULLUP | IOM_INPUT);
}

/**
 * @brief 复合CSC配置 - UART硬件流控制功能
 * @param[in] port    UART端口号(0-UART1, 1-UART2)
 * @param[in] pad_rts 用于UART RTS的IO引脚 @see enum pad_idx
 * @param[in] pad_cts 用于UART CTS的IO引脚 @see enum pad_idx
 * @note 配置UART的硬件流控制引脚(RTS/CTS)，包括CSC连接和IO模式设置
 */
void iocsc_uart_hwfc(uint8_t port, uint8_t pad_rts, uint8_t pad_cts)
{
    uint8_t fsel_add = port; // 计算功能编号偏移量：UART1=0, UART2=1

    // CSC输出和输入配置
    csc_output(pad_rts, CSC_UART1_RTS + fsel_add);  // 配置RTS输出功能
    csc_input(pad_cts,  CSC_UART1_CTS + fsel_add);  // 配置CTS输入功能

    // IO模式控制：RTS配置为CSC功能、驱动等级1、上拉；CTS配置为CSC功能、上拉、输入模式
    iom_ctrl(pad_rts, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);
    iom_ctrl(pad_cts, IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
}

/**
 * @brief 复合CSC配置 - I2C功能
 * @param[in] pad_scl 用于I2C SCL的IO引脚 @see enum pad_idx
 * @param[in] pad_sda 用于I2C SDA的IO引脚 @see enum pad_idx
 * @note 配置I2C的时钟和数据引脚，支持双向开漏通信
 */
void iocsc_i2c(uint8_t pad_scl, uint8_t pad_sda)
{
    // CSC输出和输入配置：I2C引脚需要同时配置输入和输出功能
    csc_output(pad_scl, CSC_I2C_SCL);  // 配置SCL输出功能
    csc_input(pad_scl,  CSC_I2C_SCL);  // 配置SCL输入功能
    csc_output(pad_sda, CSC_I2C_SDA);  // 配置SDA输出功能
    csc_input(pad_sda,  CSC_I2C_SDA);  // 配置SDA输入功能

    // IO模式控制：配置为CSC功能、驱动等级1、上拉、输入模式、开漏输出
    iom_ctrl(pad_scl, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP | IOM_INPUT | IOM_OPENDRAIN);
    iom_ctrl(pad_sda, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP | IOM_INPUT | IOM_OPENDRAIN);
}

/**
 * @brief 复合CSC配置 - SPI主设备功能
 * @param[in] pad_clk  用于SPI CLK的IO引脚 @see enum pad_idx
 * @param[in] pad_miso 用于SPI MISO的IO引脚 @see enum pad_idx
 * @param[in] pad_mosi 用于SPI MOSI的IO引脚 @see enum pad_idx
 * @note 配置SPI主设备的时钟、数据输入和数据输出引脚
 */
void iocsc_spim(uint8_t pad_clk, uint8_t pad_miso, uint8_t pad_mosi)
{
    // CSC连接配置
    csc_output(pad_clk,  CSC_SPIM_CLK);   // 配置CLK输出功能
    csc_input(pad_miso,  CSC_SPIM_MISO);  // 配置MISO输入功能
    csc_output(pad_mosi, CSC_SPIM_MOSI);  // 配置MOSI输出功能

    // IO模式控制：CLK和MOSI配置为输出，MISO配置为输入
    iom_ctrl(pad_clk,  IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);
    iom_ctrl(pad_miso, IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
    iom_ctrl(pad_mosi, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);
}

/**
 * @brief 复合CSC配置 - SPI从设备功能
 * @param[in] pad_cs   用于SPI CS的IO引脚 @see enum pad_idx
 * @param[in] pad_clk  用于SPI CLK的IO引脚 @see enum pad_idx
 * @param[in] pad_miso 用于SPI MISO的IO引脚 @see enum pad_idx
 * @param[in] pad_mosi 用于SPI MOSI的IO引脚 @see enum pad_idx
 * @note 配置SPI从设备的片选、时钟、数据输入和数据输出引脚
 */
void iocsc_spis(uint8_t pad_cs, uint8_t pad_clk, uint8_t pad_miso, uint8_t pad_mosi)
{
    // CSC连接配置
    csc_input(pad_cs,    CSC_SPIS_CSN);   // 配置CS输入功能
    csc_input(pad_clk,   CSC_SPIS_CLK);   // 配置CLK输入功能
    csc_output(pad_miso, CSC_SPIS_MISO);  // 配置MISO输出功能
    csc_input(pad_mosi,  CSC_SPIS_MOSI);  // 配置MOSI输入功能

    // IO模式控制：CS、CLK、MOSI配置为输入，MISO配置为输出
    iom_ctrl(pad_cs,   IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
    iom_ctrl(pad_clk,  IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
    iom_ctrl(pad_miso, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);
    iom_ctrl(pad_mosi, IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
}

/**
 * @brief 复合CSC配置 - CTMR定时器通道功能
 * @param[in] pad_ch1 用于CTMR CH1的IO引脚 @see enum pad_idx
 * @param[in] pad_ch2 用于CTMR CH2的IO引脚 @see enum pad_idx
 * @note 配置CTMR定时器的PWM/PWC通道引脚，支持输入捕获和输出比较
 */
void iocsc_ctmr_chnl(uint8_t pad_ch1, uint8_t pad_ch2)
{
    if (pad_ch1 < PA_MAX)
    {
        // CSC输出和输入配置：CTMR通道需要同时配置输入和输出功能
        csc_output(pad_ch1, CSC_CTMR_CH1);  // 配置CH1输出功能
        csc_input(pad_ch1,  CSC_CTMR_CH1);  // 配置CH1输入功能
        // IO模式控制：配置为CSC功能、驱动等级1、上拉、输入模式
        iom_ctrl(pad_ch1, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP | IOM_INPUT);
    }

    if (pad_ch2 < PA_MAX)
    {
        csc_output(pad_ch2, CSC_CTMR_CH2);  // 配置CH2输出功能
        csc_input(pad_ch2,  CSC_CTMR_CH2);  // 配置CH2输入功能
        iom_ctrl(pad_ch2, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP | IOM_INPUT);
    }
}

/**
 * @brief 配置模拟观察功能
 * @param[in] pad   IO引脚编号
 * @param[in] fsel  模拟功能选择 @see enum csc_fsel (17~24)
 * @note 配置指定IO引脚为模拟观察输出功能
 */
void iocsc_observe(uint8_t pad, uint8_t fsel)
{
    csc_output(pad, fsel);  // 配置模拟观察输出功能
    iom_ctrl(pad, IOM_SEL_CSC | IOM_DRV_LVL1);  // 配置为CSC功能、驱动等级1
}

/**
 * @brief 配置时钟输出功能（固定PA05引脚）
 * @param[in] clk  选择的时钟类型 @see enum clk_out
 * @note 配置PA05引脚为时钟输出功能，支持多种时钟源选择
 */
void iospc_clkout(uint8_t clk)
{
    // 如果DPLL2启用，配置DPLL2输出48MHz时钟
    if (APBMISC->DPLL_CTRL.DPLL2_EN)
    {
        APBMISC->DPLL_CTRL.DPLL2_EN_CLK48M = 1;  // DPLL_CTRL：启用DPLL2的48MHz时钟输出
    }

    // 配置时钟输出选择
    RCC->CFG.MCO_SW = clk;  // CFG：选择主时钟输出源
    // 配置PA05为特殊功能模式、驱动等级1
    iom_ctrl(PA_CLKOUT, IOM_SEL_SPECL | IOM_DRV_LVL1);
}

/**
 * @brief 配置USB DP/DM引脚（固定PA06 PA07，高阻模式）
 * @note 配置USB数据引脚为高阻模式，用于USB通信
 */
void iospc_usbpin(void)
{
    // 清除USB引脚的方向寄存器，配置为输入
    GPIO->DIR_CLR = (1UL << PA_USB_DP) | (1UL << PA_USB_DM);  // DIR_CLR：配置为输入方向

    // 配置USB引脚为高阻模式
    iom_ctrl(PA_USB_DP, IOM_HIZ);  // 配置USB DP引脚为高阻
    iom_ctrl(PA_USB_DM, IOM_HIZ);  // 配置USB DM引脚为高阻
}

/**
 * @brief 配置SWD调试引脚（固定PA00 PA01，默认配置）
 * @note 配置SWD调试接口的时钟和数据引脚
 */
void iospc_swdpin(void)
{
    // 配置SWCLK为特殊功能、输入模式、下拉
    iom_ctrl(PA_SWCLK, IOM_SEL_SPECL | IOM_INPUT | IOM_PULLDOWN);
    // 配置SWDIO为特殊功能、输入模式、上拉
    iom_ctrl(PA_SWDIO, IOM_SEL_SPECL | IOM_INPUT | IOM_PULLUP);
}

/**
 * @brief 配置复位引脚功能（固定PA19）
 * @param[in] as_gpio  True-作为GPIO，False-作为nRESET
 * @note 配置PA19引脚功能，可选择作为普通GPIO或系统复位引脚
 */
void iospc_rstpin(bool as_gpio)
{
    AON->BKHOLD_CTRL.PIOA19_FUNC_SEL = as_gpio;  // BKHOLD_CTRL：配置PIOA19功能选择
}
