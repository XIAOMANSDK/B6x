/**
 ****************************************************************************************
 *
 * @file rcc.h
 *
 * @brief Header file - Reset and clock control(RCC) Driver
 *
 ****************************************************************************************
 */

#ifndef _RCC_H_
#define _RCC_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Clock Selection of Core System
enum sys_clk_sel
{
    SYS_CLK_16M            = 0, // XO16M
    SYS_CLK_32M            = 1, // DPLL64M/2
    SYS_CLK_48M            = 2, // DPLL48M
    SYS_CLK_64M            = 3, // DPLL64
    SYS_CLK_HSI            = 4, // RC16M
    SYS_CLK_LSI            = 5, // RC32K
};

/// Clock Frequency of SYS_CLK
enum sys_clk_freq
{
    SYS_FREQ_32K           = 32000,
    SYS_FREQ_16M           = 16000000,
    SYS_FREQ_32M           = 32000000,
    SYS_FREQ_48M           = 48000000,
    SYS_FREQ_64M           = 64000000,
};

/// Clock Selection of Flash Controlor
enum fsh_clk_sel
{
    FSH_CLK_HSI16          = 1, // RC16M
    FSH_CLK_HSE16          = 2, // XO16M
    FSH_CLK_DPLL           = 4, // DPLL
    FSH_CLK_DPLL128        = 8, // DPLL128M
    
    // prescale from DPLL128M
    FSH_CLK_DPSC64         = FSH_CLK_DPLL128 + 1, // psc2
    FSH_CLK_DPSC42         = FSH_CLK_DPLL128 + 2, // psc3
    FSH_CLK_DPSC32         = FSH_CLK_DPLL128 + 3, // psc4
    FSH_CLK_DPSC25         = FSH_CLK_DPLL128 + 4, // psc5
};

/// Clock Frequency of fSH_CLK, unit in MHz
enum fsh_clk_freq
{
    FSH_FREQ_16MHz         = 16, // RC16M or XO16M
    FSH_FREQ_48MHz         = 48, // DPLL
    FSH_FREQ_64MHz         = 64, // DPLL
    FSH_FREQ_PSC64         = 64,  // DPLL128 / psc2
    FSH_FREQ_PSC42         = 42,  // DPLL128 / psc3
    FSH_FREQ_PSC32         = 32,  // DPLL128 / psc4
    FSH_FREQ_PSC25         = 25,  // DPLL128 / psc5
};

/// BUS Operation Index
enum rcc_bus_idx
{
    AHB_RST_CTRL,
    APB_RST_CTRL,
    AHB_CLK_RUN,
    APB_CLK_RUN,
    AHB_CLK_SLEEP,
    APB_CLK_SLEEP,
    AHB_CLK_DEEPSL,
    APB_CLK_DEEPSL,
};

/// Bits Field of AHB-BUS
enum rcc_ahb_bfs
{
    AHB_RCC_BIT            = (1UL << 0),  // CLK
    AHB_SYSCFG_BIT         = (1UL << 1),  // CLK RST
    AHB_CSC_RST_BIT        = (1UL << 2),  // RST
    AHB_SPIM_BIT           = (1UL << 4),  // CLK RST
    AHB_SPIS_BIT           = (1UL << 5),  // CLK RST
    AHB_FSHC_BIT           = (1UL << 6),  // CLK RST
    AHB_ADC_BIT            = (1UL << 7),  // CLK RST
    AHB_GPIOA_RST_BIT      = (1UL << 8),  // RST
    AHB_AHB2P1_BIT         = (1UL << 9),  // CLK RST
    AHB_AHB2P2_BIT         = (1UL << 10), // CLK RST
    AHB_CACHE_BIT          = (1UL << 11), // CLK RST
    AHB_USB_BIT            = (1UL << 12), // CLK RST
};

/// Bits Field of APB-BUS
enum rcc_apb_bfs
{
    APB_BTMR_BIT           = (1UL << 0),  // CLK RST
    APB_CTMR_BIT           = (1UL << 1),  // CLK RST
    APB_ATMR_BIT           = (1UL << 2),  // CLK RST
    APB_UART1_BIT          = (1UL << 3),  // CLK RST
    APB_UART2_BIT          = (1UL << 4),  // CLK RST
    
    APB_DMAC_BIT           = (1UL << 6),  // CLK RST
    
    APB_EXTI_BIT           = (1UL << 8),  // CLK RST
    APB_I2C_BIT            = (1UL << 9),  // CLK RST
    APB_IWDT_BIT           = (1UL << 10), // CLK
    APB_MDM_BIT            = (1UL << 11), // CLK RST
    APB_RF_BIT             = (1UL << 12), // CLK RST
    APB_AON_BIT            = (1UL << 16), // CLK
    APB_APBMISC_BIT        = (1UL << 17), // CLK
    
    APB_PLLCAL_RST_BIT     = (1UL << 13), // RST
    APB_RCCALIB_RST_BIT    = (1UL << 14), // RST
    APB_RC16MCNT_RST_BIT   = (1UL << 15), // RST
    APB_BLELTMR_RST_BIT    = (1UL << 16), // RST
    APB_RTCSOFT_RST_BIT    = (1UL << 17), // RST
};


/*
 * MACROS DECLARATION
 ****************************************************************************************
 */

#include "reg_rcc.h"

#define RCC_AHBCLK_EN(bfs)     dowl( RCC->AHBCLK_EN_RUN.Word |= (bfs); )
#define RCC_APBCLK_EN(bfs)     dowl( RCC->APBCLK_EN_RUN.Word |= (bfs); )
#define RCC_AHBCLK_DIS(bfs)    dowl( RCC->AHBCLK_EN_RUN.Word &= ~(bfs); )
#define RCC_APBCLK_DIS(bfs)    dowl( RCC->APBCLK_EN_RUN.Word &= ~(bfs); )
#define RCC_AHBRST_REQ(bfs)    dowl( RCC->AHBRST_CTRL.Word |= (bfs);RCC->AHBRST_CTRL.Word &= ~(bfs); )
#define RCC_APBRST_REQ(bfs)    dowl( RCC->APBRST_CTRL.Word |= (bfs);RCC->APBRST_CTRL.Word &= ~(bfs); )


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Get BUS Register's value
 *
 * @param[in] idx  Index of register @see enum rcc_bus_idx
 *
 * @return value
 ****************************************************************************************
 */
uint32_t rcc_bus_get(uint8_t idx);

/**
 ****************************************************************************************
 * @brief Enable Bits Field of BUS Register 
 *
 * @param[in] idx  Index of register @see enum rcc_bus_idx
 * @param[in] bfs  Bits field of value @see enum rcc_ahb_bfs OR rcc_apb_bfs
 *
 ****************************************************************************************
 */
void rcc_bus_enb(uint8_t idx, uint32_t bfs);

/**
 ****************************************************************************************
 * @brief Disable Bits Field of BUS Register 
 *
 * @param[in] idx  Index of register @see enum rcc_bus_idx
 * @param[in] bfs  Bits field of value @see enum rcc_ahb_bfs OR rcc_apb_bfs
 *
 ****************************************************************************************
 */
void rcc_bus_dis(uint8_t idx, uint32_t bfs);

/**
 ****************************************************************************************
 * @brief Reset Bits Field of BUS Register (Combine rcc_bus_enb & rcc_bus_dis)
 *
 * @param[in] idx  Index of register @see enum rcc_bus_idx
 * @param[in] bfs  Bits field of value @see enum rcc_ahb_bfs OR rcc_apb_bfs
 *
 ****************************************************************************************
 */
void rcc_bus_rst(uint8_t idx, uint32_t bfs);


/**
 ****************************************************************************************
 * @brief Enable ADC Clock Configure
 *
 ****************************************************************************************
 */
void rcc_adc_en(void);

/**
 ****************************************************************************************
 * @brief Enable BLE Clock Configure
 *
 ****************************************************************************************
 */
void rcc_ble_en(void);

/**
 ****************************************************************************************
 * @brief Enable USB Clock/Pad Configure
 *
 ****************************************************************************************
 */
void rcc_usb_en(void);

/**
 ****************************************************************************************
 * @brief Set system clock selection
 *
 * @param[in] sys_clk  Index of Clock Selected @see enum sys_clk_sel
 * 
 ****************************************************************************************
 */
void rcc_sysclk_set(uint8_t sys_clk);

/**
 ****************************************************************************************
 * @brief Get system clock selection
 *
 * @return Clock Selected @see enum sys_clk_sel
 ****************************************************************************************
 */
uint8_t rcc_sysclk_get(void);

/**
 ****************************************************************************************
 * @brief Get clock frequency of core system, unit in Hz
 *
 * @return frequency @see enum sys_clk_freq
 ****************************************************************************************
 */
uint32_t rcc_sysclk_freq(void);

/**
 ****************************************************************************************
 * @brief Get flash controlor clock selection
 *
 * @return Clock Selected @see enum fsh_clk_sel
 ****************************************************************************************
 */
uint8_t rcc_fshclk_get(void);

/**
 ****************************************************************************************
 * @brief Get clock frequency of flash controlor, unit in MHz.
 *
 * @return freq in MHz @see enum fsh_clk_freq
 ****************************************************************************************
 */
uint8_t rcc_fshclk_mhz(void);

#if (ROM_UNUSED)
/**
 ****************************************************************************************
 * @brief Set flash controlor clock selection
 *
 * @param[in] fclk_sel  Index of Clock Selected @see enum fsh_clk_sel
 * 
 ****************************************************************************************
 */
void rcc_fshclk_set(uint8_t fsh_clk);
#endif


// Macro for Deprecated Functions {
#define rccGetSysClk()         rcc_sysclk_freq()
// }


#endif // _RCC_H_
