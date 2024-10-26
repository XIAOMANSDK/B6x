/**
 ****************************************************************************************
 *
 * @file rco.c
 *
 * @brief RCO(32k, 16M)  Driver
 *
 ****************************************************************************************
 */

#include "rco.h"
#include "reg_rcc.h"
#include "reg_aon.h"
#include "reg_apbmisc.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define HSE_CALIB_MULTI        500  // 16M/32K
#define DPLL_CAKLIB_MULTI      2000 // 64M/32K
#define DPLL48M_CAKLIB_MULTI   1500 // 48M/32K


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Enable and Config rc32k
 *
 * @param[in] ref_clk   reference clock for calibration @see enum rc32k_ref_clk
 * @param[in] cal_ctrl  config control for calibration @see enum rc32k_cal_ctrl
 *
 ****************************************************************************************
 */
void rc32k_conf(uint8_t ref_clk, uint8_t cal_ctrl)
{
    uint16_t target = 0;               // bit[15:0]
    uint8_t cycle = (cal_ctrl & 0x1F); // bit[20:16]

    #if (ROM_UNUSED)
    // clock enable (keep enabled in rom)
    // RCC->APBCLK_EN_RUN.AON_CLKEN_RUN     = 1;
    // RCC->APBCLK_EN_RUN.APBMISC_CLKEN_RUN = 1;
    RCC_APBCLK_EN(APB_APBMISC_BIT | APB_AON_BIT);
    #endif

    RCC->CLK_EN_ST.RCCALIB_CLKEN  = 1;
    RCC->CLK_EN_ST.RCCALIB_CLKSEL = ref_clk;

    if (ref_clk == RCLK_HSE/*0*/)
    {
        target = HSE_CALIB_MULTI * (cycle + 1);
    }
    else //if ((ref_clk == RCLK_DPLL64/*1*/) || (ref_clk == RCLK_DPLL128/*2*/))
    {
        uint16_t multi = ((RCC->CFG.DPLL_CLK_SW == 0) || (ref_clk == RCLK_DPLL128) ? DPLL_CAKLIB_MULTI : DPLL48M_CAKLIB_MULTI);
        target =  multi* ref_clk * (cycle + 1);
    }

    // bit[15:0]--rccalib_target, bit[20:16]--cycle, bit21--SCAL_EN, bit22--DLY
    APBMISC->RCCALIB_CTRL.Word = target | ((uint32_t)cal_ctrl << APBMISC_RCCALIB_CYCLES_LSB) | (1UL << APBMISC_RCCALIB_DLY_POS);
}

/**
 ****************************************************************************************
 * @brief Start rc32k calibration, after rc32k_conf().
 *
 * @return rc32k trim value
 ****************************************************************************************
 */
uint16_t rc32k_calib(void)
{
    uint16_t trim_val = 0;

    // use rc calib result control rc32k
    AON->PMU_WKUP_CTRL.RC32K_TRIM_SEL = 1;

    // rc calib start
    APBMISC->RCCALIB_START = 1;

    // wait done, thenclear
    while(!APBMISC->RCCALIB_STCLR.RCCALIB_DONE);
    APBMISC->RCCALIB_STCLR.RCCALIB_DONE_CLR = 1;

    // rc32k calib trim val get
    //AON->BKHOLD_CTRL.RC32K_MSB_TRIM_CFG = APBMISC->RC32K_CALIB_ST.RC32K_MSB_CALIB;
    //AON->BKHOLD_CTRL.RC32K_LSB_TRIM_CFG = APBMISC->RC32K_CALIB_ST.RC32K_LSB_CALIB;
    //uint16_t value = APBMISC->RC32K_CALIB_ST.RC32K_MSB_CALIB | (APBMISC->RC32K_CALIB_ST.RC32K_LSB_CALIB << 4);
    trim_val = (APBMISC->RC32K_CALIB_ST.Word & 0xF/*MSB*/) | (((APBMISC->RC32K_CALIB_ST.Word >> 16) & 0x3FF) << 4/*LSB*/);

    AON->BKHOLD_CTRL.Word = (AON->BKHOLD_CTRL.Word & ~(0x3FFF << 0)) | (((uint32_t)trim_val & 0x3FFF) << 0);

    // use AON's trim val control rc32k
    AON->PMU_WKUP_CTRL.RC32K_TRIM_SEL = 0;

    return trim_val;
}

/**
 ****************************************************************************************
 * @brief Direct Set trim value to calibrate rc32k.
 *
 * @param[in] value   calib-target value
 *
 ****************************************************************************************
 */
void rc32k_trim_set(uint16_t value)
{
    #if (ROM_UNUSED)
    // clock enable (keep enabled in rom)
    // RCC->APBCLK_EN_RUN.AON_CLKEN_RUN = 1;
    RCC_APBCLK_EN(APB_AON_BIT);
    #endif

    //AON->BKHOLD_CTRL.RC32K_MSB_TRIM_CFG = value & 0xF;          // bit[3:0]
    //AON->BKHOLD_CTRL.RC32K_LSB_TRIM_CFG = (value >> 4) & 0x3FF; // bit[13:4]
    AON->BKHOLD_CTRL.Word = (AON->BKHOLD_CTRL.Word & ~(0x3FFF << 0)) | (((uint32_t)value & 0x3FFF) << 0);

    // use AON's trim val control rc32k
    AON->PMU_WKUP_CTRL.RC32K_TRIM_SEL = 0;
}

/**
 ****************************************************************************************
 * @brief Get current value of rc32k calibrated or used.
 *
 * @return calib-target value
 *
 ****************************************************************************************
 */
uint16_t rc32k_trim_get(void)
{
    uint16_t value;

    #if (ROM_UNUSED)
    // RCC->APBCLK_EN_RUN.AON_CLKEN_RUN = 1;
    RCC_APBCLK_EN(APB_AON_BIT);
    #endif

    if (AON->PMU_WKUP_CTRL.RC32K_TRIM_SEL == 0)
        value = (AON->BKHOLD_CTRL.Word >> 0) & 0x3FFF; // 0=AON_RC32K_MSB_TRIM_CFG_LSB
    else
        value = (APBMISC->RC32K_CALIB_ST.Word & 0xF/*MSB*/) | (((APBMISC->RC32K_CALIB_ST.Word >> 16) & 0x3FF) << 4/*LSB*/);

    return value;
}

/**
 ****************************************************************************************
 * @brief Direct Set trim value to calibrate rc16m.
 *
 * @param[in] value   calib-target value 0~63
 *
 ****************************************************************************************
 */
void rc16m_trim_set(uint8_t value)
{
    #if (ROM_UNUSED)
    // clock enable (keep enabled in rom)
    // RCC->APBCLK_EN_RUN.APBMISC_CLKEN_RUN = 1;
    RCC_APBCLK_EN(APB_APBMISC_BIT);
    #endif

    APBMISC->RC16M_FREQ_TRIM = value;
}

/**
 ****************************************************************************************
 * @brief Get current value of rc16m calibrated or used.
 *
 * @return calib-target value 0~63
 *
 ****************************************************************************************
 */
uint8_t rc16m_trim_get(void)
{
    uint8_t value;

    #if (ROM_UNUSED)
    // RCC->APBCLK_EN_RUN.APBMISC_CLKEN_RUN = 1;
    RCC_APBCLK_EN(APB_APBMISC_BIT);
    #endif

    value = APBMISC->RC16M_FREQ_TRIM;

    return value;
}

#if (ROM_UNUSED)
#include "utils.h"    // call co_abs()

/**
 ****************************************************************************************
 * @brief Start rc16m calibration in dichotomy.
 *
 * @return rc16m trim value (0~63)
 ****************************************************************************************
 */
uint8_t rc16m_calib(void)
{
    uint8_t trim_val = 0x20; // bit[5:0]
    int win_cnt = 0;
    int win_set = APBMISC->RC16M_CNT_CTRL.RC16M_WIN_SET;

    RCC->CLK_EN_ST.RC16M_CNT_CLKEN = 1;

    for (uint8_t step = 0x20; step > 0; step >>= 1)
    {
        RCC->APBRST_CTRL.RC16M_CNT_RSTREQ = 1;
        RCC->APBRST_CTRL.RC16M_CNT_RSTREQ = 0;

        APBMISC->RC16M_FREQ_TRIM = trim_val;
        APBMISC->RC16M_CNT_CTRL.RC16M_CNT_START = 1;

        while (!APBMISC->RC16M_CNT_CTRL.RC16M_CNT_DONE);
        win_cnt = APBMISC->RC16M_CNT_CTRL.RC16M_WIN_CNT;

        //if ((win_cnt > (win_set - 100)) && (win_cnt < (win_set + 100)))
        if (co_abs(win_cnt - win_set) <= 100)
            break;

        trim_val = trim_val + (step >> 1) - ((win_cnt > win_set) ? step : 0);
    }

    //RCC->CLK_EN_ST.RC16M_CNT_CLKEN = 0;

    return trim_val;
}
#endif
