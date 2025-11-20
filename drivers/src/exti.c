/**
 ****************************************************************************************
 *
 * @file exti.c
 *
 * @brief 外部中断(EXTI)配置驱动
 *
 ****************************************************************************************
 */

#include "exti.h"
#include "rcc.h"
#include "reg_exti.h"

/**
 * @brief 外部中断初始化
 * @param debounce 去抖时间配置值
 * @note 启用外部中断时钟，复位EXTI控制器，配置去抖时间
 */
void exti_init(uint16_t debounce)
{
    // 启用外部中断APB时钟
    RCC_APBCLK_EN(APB_EXTI_BIT);
    // 请求外部中断复位
    RCC_APBRST_REQ(APB_EXTI_BIT);

    // 配置外部中断去抖时间
    EXTI->DBC.Word = debounce;  // DBC：设置去抖计数器值，用于消除GPIO抖动
}

/**
 * @brief 外部中断反初始化
 * @note 禁用外部中断时钟，复位EXTI控制器
 */
void exti_deinit(void)
{
    // 禁用外部中断APB时钟
    RCC_APBCLK_DIS(APB_EXTI_BIT);
    // 请求外部中断复位
    RCC_APBRST_REQ(APB_EXTI_BIT);
}

/**
 * @brief 设置外部中断寄存器
 * @param typ 寄存器类型（偏移量，0-4对应不同寄存器）
 * @param loca 配置值（位图，对应GPIO19~GPIO0）
 * @note 根据寄存器类型设置相应的EXTI寄存器
 * 
 * 寄存器类型对应关系：
 * - 0: 外部中断使能寄存器(IER) - 使能指定GPIO的中断功能
 * - 1: 外部中断关闭寄存器(IDR) - 关闭指定GPIO的中断功能  
 * - 2: 外部中断有效状态寄存器(IVS) - 读取中断有效状态
 * - 3: 外部中断标志状态寄存器(RIF) - 读取中断标志状态
 * - 4: 外部中断屏蔽寄存器(IFM) - 配置中断屏蔽
 */
void exti_set(uint8_t typ, uint32_t loca)
{
    // 根据寄存器类型设置相应的EXTI寄存器
    // EXTI_BASE + typ * 4 计算寄存器地址偏移
    (*(volatile uint32_t*)(EXTI_BASE + typ * 4)) = loca;
}

/**
 * @brief 获取外部中断寄存器值
 * @param typ 寄存器类型（偏移量，0-4对应不同寄存器）
 * @return 寄存器当前值
 * @note 读取指定类型的EXTI寄存器值
 * 
 * 寄存器类型对应关系：
 * - 0: 外部中断使能寄存器(IER) - 读取中断使能状态
 * - 1: 外部中断关闭寄存器(IDR) - 读取中断关闭状态
 * - 2: 外部中断有效状态寄存器(IVS) - 读取中断有效状态
 * - 3: 外部中断标志状态寄存器(RIF) - 读取中断标志状态
 * - 4: 外部中断屏蔽寄存器(IFM) - 读取中断屏蔽状态
 */
uint32_t exti_get(uint8_t typ)
{
    // 根据寄存器类型读取相应的EXTI寄存器值
    // EXTI_BASE + typ * 4 计算寄存器地址偏移
    return (*(volatile uint32_t*)(EXTI_BASE + typ * 4));
}
