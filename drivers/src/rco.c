/**
 ****************************************************************************************
 *
 * @file rco.c
 *
 * @brief RCO(32k, 16M) 晶振驱动程序
 *
 * 该文件包含RCO(32k, 16M)晶振的校准、配置和频率调整等功能的实现。
 * 支持使用HSE、DPLL64、DPLL128作为参考时钟进行RC32K和RC16M的校准。
 *
 ****************************************************************************************
 */

#include "rco.h"
#include "reg_rcc.h"
#include "reg_aon.h"
#include "reg_apbmisc.h"

#define TARGET_32000HZ 1
/*
 * DEFINES
 ****************************************************************************************
 */

#define HSE_CALIB_MULTI        500  // 16M/32K  /* HSE校准倍数 */

#if (TARGET_32000HZ)
#define DPLL_CAKLIB_MULTI      2000 // 64M/32K  /* DPLL校准倍数(64MHz) */
#define DPLL48M_CAKLIB_MULTI   1500 // 48M/32K  /* DPLL校准倍数(48MHz) */
#else //31992Hz
#define DPLL_CAKLIB_MULTI      20005 // 64M/31992 * 10  /* DPLL校准倍数(64MHz, 31992Hz目标) */
#define DPLL48M_CAKLIB_MULTI   15003 // 48M/31993 * 10  /* DPLL校准倍数(48MHz, 31992Hz目标) */
#endif


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * @brief 配置RC32K校准参数
 * 
 * @param[in] ref_clk  参考时钟选择 @see enum rc32k_ref_clk
 * @param[in] cal_ctrl 校准控制配置 @see enum rc32k_cal_ctrl
 * 
 * @details
 * - 使能AON和APBMISC时钟
 * - 配置RCCALIB时钟源和使能
 * - 根据参考时钟计算校准目标值
 * - 配置RCCALIB控制寄存器
 */
void rc32k_conf(uint8_t ref_clk, uint8_t cal_ctrl)
{
    uint32_t target = 0;               // bit[19:0]  /* 校准目标值 */
    uint8_t cycle = (cal_ctrl & 0x1F); // bit[27:20] /* 校准周期数 */

    #if (ROM_UNUSED)
    // 时钟使能（在ROM中保持使能）
    // RCC->APBCLK_EN_RUN.AON_CLKEN_RUN     = 1;
    // RCC->APBCLK_EN_RUN.APBMISC_CLKEN_RUN = 1;
    RCC_APBCLK_EN(APB_APBMISC_BIT | APB_AON_BIT);  /* 使能AON和APBMISC时钟 */
    #endif

    RCC->CLK_EN_ST.RCCALIB_CLKEN  = 1;         /* 使能RCCALIB时钟 */
    RCC->CLK_EN_ST.RCCALIB_CLKSEL = ref_clk;   /* 选择RCCALIB参考时钟源 */

    if (ref_clk == RCLK_HSE/*0*/)
    {
        target = HSE_CALIB_MULTI * (cycle + 1);  /* 使用HSE作为参考时钟计算目标值 */
    }
    else //if ((ref_clk == RCLK_DPLL64/*1*/) || (ref_clk == RCLK_DPLL128/*2*/))
    {
        uint16_t multi = ((RCC->CFG.DPLL_CLK_SW == 0) || (ref_clk == RCLK_DPLL128) ? DPLL_CAKLIB_MULTI : DPLL48M_CAKLIB_MULTI);  /* 根据DPLL时钟选择倍数 */
        #if (TARGET_32000HZ)
        target =  multi* ref_clk * (cycle + 1);  /* 计算32000Hz目标值 */
        #else
        target =  multi* ref_clk * (cycle + 1) / 10;  /* 计算31992Hz目标值 */
        #endif
    }

    // bit[19:0]--rccalib_target, bit[27:20]--cycle, bit28--SCAL_EN, bit29--DLY
    APBMISC->RCCALIB_CTRL.Word = (target & 0xFFFFFUL) | ((uint32_t)cal_ctrl << APBMISC_RCCALIB_CYCLES_LSB) | (1UL << APBMISC_RCCALIB_DLY_POS);  /* 配置RCCALIB控制寄存器 */
}

/**
 * @brief 启动RC32K校准过程
 * 
 * @return uint16_t RC32K校准后的修调值
 * 
 * @details
 * - 选择使用校准结果控制RC32K
 * - 启动RCCALIB校准
 * - 等待校准完成并清除完成标志
 * - 获取校准修调值并配置到AON寄存器
 * - 恢复使用AON修调值控制RC32K
 */
uint16_t rc32k_calib(void)
{
    uint16_t trim_val = 0;

    // 使用校准结果控制RC32K
    AON->PMU_WKUP_CTRL.RC32K_TRIM_SEL = 1;     /* 选择RCCALIB修调值控制RC32K */

    // 启动RC校准
    APBMISC->RCCALIB_START = 1;                /* 启动RCCALIB校准过程 */

    // 等待完成，然后清除标志
    while(!APBMISC->RCCALIB_STCLR.RCCALIB_DONE);  /* 等待校准完成 */
    APBMISC->RCCALIB_STCLR.RCCALIB_DONE_CLR = 1;  /* 清除校准完成标志 */

    // 获取RC32K校准修调值
    //AON->BKHOLD_CTRL.RC32K_MSB_TRIM_CFG = APBMISC->RC32K_CALIB_ST.RC32K_MSB_CALIB;
    //AON->BKHOLD_CTRL.RC32K_LSB_TRIM_CFG = APBMISC->RC32K_CALIB_ST.RC32K_LSB_CALIB;
    //uint16_t value = APBMISC->RC32K_CALIB_ST.RC32K_MSB_CALIB | (APBMISC->RC32K_CALIB_ST.RC32K_LSB_CALIB << 4);
    trim_val = (APBMISC->RC32K_CALIB_ST.Word & 0xF/*MSB*/) | (((APBMISC->RC32K_CALIB_ST.Word >> 16) & 0x3FF) << 4/*LSB*/);  /* 组合MSB和LSB修调值 */

    AON->BKHOLD_CTRL.Word = (AON->BKHOLD_CTRL.Word & ~(0x3FFF << 0)) | (((uint32_t)trim_val & 0x3FFF) << 0);  /* 配置修调值到AON寄存器 */

    // 使用AON的修调值控制RC32K
    AON->PMU_WKUP_CTRL.RC32K_TRIM_SEL = 0;     /* 恢复使用AON修调值控制RC32K */

    return trim_val;
}

/**
 * @brief 直接设置RC32K修调值
 * 
 * @param[in] value   修调值(0~0x3FFF)
 * 
 * @details
 * - 使能AON时钟
 * - 配置修调值到AON寄存器
 * - 选择使用AON修调值控制RC32K
 */
void rc32k_trim_set(uint16_t value)
{
    #if (ROM_UNUSED)
    // 时钟使能（在ROM中保持使能）
    // RCC->APBCLK_EN_RUN.AON_CLKEN_RUN = 1;
    RCC_APBCLK_EN(APB_AON_BIT);                /* 使能AON时钟 */
    #endif

    //AON->BKHOLD_CTRL.RC32K_MSB_TRIM_CFG = value & 0xF;          // bit[3:0]
    //AON->BKHOLD_CTRL.RC32K_LSB_TRIM_CFG = (value >> 4) & 0x3FF; // bit[13:4]
    AON->BKHOLD_CTRL.Word = (AON->BKHOLD_CTRL.Word & ~(0x3FFF << 0)) | (((uint32_t)value & 0x3FFF) << 0);  /* 直接配置修调值 */

    // 使用AON的修调值控制RC32K
    AON->PMU_WKUP_CTRL.RC32K_TRIM_SEL = 0;     /* 选择AON修调值控制RC32K */
}

/**
 * @brief 获取当前RC32K使用的修调值
 * 
 * @return uint16_t 当前修调值(0~0x3FFF)
 * 
 * @details
 * - 使能AON时钟
 * - 根据修调值选择位确定当前使用的修调值源
 * - 返回相应的修调值
 */
uint16_t rc32k_trim_get(void)
{
    uint16_t value;

    #if (ROM_UNUSED)
    // RCC->APBCLK_EN_RUN.AON_CLKEN_RUN = 1;
    RCC_APBCLK_EN(APB_AON_BIT);                /* 使能AON时钟 */
    #endif

    if (AON->PMU_WKUP_CTRL.RC32K_TRIM_SEL == 0)
        value = (AON->BKHOLD_CTRL.Word >> 0) & 0x3FFF; // 0=AON_RC32K_MSB_TRIM_CFG_LSB  /* 从AON寄存器获取修调值 */
    else
        value = (APBMISC->RC32K_CALIB_ST.Word & 0xF/*MSB*/) | (((APBMISC->RC32K_CALIB_ST.Word >> 16) & 0x3FF) << 4/*LSB*/);  /* 从RCCALIB状态寄存器获取修调值 */

    return value;
}

/**
 * @brief 直接设置RC16M修调值
 * 
 * @param[in] value   修调值(0~63)
 * 
 * @details
 * - 使能APBMISC时钟
 * - 配置RC16M频率修调寄存器
 */
void rc16m_trim_set(uint8_t value)
{
    #if (ROM_UNUSED)
    // 时钟使能（在ROM中保持使能）
    // RCC->APBCLK_EN_RUN.APBMISC_CLKEN_RUN = 1;
    RCC_APBCLK_EN(APB_APBMISC_BIT);            /* 使能APBMISC时钟 */
    #endif

    APBMISC->RC16M_FREQ_TRIM = value;          /* 设置RC16M频率修调值 */
}

/**
 * @brief 获取当前RC16M使用的修调值
 * 
 * @return uint8_t 当前修调值(0~63)
 * 
 * @details
 * - 使能APBMISC时钟
 * - 读取RC16M频率修调寄存器
 */
uint8_t rc16m_trim_get(void)
{
    uint8_t value;

    #if (ROM_UNUSED)
    // RCC->APBCLK_EN_RUN.APBMISC_CLKEN_RUN = 1;
    RCC_APBCLK_EN(APB_APBMISC_BIT);            /* 使能APBMISC时钟 */
    #endif

    value = APBMISC->RC16M_FREQ_TRIM;          /* 读取RC16M频率修调值 */

    return value;
}

#if (ROM_UNUSED)
#include "utils.h"    // 调用co_abs()

/**
 * @brief 使用二分法启动RC16M校准
 * 
 * @return uint8_t RC16M校准后的修调值(0~63)
 * 
 * @details
 * - 使能RC16M计数时钟
 * - 使用二分法逐步逼近目标频率
 * - 根据窗口计数与目标值的差异调整修调值
 * - 返回最终校准的修调值
 */
uint8_t rc16m_calib(void)
{
    uint8_t trim_val = 0x20; // bit[5:0]  /* 初始修调值 */
    int win_cnt = 0;
    int win_set = APBMISC->RC16M_CNT_CTRL.RC16M_WIN_SET;  /* 获取目标窗口计数值 */

    RCC->CLK_EN_ST.RC16M_CNT_CLKEN = 1;        /* 使能RC16M计数时钟 */

    for (uint8_t step = 0x20; step > 0; step >>= 1)  /* 二分法步进 */
    {
        RCC->APBRST_CTRL.RC16M_CNT_RSTREQ = 1; /* 请求RC16M计数器复位 */
        RCC->APBRST_CTRL.RC16M_CNT_RSTREQ = 0;

        APBMISC->RC16M_FREQ_TRIM = trim_val;   /* 设置当前修调值 */
        APBMISC->RC16M_CNT_CTRL.RC16M_CNT_START = 1;  /* 启动RC16M计数 */

        while (!APBMISC->RC16M_CNT_CTRL.RC16M_CNT_DONE);  /* 等待计数完成 */
        win_cnt = APBMISC->RC16M_CNT_CTRL.RC16M_WIN_CNT;  /* 获取实际窗口计数值 */

        //if ((win_cnt > (win_set - 100)) && (win_cnt < (win_set + 100)))
        if (co_abs(win_cnt - win_set) <= 100)  /* 检查是否达到目标精度 */
            break;

        trim_val = trim_val + (step >> 1) - ((win_cnt > win_set) ? step : 0);  /* 根据计数结果调整修调值 */
    }

    //RCC->CLK_EN_ST.RC16M_CNT_CLKEN = 0;

    return trim_val;
}
#endif
