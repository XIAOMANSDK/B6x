/**
 ****************************************************************************************
 *
 * @file rcc.c
 *
 * @brief Reset and clock control(RCC) Driver
 *
 ****************************************************************************************
 */

#include "rcc.h"
#include "reg_rcc.h"
#include "reg_rf.h"
#include "reg_aon.h"
#include "reg_apbmisc.h"
#include "gpio.h"  // for USB pad
#include "reg_syscfg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Value of CFG.SYSCLK_SW
enum sysclk_sw
{
    SCLK_HSI = 1,
    SCLK_HSE = 2,
    SCLK_PLL = 4,
    SCLK_LSI = 8,
};

/// BLE_CLK(16M fixed) prescale from SYS_CLK_16M~64M --> psc0~3 same order
#define BLE_CLK_PSC(sclk)       (((sclk) <= SYS_CLK_64M) ? (sclk) : 0)    

/// Select DPLL_EN 64M or DPLL2_EN 48M
#define DPLL_IS_48M()           (RCC->CFG.DPLL_CLK_SW)


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

uint32_t rcc_bus_get(uint8_t idx)
{
    uint32_t reg_addr = RCC_BASE + RCC_AHBRST_CTRL_ADDR_OFFSET + (idx * 4);
    
    return (*(volatile uint32_t *)reg_addr);
}

void rcc_bus_enb(uint8_t idx, uint32_t bfs)
{
    uint32_t reg_addr = RCC_BASE + RCC_AHBRST_CTRL_ADDR_OFFSET + (idx * 4);
    
    (*(volatile uint32_t *)reg_addr) |= bfs;
}

void rcc_bus_dis(uint8_t idx, uint32_t bfs)
{
    uint32_t reg_addr = RCC_BASE + RCC_AHBRST_CTRL_ADDR_OFFSET + (idx * 4);
    
    (*(volatile uint32_t *)reg_addr) &= ~bfs;
}

void rcc_bus_rst(uint8_t idx, uint32_t bfs)
{
    uint32_t reg_addr = RCC_BASE + RCC_AHBRST_CTRL_ADDR_OFFSET + (idx * 4);
    
    (*(volatile uint32_t *)reg_addr) |= bfs;
    (*(volatile uint32_t *)reg_addr) &= ~bfs;
}

void rcc_adc_en(void)
{
    // ADC reset: clk=0, rst=1, rst=0, clk=1
    RCC->AHBCLK_EN_RUN.ADC_CLKEN_RUN     = 0;
    RCC->AHBRST_CTRL.ADC_RSTREQ          = 1;
    RCC->AHBRST_CTRL.ADC_RSTREQ          = 0;
    RCC->AHBCLK_EN_RUN.ADC_CLKEN_RUN     = 1;
}

void rcc_ble_en(void)
{
    // modem&rf clk enable
    //RCC->APBCLK_EN_RUN.Word |= (APB_MDM_BIT | APB_RF_BIT);
    RCC_APBCLK_EN(APB_MDM_BIT | APB_RF_BIT);
    
    // ble clk enable
    RCC->BLE_CLKRST_CTRL.BLE_AHBEN = 1;
    
    // ble reg reset
    RCC->BLE_CLKRST_CTRL.BLE_AHB_RSTREQ = 1;
    RCC->BLE_CLKRST_CTRL.BLE_AHB_RSTREQ = 0;
    
    #if (EM_PWOFF)
    // em power enable
    AON->PWOFF_CTRL.EM_PD_EN = 0;
    AON->PWOFF_CTRL.EM_ACCESS_EN = 1;
    // wait em power stable
    while (AON->PWOFF_CTRL.EM_PD_ACK);
    #endif
    
    // ble signal latch release - move to ble_init() / ble_resume()
    //AON->PMU_WKUP_CTRL.BLE_LATCH_N = 1;
    // ble low power source select lsi
    AON->BLE_RTC_CTL.BLELOWCLK_SEL = 0;
    // ble low power enable
    AON->BLE_RTC_CTL.BLELOWCLK_EN  = 1;
}

void rcc_usb_en(void)
{
    // clk rst enable
    RCC_AHBCLK_EN(AHB_USB_BIT);
    RCC_AHBRST_REQ(AHB_USB_BIT);
    
    // iopad to hiz
    GPIO_DIR_CLR((1UL << PA_USB_DP) | (1UL << PA_USB_DM));
    iom_ctrl(PA_USB_DP, IOM_HIZ);
    iom_ctrl(PA_USB_DM, IOM_HIZ);
    
    // 1.5k pull-up(.USB_PHY_MOD=0, .DIG_USB_PU=3, .DIG_USB_RXEN=1,)
    //SYSCFG->USB_CTRL.DIG_USB_PU   = 3; // FIB result 1 --> 3
    //SYSCFG->USB_CTRL.DIG_USB_RXEN = 1;
    SYSCFG->USB_CTRL.Word = 0x1C;
}

void rcc_sysclk_set(uint8_t sclk_sel)
{
    uint8_t clk_sw  = RCC->CFG.SYSCLK_SW; // curr sclk
    uint8_t clk_div = 0; // default RCC->AHBCLK_DIV = 0
    uint8_t ble_div = 0; // default RCC->BLE_CLKRST_CTRL.BLECLK_DIV = 0

    // sysclk switch, bypass if already*
    switch (sclk_sel)
    {
        case SYS_CLK_16M:
        {
            if (clk_sw == SCLK_HSE) return;
            
            clk_sw = SCLK_HSE;
            // use xo16m_en() as clock src 
            //AON->PMU_CTRL.OSC_EN_RUN          = 1;
            //APBMISC->XOSC16M_CTRL.EN_XO16MBUF = 1;
            RCC->CLK_EN_ST.HSE_DIG_EN = 1;
        } break;
        
        case SYS_CLK_HSI:
        {
            if (clk_sw == SCLK_HSI) return;

            clk_sw = SCLK_HSI;
            // use rc16m as clock src
            //APBMISC->AON_PMU_CTRL.HSI_EN_RUN = 1;
            RCC->CLK_EN_ST.HSI_DIG_EN = 1;
        } break;
        
        case SYS_CLK_LSI:
        {
            if (clk_sw == SCLK_LSI) return;
            
            clk_sw = SCLK_LSI;
            // use rc32k as clock src
            AON->PMU_CTRL.RC32K_EN_RUN = 1;
            RCC->CLK_EN_ST.LSI_DIG_EN  = 1;
        } break;

        default:
        {
            clk_sw = SCLK_PLL;
            ble_div = (sclk_sel - SYS_CLK_16M);
            
            // use dpll as clock src
            if (sclk_sel == SYS_CLK_48M)
            {
                RCC->CFG.DPLL_CLK_SW        = 1; // 48M
            }
            else
            {
                clk_div = (sclk_sel == SYS_CLK_32M) ? 1 : 0;
                RCC->CFG.DPLL_CLK_SW        = 0; // 64M
            }

            APBMISC->DPLL_CTRL.DPLL2_EN     = 1;
            RCC->CLK_EN_ST.DPLL_DIG_EN      = 1;
        } break;
    }
    
    RCC->BLE_CLKRST_CTRL.BLECLK_DIV = ble_div; //BLE Must 16M
    RCC->AHBCLK_DIV    = clk_div;
    RCC->CFG.SYSCLK_SW = clk_sw;
}

uint8_t rcc_sysclk_get(void)
{
    uint8_t sclk_cur; 
    uint8_t clk_sw = RCC->CFG.SYSCLK_SW;
    
    if (clk_sw == SCLK_HSE)
    {
        sclk_cur = SYS_CLK_16M;
    }
    else if (clk_sw == SCLK_PLL)
    {
        sclk_cur = DPLL_IS_48M() ? SYS_CLK_48M : ((RCC->AHBCLK_DIV == 1) ? SYS_CLK_32M : SYS_CLK_64M);
    }
    else if (clk_sw == SCLK_LSI)
    {
        sclk_cur = SYS_CLK_LSI;
    }
    else /*SCLK_HSI*/
    {
        sclk_cur = SYS_CLK_HSI;
    }
    
    return sclk_cur;
}

uint32_t rcc_sysclk_freq(void)
{
    uint32_t clk_freq;
    uint8_t clk_sw = RCC->CFG.SYSCLK_SW;
    
    if (clk_sw == SCLK_PLL)
    {
        clk_freq = DPLL_IS_48M() ? SYS_FREQ_48M : SYS_FREQ_64M;
    }
    else if (clk_sw == SCLK_LSI)
    {
        clk_freq = SYS_FREQ_32K;
    }
    else // SCLK_HSE SCLK_HSI
    {
        clk_freq = SYS_FREQ_16M;
    }
    
    // prescale clock
    uint8_t clk_div = (RCC->AHBCLK_DIV & 0xFF);
    if (clk_div > 0)
    {
        clk_freq /= (clk_div + 1);
    }

    return clk_freq;
}

uint8_t rcc_fshclk_get(void)
{
    uint8_t fclk_sel = RCC->CLK_EN_ST.FSHCCLK_SEL;
    
    if (fclk_sel == FSH_CLK_DPLL128)
    {
        fclk_sel += (RCC->CLK_EN_ST.FSHCCLK_DIV_NUM);
    }
    
    return fclk_sel;
}

uint8_t rcc_fshclk_mhz(void)
{
    uint8_t fclk_mhz;
    uint8_t fclk_sel = RCC->CLK_EN_ST.FSHCCLK_SEL;
    
    if (fclk_sel == FSH_CLK_DPLL128)
    {
        fclk_mhz = 128 / (RCC->CLK_EN_ST.FSHCCLK_DIV_NUM + 1); // FSH_FREQ_PSCxx
    }
    else if (fclk_sel == FSH_CLK_DPLL)
    {
        fclk_mhz = (DPLL_IS_48M() ? FSH_FREQ_48MHz : FSH_FREQ_64MHz);
    }
    else // FSH_CLK_HSE16 FSH_CLK_HSI16
    {
        fclk_mhz = FSH_FREQ_16MHz;
    }
    
    return fclk_mhz;
}

#if (ROM_UNUSED)
void rcc_fshclk_set(uint8_t fclk_sel)
{
    if (fclk_sel >= FSH_CLK_DPLL128)
    {
        // note fshcclk_div_num must be larger than 0, so defalut 2(psc3=42M)
        uint8_t clk_div = (fclk_sel > FSH_CLK_DPLL128) ? (fclk_sel - FSH_CLK_DPLL128) : 2;
        
        RCC->CLK_EN_ST.FSHCCLK_SEL     = FSH_CLK_HSI16;
        
        RCC->CLK_EN_ST.FSHCCLK_DIVEN   = 0;
        RCC->CLK_EN_ST.FSHCCLK_DIV_NUM = clk_div;
        // fshcclk_diven must be set as 1 before fshcclk_sel
        RCC->CLK_EN_ST.FSHCCLK_DIVEN   = 1;

        RCC->CLK_EN_ST.FSHCCLK_SEL     = FSH_CLK_DPLL128;
    }
    else
    {
        RCC->CLK_EN_ST.FSHCCLK_SEL     = fclk_sel;
    }
}
#endif
