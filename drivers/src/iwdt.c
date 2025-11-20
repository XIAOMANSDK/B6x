/**
 ****************************************************************************************
 *
 * @file iwdt.c
 *
 * @brief 独立看门狗(IWDG)驱动程序
 *
 * 该文件包含独立看门狗(IWDG)的初始化、喂狗、反初始化和配置等功能的实现。
 * 独立看门狗可用于检测软件和硬件异常，如主时钟停振，程序跑飞不再喂狗等。
 *
 ****************************************************************************************
 */

#include "iwdt.h"
#include "rcc.h"
#include "reg_iwdt.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * @brief 初始化独立看门狗
 * 
 * @param[in] ctrl 控制寄存器值，用于配置IWDT的工作模式
 *                - 包含时钟选择(CLKSEL)、复位使能(RSTEN)、中断使能(INTEN)和外设使能(EN)位
 * 
 * @details
 * - 使能IWDT的APB时钟
 * - 解锁IWDT寄存器保护
 * - 禁用IWDT并清除中断标志
 * - 配置控制寄存器
 * - 锁定IWDT寄存器保护
 */
void iwdt_init(uint8_t ctrl)
{
    RCC_APBCLK_EN(APB_IWDT_BIT);  // 使能IWDT的APB时钟

    IWDT->LOCK    = 0x1ACCE551;   // 解锁IWDT寄存器保护
    IWDT->CTRL.EN = 0;            // 禁用IWDT外设使能位
    IWDT->INTCLR  = 1;            // 清除IWDT中断标志位

    IWDT->CTRL.Word = ctrl;       // 配置控制寄存器(CLKSEL/RSTEN/INTEN/EN)
    IWDT->LOCK = 0;               // 锁定IWDT寄存器保护
}

/**
 * @brief 喂狗操作，重新加载看门狗计数器
 * 
 * @details
 * - 解锁IWDT寄存器保护
 * - 写入INTCLR寄存器以重新加载IWDT计数器
 * - 锁定IWDT寄存器保护
 * 
 * @note 默认8s不喂狗会产生复位，需定期调用此函数
 */
void iwdt_feed(void)
{
    IWDT->LOCK   = 0x1ACCE551;    // 解锁IWDT寄存器保护
    IWDT->INTCLR = 1;             // 清除中断标志并重新加载计数器
    IWDT->LOCK   = 0;             // 锁定IWDT寄存器保护
}

/**
 * @brief 反初始化独立看门狗
 * 
 * @details
 * - 解锁IWDT寄存器保护
 * - 清除中断标志
 * - 禁用IWDT功能
 * - 锁定IWDT寄存器保护
 * - 关闭IWDT的APB时钟
 */
void iwdt_deinit(void)
{
    IWDT->LOCK      = 0x1ACCE551; // 解锁IWDT寄存器保护
    IWDT->INTCLR    = 1;          // 清除IWDT中断标志位
    IWDT->CTRL.Word = 0x08;       // 配置控制寄存器(具体值需参考实际需求)
    IWDT->LOCK      = 0;          // 锁定IWDT寄存器保护
    RCC_APBCLK_DIS(APB_IWDT_BIT); // 禁用IWDT的APB时钟
}

#if (ROM_UNUSED)
/**
 * @brief 配置独立看门狗的重载值
 * 
 * @param[in] load 计数器重载值，范围0x00000001~0xFFFFFFFF
 *                 - 如果为0，IWDT不计数
 *                 - 默认值：0x20000
 * 
 * @return uint32_t 配置前的IWDT计数器当前值
 * 
 * @details
 * - 解锁IWDT寄存器保护
 * - 清除中断标志
 * - 读取当前计数值
 * - 如果load>0，设置重载值并使能IWDT
 * - 如果load=0，禁用IWDT
 * - 锁定IWDT寄存器保护
 */
uint32_t iwdt_conf(uint32_t load)
{
    uint32_t value;

    IWDT->LOCK = 0x1ACCE551;      // 解锁IWDT寄存器保护
    IWDT->INTCLR = 1;             // 清除IWDT中断标志位

    value = IWDT->VALUE;          // 读取IWDT计数器当前值
    if (load > 0)
    {
        IWDT->LOAD = load;        // 设置IWDT计数器重载值
        IWDT->CTRL.EN = 1;        // 使能IWDT外设使能位
    }
    else
    {
        IWDT->CTRL.EN = 0;        // 禁用IWDT外设使能位
    }

    IWDT->LOCK = 0;               // 锁定IWDT寄存器保护

    return value;
}
#endif // (ROM_UNUSED)

