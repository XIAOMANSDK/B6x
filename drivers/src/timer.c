/**
 ****************************************************************************************
 *
 * @file timer.c
 *
 * @brief Base/Common/Advance Timer Driver
 *
 ****************************************************************************************
 */

#include "timer.h"
#include "rcc.h"
#include "reg_timer.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if 0
/**
 * @brief 基本定时器延时函数
 * @param tpsc 预分频器值
 * @param tcnt 自动重载值
 */
void btmr_delay(uint16_t tpsc, uint16_t tcnt)
{
    // config Params
    BTMR->CR1.Word = CR1_ONE_MODE;  /* CR1: 配置为单脉冲模式(OPM=1) */
    BTMR->PSC = tpsc - 1;           /* PSC: 设置预分频器值 */
    BTMR->ARR = tcnt - 1;           /* ARR: 设置自动重载值 */
    BTMR->CNT = 0;                  /* CNT: 清零计数器 */
    BTMR->EGR = 1;                  /* EGR: 产生更新事件，重新初始化计数器 */

    // enable CEN, wait Time-Reach
    BTMR->CR1.CEN = 1;              /* CR1: 使能计数器(CEN=1) */
    while(!BTMR->RIF && BTMR->CR1.CEN);  /* 等待更新中断标志或计数器停止 */
    BTMR->ICR = 1;                  /* ICR: 清除更新中断标志 */

    // clear Mode
    BTMR->CR1.Word = 0;             /* CR1: 禁用计数器 */
}
#endif

/**
 * @brief 初始化通用定时器
 * @param psc 预分频器值
 * @param arr 自动重载值
 */
void ctmr_init(uint16_t psc, uint16_t arr)
{
    RCC_APBCLK_EN(APB_CTMR_BIT);    /* 使能通用定时器时钟 */
    RCC_APBRST_REQ(APB_CTMR_BIT);   /* 请求通用定时器复位 */

    CTMR->CR1.Word = 0;             /* CR1: 禁用计数器(CEN=0) */
    CTMR->PSC = psc;                /* PSC: 设置预分频器值 */
    CTMR->ARR = arr;                /* ARR: 设置自动重载值 */
}

/**
 * @brief 反初始化通用定时器
 */
void ctmr_deinit(void)
{
    RCC_APBCLK_DIS(APB_CTMR_BIT);   /* 禁用通用定时器时钟 */
    RCC_APBRST_REQ(APB_CTMR_BIT);   /* 请求通用定时器复位 */
}

/**
 * @brief 控制通用定时器
 * @param mode 控制寄存器1配置模式
 * @param intr 中断配置
 */
void ctmr_ctrl(uint16_t mode, uint16_t intr)
{
    // clear interrupt, then enable
    CTMR->ICR.Word = intr;          /* ICR: 清除指定中断标志 */
    CTMR->IER.Word = intr;          /* IER: 使能指定中断 */
    // event generation
    CTMR->EGR.Word = intr | 0x01;   /* EGR: 产生更新事件(UG=1)和指定事件 */

    // conf control (counter start from 0)
    CTMR->CNT = 0;                  /* CNT: 清零计数器 */
    CTMR->CR1.Word = mode;          /* CR1: 配置控制模式 */
}

/**
 * @brief 初始化高级定时器
 * @param psc 预分频器值
 * @param arr 自动重载值
 */
void atmr_init(uint16_t psc, uint16_t arr)
{
    RCC_APBCLK_EN(APB_ATMR_BIT);    /* 使能高级定时器时钟 */
    RCC_APBRST_REQ(APB_ATMR_BIT);   /* 请求高级定时器复位 */

    ATMR->CR1.Word = 0;             /* CR1: 禁用计数器(CEN=0) */
    ATMR->PSC = psc;                /* PSC: 设置预分频器值 */
    ATMR->ARR = arr;                /* ARR: 设置自动重载值 */
}

/**
 * @brief 反初始化高级定时器
 */
void atmr_deinit(void)
{
    RCC_APBCLK_DIS(APB_ATMR_BIT);   /* 禁用高级定时器时钟 */
    RCC_APBRST_REQ(APB_ATMR_BIT);   /* 请求高级定时器复位 */
}

/**
 * @brief 控制高级定时器
 * @param mode 控制寄存器1配置模式
 * @param intr 中断配置
 */
void atmr_ctrl(uint16_t mode, uint16_t intr)
{
    // clear interrupt, then enable
    ATMR->ICR.Word = intr;          /* ICR: 清除指定中断标志 */
    ATMR->IER.Word = intr;          /* IER: 使能指定中断 */
    // event generation
    ATMR->EGR.Word = intr | 0x01;   /* EGR: 产生更新事件(UG=1)和指定事件 */

    // conf control (counter start from 0)
    ATMR->CNT = 0;                  /* CNT: 清零计数器 */
    ATMR->CR1.Word = mode;          /* CR1: 配置控制模式 */
}
