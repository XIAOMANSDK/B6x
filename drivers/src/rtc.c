/**
 ****************************************************************************************
 *
 * @file rtc.c
 *
 * @brief 实时时钟(RTC)驱动程序
 *
 * 该文件包含实时时钟(RTC)的配置、时间获取/设置、报警功能、中断和唤醒控制等功能的实现。
 * RTC模块提供高精度的时间计数功能，支持秒和毫秒级时间管理。
 *
 ****************************************************************************************
 */

#include "rtc.h"
#include "reg_aon.h"
#include "reg_apbmisc.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * @brief 配置RTC模块
 *
 * @param[in] en  使能控制: True-使能RTC, False-禁用RTC
 *
 * @details
 * - 使能AON和APBMISC模块时钟
 * - 禁用RTC中断并清除中断状态
 * - 配置RTC使能控制位
 */
void rtc_conf(bool en)
{
    #if (ROM_UNUSED)
    // 时钟使能（在ROM中保持使能）
    // RCC->APBCLK_EN_RUN.AON_CLKEN_RUN     = 1;
    // RCC->APBCLK_EN_RUN.APBMISC_CLKEN_RUN = 1;
    RCC_APBCLK_EN(APB_APBMISC_BIT | APB_AON_BIT);  /* 使能APBMISC和AON模块时钟 */
    #endif

    // RTC中断禁用和状态清除
    APBMISC->RTCINT_CTRL.Word = 0x02;          /* RTC_INT_EN=0(禁用中断), RTC_INT_CLR=1(清除中断标志) */

    // RTC使能或禁用
    AON->BLE_RTC_CTL.RTC_EN = en;              /* RTC_EN: RTC模块使能控制位 */
}

/**
 * @brief 获取当前RTC时间
 *
 * @return rtc_time_t 包含秒和毫秒的RTC时间结构体
 *
 * @details
 * - 从影子寄存器读取秒和毫秒值
 * - 检查秒值是否发生变化，如果变化则重新读取以确保时间一致性
 * - 返回包含秒和毫秒的时间结构体
 */
rtc_time_t rtc_time_get(void)
{
    rtc_time_t time;

    time.sec = APBMISC->RTC_SEC_SHD;           /* 从影子寄存器读取秒值 */
    time.ms  = APBMISC->RTC_MS_SHD;            /* 从影子寄存器读取毫秒值 */

    if (time.sec != APBMISC->RTC_SEC_SHD)
    {
        // 刚好跨越到下一秒
        time.sec = APBMISC->RTC_SEC_SHD;       /* 重新读取秒值 */
        time.ms  = APBMISC->RTC_MS_SHD;        /* 重新读取毫秒值 */
    }

    return time;
}

/**
 * @brief 设置/更改当前RTC时间
 *
 * @param[in] sec  秒时间值
 * @param[in] ms   毫秒时间值 (范围: 0~999ms)
 *
 * @details
 * - 设置RTC秒寄存器
 * - 设置RTC毫秒寄存器
 * - 等待RTC当前时间寄存器写入完成
 */
void rtc_time_set(uint32_t sec, uint32_t ms)
{
    APBMISC->RTC_SEC = sec;                    /* 设置RTC秒寄存器 */
    APBMISC->RTC_MS  = ms;                     /* 设置RTC毫秒寄存器 */

    // 等待RTC当前时间寄存器写入完成
    while(APBMISC->RTCINT_CTRL.RTC_SET_BUSY);  /* RTC_SET_BUSY: RTC设置忙状态位 */
}

/**
 * @brief 设置报警时间
 *
 * @param[in] ms_time  报警时间(单位: 毫秒)
 *
 * @details
 * - 禁用中断并清除上次报警状态
 * - 如果报警时间不为0，则计算并设置报警时间
 * - 获取当前RTC时间
 * - 计算报警时间(当前时间 + 报警时间偏移)
 * - 设置报警秒和毫秒寄存器
 * - 使能RTC中断
 */
void rtc_alarm_set(uint32_t ms_time)
{
    // 禁用中断，清除上次报警状态
    APBMISC->RTCINT_CTRL.RTC_INT_EN = 0;       /* RTC_INT_EN: RTC中断使能位 */
    APBMISC->RTCINT_CTRL.RTC_INT_CLR = 1;      /* RTC_INT_CLR: RTC中断清除位 */

    if (ms_time)
    {
        rtc_time_t time;

        // 等待RTC报警时间寄存器写入完成
        while(APBMISC->RTCINT_CTRL.RTC_SET_BUSY);  /* RTC_SET_BUSY: RTC设置忙状态位 */

        // time = rtc_time_get();
        time.sec = APBMISC->RTC_SEC_SHD;       /* 读取当前秒值 */
        time.ms  = APBMISC->RTC_MS_SHD;        /* 读取当前毫秒值 */

        if (time.sec != APBMISC->RTC_SEC_SHD)
        {
            // 刚好跨越到下一秒
            time.sec = APBMISC->RTC_SEC_SHD;   /* 重新读取秒值 */
            time.ms  = APBMISC->RTC_MS_SHD;    /* 重新读取毫秒值 */
        }

        // 添加时间偏移，然后设置报警
        ms_time += time.ms;                    /* 计算总毫秒数 */
        APBMISC->RTC_ALARM_SEC = time.sec + (ms_time / 1000);  /* 设置报警秒值 */
        APBMISC->RTC_ALARM_MS  = ms_time % 1000;               /* 设置报警毫秒值 */

        // 使能中断
        APBMISC->RTCINT_CTRL.RTC_INT_EN = 1;   /* RTC_INT_EN: RTC中断使能位 */
    }
}

/**
 * @brief 获取已配置的报警RTC时间
 *
 * @return rtc_time_t 包含报警秒和毫秒的时间结构体
 *
 * @details
 * - 从报警寄存器读取秒和毫秒值
 * - 返回包含报警时间的结构体
 */
rtc_time_t rtc_alarm_get(void)
{
    rtc_time_t time;

    time.sec = APBMISC->RTC_ALARM_SEC;         /* 读取报警秒寄存器 */
    time.ms  = APBMISC->RTC_ALARM_MS;          /* 读取报警毫秒寄存器 */
    return time;
}

/**
 * @brief 判断报警时间是否到达，如果到达则自动清除标志
 *
 * @return bool True-报警时间到达, False-报警时间未到达
 *
 * @details
 * - 检查RTC中断状态位
 * - 如果报警触发，则清除中断标志
 * - 返回报警状态
 */
bool rtc_is_alarm(void)
{
    bool ret = APBMISC->RTCINT_CTRL.RTC_INT_ST;  /* RTC_INT_ST: RTC中断状态位 */

    if (ret)
    {
        APBMISC->RTCINT_CTRL.RTC_INT_CLR = 1;  /* RTC_INT_CLR: RTC中断清除位 */
    }

    return ret;
}

/**
 * @brief 设置RTC中断模式
 *
 * @param[in] en  中断使能控制: True-使能中断, False-禁用中断
 *
 * @details
 * - 配置RTC中断使能位
 * - 清除中断标志位
 */
void rtc_irq_set(bool en)
{
    APBMISC->RTCINT_CTRL.RTC_INT_EN = en;      /* RTC_INT_EN: RTC中断使能位 */
    APBMISC->RTCINT_CTRL.RTC_INT_CLR = 1;      /* RTC_INT_CLR: RTC中断清除位 */
}

/**
 * @brief 设置RTC唤醒模式
 *
 * @param[in] en  唤醒使能控制: True-使能唤醒, False-禁用唤醒
 *
 * @details
 * - 配置PMU唤醒控制寄存器中的RTC唤醒使能位
 * - 清除唤醒状态标志
 */
void rtc_wkup_set(bool en)
{
    AON->PMU_WKUP_CTRL.RTC_WKUP_EN = en;       /* RTC_WKUP_EN: RTC唤醒使能位 */
    APBMISC->AON_PMU_CTRL.WKUP_ST_CLR = 1;     /* WKUP_ST_CLR: 唤醒状态清除位 */
}
