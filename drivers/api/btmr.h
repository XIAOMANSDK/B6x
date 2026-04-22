#ifndef _BTMR_H_
#define _BTMR_H_

#include <stdint.h>

#if (ROM_UNUSED)
/**
 * @brief 使用基本定时器实现精确延时
 * @param tpsc 预分频器值，实际分频系数 = tpsc
 * @param tcnt 自动重载值，决定延时周期数
 * @note 此函数使用基本定时器的单次模式实现精确延时
 */
void btmr_delay(uint16_t tpsc, uint16_t tcnt);
#endif

/**
 * @brief 使用SysTick定时器实现延时
 * @param tpsc 预分频系数
 * @param tcnt 延时周期数
 * @note 此函数使用ARM Cortex-M的SysTick定时器实现延时
 */
void tick_delay(uint16_t tpsc, uint16_t tcnt);

#endif
