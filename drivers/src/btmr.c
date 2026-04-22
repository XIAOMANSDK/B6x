/**
 ****************************************************************************************
 *
 * @file btmr.c
 *
 * @brief Basic Timer Driver
 *
 ****************************************************************************************
 */
#include "b6x.h"
#include "reg_btmr.h"
#include "btmr.h"

#if (ROM_UNUSED)
#define CR1_ONE_MODE           (0x0C) // 单次模式配置：URS=1(bit2), OPM=1(bit3)

/**
 * @brief 使用基本定时器实现精确延时
 * @param tpsc 预分频器值，实际分频系数 = tpsc
 * @param tcnt 自动重载值，决定延时周期数
 * @note 此函数使用基本定时器的单次模式实现精确延时
 */
void btmr_delay(uint16_t tpsc, uint16_t tcnt)
{
    // 配置定时器参数
    BTMR->CR1.Word = CR1_ONE_MODE;  // 设置单次模式，URS=1(只有计数器溢出时生成更新事件)，OPM=1(单脉冲模式)
    BTMR->PSC = tpsc - 1;           // 设置预分频器值
    BTMR->ARR = tcnt - 1;           // 设置自动重载值
    BTMR->CNT = 0;                  // 清零计数器
    BTMR->EGR = 1;                  // 生成更新事件，重新初始化计数器
    BTMR->ICR = 1;                  // 清除更新中断标志

    // 使能计数器，等待定时器到达设定时间
    BTMR->CR1.CEN = 1;              // CEN=1，使能计数器
    while(!BTMR->RIF && BTMR->CR1.CEN); // 等待更新中断标志置位或计数器停止
    BTMR->ICR = 1;                  // 清除更新中断标志

    // 清除模式设置
    BTMR->CR1.Word = 0;             // 禁用计数器，清除所有控制位
}
#endif

/**
 * @brief 使用SysTick定时器实现延时
 * @param tpsc 预分频系数
 * @param tcnt 延时周期数
 * @note 此函数使用ARM Cortex-M的SysTick定时器实现延时
 */
void tick_delay(uint16_t tpsc, uint16_t tcnt)
{
    uint32_t temp;

    SysTick->LOAD = tcnt * tpsc - 1;    // 设置重载值
    SysTick->VAL  = 0x00;               // 清零当前值
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // 使能SysTick，使用处理器时钟

    do
    {
        temp = SysTick->CTRL;           // 读取控制状态寄存器
    } while((temp & SysTick_CTRL_ENABLE_Msk) && !(temp & SysTick_CTRL_COUNTFLAG_Msk)); // 等待计数完成

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // 禁用SysTick
    SysTick->VAL   = 0x00;              // 清零当前值
}