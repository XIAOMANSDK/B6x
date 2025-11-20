/**
 ****************************************************************************************
 *
 * @file pwm.c
 *
 * @brief Pulse Width Modulation(PWM) Driver
 *
 ****************************************************************************************
 */

#include "pwm.h"
#include "rcc.h"
#include "reg_timer.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// 获取定时器基地址宏，tmr: 定时器编号
#define TMR_USED(tmr)         ((TIMER_TypeDef* )(CTMR_BASE + (tmr) * 0x1000))

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * @brief 初始化PWM定时器
 * @param tmr 定时器编号 (0: CTMR, 1: ATMR)
 * @param psc 预分频器值
 * @param arr 自动重载值（决定PWM频率）
 */
void pwm_init(uint8_t tmr, uint16_t psc, uint16_t arr)
{
    TIMER_TypeDef* TIMx = TMR_USED(tmr);
    uint32_t tmr_bit = 1UL << (1 + tmr); //APB_CTMR_BIT APB_ATMR_BIT

    //enable clock, init PSC & Freq
    RCC_APBCLK_EN(tmr_bit);           /* 使能定时器时钟 */
    TIMx->CR1.CEN = 0;                /* CR1: 禁用计数器(CEN=0) */

    //if (conf != NULL)
    {
        TIMx->CNT = 0;                /* CNT: 清零计数器 */
        TIMx->PSC = psc;              /* PSC: 设置预分频器值 */
        TIMx->ARR = arr;              /* ARR: 设置自动重载值 */
    }

    TIMx->CR1.DIR = 0;                /* CR1: 设置为向上计数模式(DIR=0) */
    TIMx->CR1.CMS = 0;                /* CR1: 设置为边沿对齐模式(CMS=0) */
}

/**
 * @brief 反初始化PWM定时器
 * @param tmr 定时器编号 (0: CTMR, 1: ATMR)
 */
void pwm_deinit(uint8_t tmr)
{
    uint32_t tmr_bit = 1UL << (1 + tmr); //APB_CTMR_BIT APB_ATMR_BIT

    RCC_APBCLK_DIS(tmr_bit);          /* 禁用定时器时钟 */
    RCC_AHBRST_REQ(tmr_bit);          /* 请求定时器复位 */
}

/**
 * @brief 配置PWM定时器模式和中断
 * @param tmr 定时器编号 (0: CTMR, 1: ATMR)
 * @param smcr 从模式控制寄存器配置
 * @param intr 中断使能寄存器配置
 */
void pwm_conf(uint8_t tmr, uint16_t smcr, uint16_t intr)
{
    TIMER_TypeDef* TIMx = TMR_USED(tmr);

    TIMx->SMCR.Word = smcr;           /* SMCR: 配置从模式控制 */
    TIMx->IER.Word  = intr;           /* IER: 使能中断 */
    TIMx->ICR.Word  = intr;           /* ICR: 清除中断标志 */
}

/**
 * @brief 启动PWM定时器
 * @param tmr 定时器编号 (0: CTMR, 1: ATMR)
 */
void pwm_start(uint8_t tmr)
{
    TIMER_TypeDef* TIMx = TMR_USED(tmr);

    if (tmr == PWM_ATMR)
    {
        TIMx->BDTR.MOE = 1;           /* BDTR: 使能主输出(MOE=1) */
    }

    TIMx->CR1.ARPE = 1;               /* CR1: 使能自动重载预装载(ARPE=1) */
    TIMx->CR1.CEN = 1;                /* CR1: 使能计数器(CEN=1) */
    TIMx->EGR.UG = 1;                 /* EGR: 产生更新事件重新初始化计数器 */
}

/**
 * @brief 停止PWM定时器
 * @param tmr 定时器编号 (0: CTMR, 1: ATMR)
 */
void pwm_stop(uint8_t tmr)
{
    TIMER_TypeDef* TIMx = TMR_USED(tmr);
    TIMx->CR1.CEN = 0;                /* CR1: 禁用计数器(CEN=0) */
}

/**
 * @brief 配置PWM通道参数
 * @param chnl PWM通道编号 (PWM_CTMR_CH1~PWM_CTMR_CH4, PWM_ATMR_CH1P~PWM_ATMR_CH4P)
 * @param conf PWM通道配置参数指针
 */
#if (0)
void pwm_chnl_set(uint8_t chnl, const pwm_chnl_cfg_t *conf)
{
    uint8_t cidx = chnl % 4;
    TIMER_TypeDef* TIMx = TMR_USED(chnl / 4);

    // CCER&CCMR Clear
    TIMx->CCER.Word    &= ~(0x0F << (cidx * 4)); // bit[3:0]
    TIMx->DMAEN.Word   &= ~(0x01 << (cidx + 1)); // bit1 - CC1DE
    TIMx->CCMR[cidx/2] &= ~(0xFF << ((cidx % 2) * 8)); // bit[7:0]

    if (conf != NULL)
    {
        // CCR value
        TIMx->CCR[cidx]     = conf->duty;
        // CCMR conf
        TIMx->CCMR[cidx/2] |= (conf->ccmr << ((cidx % 2) * 8));
        // CCER enable
        TIMx->DMAEN.Word   |= (((conf->ccer & PWM_CCxDE_BIT) >> PWM_CCxDE_POS) << (cidx + 1));
        TIMx->CCER.Word    |= ((conf->ccer & 0x0F) << (cidx * 4));
    }
}
#else
void pwm_chnl_set(uint8_t chnl, const pwm_chnl_cfg_t *conf)
{
    uint8_t used_nchnl = (chnl > PWM_ATMR_CH4P) ? 2 : 0;  /* 判断是否为互补通道 */
    uint8_t cidx = chnl % 4;                              /* 通道索引 (0-3) */
    TIMER_TypeDef* TIMx = TMR_USED(chnl > PWM_CTMR_CH4);  /* 根据通道选择定时器 */

    // CCER&CCMR Clear
    TIMx->CCER.Word    &= ~(0x03 << ((cidx * 4) + used_nchnl)); /* CCER: 清除通道使能位 */
    TIMx->DMAEN.Word   &= ~(0x01 << (cidx + 1));               /* DMAEN: 清除DMA请求使能 */
    // CCMR在配置时清除

    if (conf != NULL)
    {
        // CCR value
        TIMx->CCR[cidx]     = conf->duty;                    /* CCR: 设置占空比值 */
        // CCMR conf
        TIMx->CCMR[cidx/2] &= ~(0xFF << ((cidx % 2) * 8));   /* CCMR: 清除通道模式配置 */
        TIMx->CCMR[cidx/2] |= (conf->ccmr << ((cidx % 2) * 8)); /* CCMR: 设置通道模式 */
        // CCER enable
        TIMx->DMAEN.Word   |= (((conf->ccer & PWM_CCxDE_BIT) >> PWM_CCxDE_POS) << (cidx + 1)); /* DMAEN: 设置DMA请求使能 */
        TIMx->CCER.Word    |= ((conf->ccer & 0x03) << ((cidx * 4) + used_nchnl)); /* CCER: 使能通道输出 */
    }
}
#endif

/**
 * @brief 更新PWM通道占空比
 * @param chnl PWM通道编号 (PWM_CTMR_CH1~PWM_CTMR_CH4, PWM_ATMR_CH1P~PWM_ATMR_CH4P)
 * @param duty 新的占空比值
 */
void pwm_duty_upd(uint8_t chnl, uint16_t duty)
{
    uint8_t cidx = chnl % 4;
    TIMER_TypeDef* TIMx = TMR_USED(chnl > PWM_CTMR_CH4);
    TIMx->CCR[cidx]     = duty;                             /* CCR: 更新占空比值 */
}
