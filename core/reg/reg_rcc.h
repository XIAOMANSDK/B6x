#ifndef _REG_RCC_H_
#define _REG_RCC_H_

#include "reg_base.h" 

//================================
//BLOCK RCC define 

#define RCC_BASE                               ((uint32_t)0x40000000) 
#define RCC_CLK_EN_ST_ADDR_OFFSET              0x000 
#define RCC_AHBCLK_DIV_ADDR_OFFSET             0x004 
#define RCC_APB1CLK_DIV_ADDR_OFFSET            0x008 
#define RCC_APB2CLK_DIV_ADDR_OFFSET            0x00c 
#define RCC_CFG_ADDR_OFFSET                    0x010 
#define RCC_STCALIB_ADDR_OFFSET                0x014 
#define RCC_HSE_DET_CTRL_ADDR_OFFSET           0x018 
#define RCC_CHIP_RSTREQ_ADDR_OFFSET            0x01c 
#define RCC_AHBRST_CTRL_ADDR_OFFSET            0x020 
#define RCC_APBRST_CTRL_ADDR_OFFSET            0x024 
#define RCC_AHBCLK_EN_RUN_ADDR_OFFSET          0x028 
#define RCC_APBCLK_EN_RUN_ADDR_OFFSET          0x02c 
#define RCC_AHBCLK_EN_SLEEP_ADDR_OFFSET        0x030 
#define RCC_APBCLK_EN_SLEEP_ADDR_OFFSET        0x034 
#define RCC_AHBCLK_EN_DPSLEEP_ADDR_OFFSET      0x038 
#define RCC_APBCLK_EN_DPSLEEP_ADDR_OFFSET      0x03c 
#define RCC_BLE_CLKRST_CTRL_ADDR_OFFSET        0x040 
#define RCC_CHIP_RST_ST_CLR_ADDR_OFFSET        0x044 

//================================
//BLOCK RCC reg struct define 
typedef union //0x000 
{
    struct
    {
        uint32_t HSE_DIG_EN:                         1; // bit0
        uint32_t LSI_DIG_EN:                         1; // bit1
        uint32_t HSI_DIG_EN:                         1; // bit2
        uint32_t DPLL_DIG_EN:                        1; // bit3
        uint32_t CSS_EN:                             1; // bit4 --- HSE stable check enable
        uint32_t RCCALIB_CLKEN:                      1; // bit5
        uint32_t BIST_CLK_EN:                        1; // bit6
        uint32_t RC16M_CNT_CLKEN:                    1; // bit7
        uint32_t FSHCCLK_SEL:                        4; // bit[11:8] --- fshc clock source select
                                                        // 4'b0001: HSI clock 
                                                        // 4'b0010: HSE clock
                                                        // 4'b0100: DPLL clock
                                                        // 4'b1000: DPLL 128M div out
                                                        // note: if fshc clock select DPLL 42M, 
                                                        // fshcclk_diven must be set as 1 before fshcclk_sel
                                                        // config as 4'b1000
        uint32_t RCCALIB_CLKSEL:                     2; // bit[13:12] 
        uint32_t FSHCCLK_DIVEN:                      1; // bit14     --- 1: 128M divider for fshc enable
        uint32_t FSHCCLK_DIV_NUM:                    3; // bit[17:15]--- note fshcclk_div_num must be larger than 0
                                                        // div clock = dpll_128m/(fshcclk_div_num + 1)
        uint32_t IWDT_FCLK_DIS_DP:                   1; // bit18 --- disable the IWDT function clock (RC32K clock) when chip is in DeepSleep
                                                        // Note : the IWDT function clock is auto enable, when chip is exit out DeepSleep
        uint32_t CSS_FLAG:                           1; // bit19
        uint32_t HSI_RDY:                            1; // bit20
        uint32_t RSV_NOUSE1:                         1; // bit21 --- Ignore me
        uint32_t LSI_RDY:                            1; // bit22
        uint32_t HSE_RDY:                            1; // bit23
        uint32_t RSV_END:                            8; // bit[31:24]
    };
    uint32_t Word;
} RCC_CLK_EN_ST_TypeDef; //0x000 


//================================
#define RCC_HSE_DIG_EN_POS                  0
#define RCC_LSI_DIG_EN_POS                  1
#define RCC_HSI_DIG_EN_POS                  2
#define RCC_DPLL_DIG_EN_POS                 3
#define RCC_CSS_EN_POS                      4
#define RCC_RCCALIB_CLKEN_POS               5
#define RCC_BIST_CLK_EN_POS                 6
#define RCC_RC16M_CNT_CLKEN_POS             7
#define RCC_FSHCCLK_SEL_LSB                 8
#define RCC_FSHCCLK_SEL_WIDTH               4
#define RCC_RCCALIB_CLKSEL_LSB              12
#define RCC_RCCALIB_CLKSEL_WIDTH            2
#define RCC_FSHCCLK_DIVEN_POS               14
#define RCC_FSHCCLK_DIV_NUM_LSB             15
#define RCC_FSHCCLK_DIV_NUM_WIDTH           3
#define RCC_IWDT_FCLK_DIS_DP_POS            18
#define RCC_CSS_FLAG_POS                    19
#define RCC_HSI_RDY_POS                     20
#define RCC_LSI_RDY_POS                     22
#define RCC_HSE_RDY_POS                     23
//================================

typedef union //0x010 
{
    struct
    {
        uint32_t SYSCLK_SW:                          4; // bit[3:0] --- system clock source select
                                                        // 1: HSI clock
                                                        // 2: HSE clock
                                                        // 4: PLL 64M clock
                                                        // 8: LSI clock 
        uint32_t MCO_SW:                             4; // bit[7:4] ---
                                                        // 0: no output 
                                                        // 1: hsi clock
                                                        // 2: hse clock
                                                        // 3: PLL clock
                                                        // 4: lsi clock
                                                        // 5: system clock
                                                        // 6: ahb clock
                                                        // 7: apb clock    
                                                        // 8: fshc clock
        uint32_t DPLL_CLK_SW:                        1; // bit8 ---
                                                        // 1'b0: DPLL clock is use 64M
                                                        // 1'b1: DPLL clock is use 48M
        uint32_t RSV_END:                           23; // bit[31:9]
    };
    uint32_t Word;
} RCC_CFG_TypeDef; //0x010 


//================================
#define RCC_SYSCLK_SW_LSB                   0
#define RCC_SYSCLK_SW_WIDTH                 4
#define RCC_MCO_SW_LSB                      4
#define RCC_MCO_SW_WIDTH                    4
#define RCC_DPLL_CLK_SW_POS                 8
//================================

typedef union //0x018 
{
    struct
    {
        uint32_t CLK_DET_EN:                         1; // bit0     --- 1: HSE clock detect enable
        uint32_t CLK_DET_RSTREQ:                     1; // bit1     --- 1: reset the HSE clock detect module
        uint32_t RSV_NOUSE1:                         6; // bit[7:2] --- Ignore me
        uint32_t CLK_DET_PASS:                       1; // bit8
        uint32_t CLK_DET_FAIL:                       1; // bit9
        uint32_t RSV_END:                           22; // bit[31:10]
    };
    uint32_t Word;
} RCC_HSE_DET_CTRL_TypeDef; //0x018 


//================================
#define RCC_CLK_DET_EN_POS                  0
#define RCC_CLK_DET_RSTREQ_POS              1
#define RCC_CLK_DET_PASS_POS                8
#define RCC_CLK_DET_FAIL_POS                9
//================================

typedef union //0x020 
{
    struct
    {
        uint32_t RSV_NOUSE1:                         1; // bit0 --- Ignore me
        uint32_t SYSCFG_RSTREQ:                      1; // bit1
        uint32_t CSC_RSTREQ:                         1; // bit2
        uint32_t RSV_NOUSE2:                         1; // bit3 --- Ignore me
        uint32_t SPIM_RSTREQ:                        1; // bit4
        uint32_t SPIS_RSTREQ:                        1; // bit5
        uint32_t FSHC_RSTREQ:                        1; // bit6
        uint32_t ADC_RSTREQ:                         1; // bit7
        uint32_t GPIOA_RSTREQ:                       1; // bit8
        uint32_t AHB2P1_RSTREQ:                      1; // bit9
        uint32_t AHB2P2_RSTREQ:                      1; // bit10
        uint32_t CACHE_RSTREQ:                       1; // bit11
        uint32_t USB_RSTREQ:                         1; // bit12 ---use for ahb2apb bridge
        uint32_t RSV_END:                           19; // bit[31:13]
    };
    uint32_t Word;
} RCC_AHBRST_CTRL_TypeDef; //0x020 


//================================
#define RCC_SYSCFG_RSTREQ_POS               1
#define RCC_CSC_RSTREQ_POS                  2
#define RCC_SPIM_RSTREQ_POS                 4
#define RCC_SPIS_RSTREQ_POS                 5
#define RCC_FSHC_RSTREQ_POS                 6
#define RCC_ADC_RSTREQ_POS                  7
#define RCC_GPIOA_RSTREQ_POS                8
#define RCC_AHB2P1_RSTREQ_POS               9
#define RCC_AHB2P2_RSTREQ_POS               10
#define RCC_CACHE_RSTREQ_POS                11
#define RCC_USB_RSTREQ_POS                  12
//================================

typedef union //0x024 
{
    struct
    {
        uint32_t BTMR_RSTREQ:                        1; // bit0
        uint32_t CTMR_RSTREQ:                        1; // bit1
        uint32_t ATMR_RSTREQ:                        1; // bit2
        uint32_t UART1_RSTREQ:                       1; // bit3
        uint32_t UART2_RSTREQ:                       1; // bit4
        uint32_t RSV_NOUSE1:                         1; // bit5 --- Ignore me
        uint32_t DMAC_RSTREQ:                        1; // bit6
        uint32_t RSV_NOUSE2:                         1; // bit7 --- Ignore me
        uint32_t EXTI_RSTREQ:                        1; // bit8
        uint32_t I2C_RSTREQ:                         1; // bit9
        uint32_t RSV_NOUSE3:                         1; // bit10 ---Ignore me
        uint32_t MDM_RSTREQ:                         1; // bit11
        uint32_t RF_RSTREQ:                          1; // bit12
        uint32_t PLL_CAL_RSTREQ:                     1; // bit13
        uint32_t RCCALIB_RSTREQ:                     1; // bit14
        uint32_t RC16M_CNT_RSTREQ:                   1; // bit15
        uint32_t BLE_LOW_CLK_RSTREQ:                 1; // bit16
        uint32_t RTC_RST_REQ:                        1; // bit17 ---
                                                        // 1:soft reset RTC
        uint32_t RSV_END:                           14; // bit[31:18]
    };
    uint32_t Word;
} RCC_APBRST_CTRL_TypeDef; //0x024 


//================================
#define RCC_BTMR_RSTREQ_POS                 0
#define RCC_CTMR_RSTREQ_POS                 1
#define RCC_ATMR_RSTREQ_POS                 2
#define RCC_UART1_RSTREQ_POS                3
#define RCC_UART2_RSTREQ_POS                4
#define RCC_DMAC_RSTREQ_POS                 6
#define RCC_EXTI_RSTREQ_POS                 8
#define RCC_I2C_RSTREQ_POS                  9
#define RCC_MDM_RSTREQ_POS                  11
#define RCC_RF_RSTREQ_POS                   12
#define RCC_PLL_CAL_RSTREQ_POS              13
#define RCC_RCCALIB_RSTREQ_POS              14
#define RCC_RC16M_CNT_RSTREQ_POS            15
#define RCC_BLE_LOW_CLK_RSTREQ_POS          16
#define RCC_RTC_RST_REQ_POS                 17
//================================

typedef union //0x028 
{
    struct
    {
        uint32_t RCC_CLKEN_RUN:                      1; // bit0
        uint32_t SYSCFG_CLKEN_RUN:                   1; // bit1
        uint32_t RSV_NOUSE1:                         2; // bit[3:2] --- Ignore me
        uint32_t SPIM_CLKEN_RUN:                     1; // bit4
        uint32_t SPIS_CLKEN_RUN:                     1; // bit5
        uint32_t FSHC_CLKEN_RUN:                     1; // bit6
        uint32_t ADC_CLKEN_RUN:                      1; // bit7
        uint32_t RSV_NOUSE2:                         1; // bit8     --- Ignore me
        uint32_t AHB2P1_CLKEN_RUN:                   1; // bit9     --- use for ahb2apb1 bridge
        uint32_t AHB2P2_CLKEN_RUN:                   1; // bit10    --- use for ahb2apb2 bridge
        uint32_t CACHE_CLKEN_RUN:                    1; // bit11
        uint32_t USB_CLKEN_RUN:                      1; // bit12
        uint32_t RSV_END:                           19; // bit[31:13]
    };
    uint32_t Word;
} RCC_AHBCLK_EN_RUN_TypeDef; //0x028 


//================================
#define RCC_RCC_CLKEN_RUN_POS               0
#define RCC_SYSCFG_CLKEN_RUN_POS            1
#define RCC_SPIM_CLKEN_RUN_POS              4
#define RCC_SPIS_CLKEN_RUN_POS              5
#define RCC_FSHC_CLKEN_RUN_POS              6
#define RCC_ADC_CLKEN_RUN_POS               7
#define RCC_AHB2P1_CLKEN_RUN_POS            9
#define RCC_AHB2P2_CLKEN_RUN_POS            10
#define RCC_CACHE_CLKEN_RUN_POS             11
#define RCC_USB_CLKEN_RUN_POS               12
//================================

typedef union //0x02c 
{
    struct
    {
        uint32_t BTMR_CLKEN_RUN:                     1; // bit0
        uint32_t CTMR_CLKEN_RUN:                     1; // bit1
        uint32_t ATMR_CLKEN_RUN:                     1; // bit2
        uint32_t UART1_CLKEN_RUN:                    1; // bit3
        uint32_t UART2_CLKEN_RUN:                    1; // bit4
        uint32_t RSV_NOUSE1:                         1; // bit5       --- Ignore me
        uint32_t DMAC_CLKEN_RUN:                     1; // bit6
        uint32_t RSV_NOUSE2:                         1; // bit7       --- Ignore me
        uint32_t EXTI_CLKEN_RUN:                     1; // bit8
        uint32_t I2C_CLKEN_RUN:                      1; // bit9
        uint32_t IWDT_CLKEN_RUN:                     1; // bit10
        uint32_t MDM_CLKEN_RUN:                      1; // bit11
        uint32_t RF_CLKEN_RUN:                       1; // bit12
        uint32_t RSV_NOUSE3:                         3; // bit[15:13] --- Ignore me
        uint32_t AON_CLKEN_RUN:                      1; // bit16
        uint32_t APBMISC_CLKEN_RUN:                  1; // bit17
        uint32_t RSV_END:                           14; // bit[31:18]
    };
    uint32_t Word;
} RCC_APBCLK_EN_RUN_TypeDef; //0x02c 


//================================
#define RCC_BTMR_CLKEN_RUN_POS              0
#define RCC_CTMR_CLKEN_RUN_POS              1
#define RCC_ATMR_CLKEN_RUN_POS              2
#define RCC_UART1_CLKEN_RUN_POS             3
#define RCC_UART2_CLKEN_RUN_POS             4
#define RCC_DMAC_CLKEN_RUN_POS              6
#define RCC_EXTI_CLKEN_RUN_POS              8
#define RCC_I2C_CLKEN_RUN_POS               9
#define RCC_IWDT_CLKEN_RUN_POS              10
#define RCC_MDM_CLKEN_RUN_POS               11
#define RCC_RF_CLKEN_RUN_POS                12
#define RCC_AON_CLKEN_RUN_POS               16
#define RCC_APBMISC_CLKEN_RUN_POS           17
//================================

typedef union //0x030 
{
    struct
    {
        uint32_t RCC_CLKEN_SLEEP:                    1; // bit0
        uint32_t SYSCFG_CLKEN_SLEEP:                 1; // bit1
        uint32_t RSV_NOUSE1:                         2; // bit[3:2] --- Ignore me
        uint32_t SPIM_CLKEN_SLEEP:                   1; // bit4
        uint32_t SPIS_CLKEN_SLEEP:                   1; // bit5
        uint32_t FSHC_CLKEN_SLEEP:                   1; // bit6
        uint32_t ADC_CLKEN_SLEEP:                    1; // bit7
        uint32_t RSV_NOUSE2:                         1; // bit8     --- Ignore me
        uint32_t AHB2P1_CLKEN_SLEEP:                 1; // bit9     --- use for ahb2apb1 bridge
        uint32_t AHB2P2_CLKEN_SLEEP:                 1; // bit10    --- use for ahb2apb2 bridge
        uint32_t CACHE_CLKEN_SLEEP:                  1; // bit11
        uint32_t USB_CLKEN_SLEEP:                    1; // bit12
        uint32_t RSV_END:                           19; // bit[31:13]
    };
  uint32_t Word;
} RCC_AHBCLK_EN_SLEEP_TypeDef; //0x030 


//================================
#define RCC_RCC_CLKEN_SLEEP_POS             0
#define RCC_SYSCFG_CLKEN_SLEEP_POS          1
#define RCC_SPIM_CLKEN_SLEEP_POS            4
#define RCC_SPIS_CLKEN_SLEEP_POS            5
#define RCC_FSHC_CLKEN_SLEEP_POS            6
#define RCC_ADC_CLKEN_SLEEP_POS             7
#define RCC_AHB2P1_CLKEN_SLEEP_POS          9
#define RCC_AHB2P2_CLKEN_SLEEP_POS          10
#define RCC_CACHE_CLKEN_SLEEP_POS           11
#define RCC_USB_CLKEN_SLEEP_POS             12
//================================

typedef union //0x034 
{
    struct
    {
        uint32_t BTMR_CLKEN_SLEEP:                   1; // bit0
        uint32_t CTMR_CLKEN_SLEEP:                   1; // bit1
        uint32_t ATMR_CLKEN_SLEEP:                   1; // bit2
        uint32_t UART1_CLKEN_SLEEP:                  1; // bit3
        uint32_t UART2_CLKEN_SLEEP:                  1; // bit4
        uint32_t RSV_NOUSE1:                         1; // bit5       --- Ignore me
        uint32_t DMAC_CLKEN_SLEEP:                   1; // bit6
        uint32_t RSV_NOUSE2:                         1; // bit7       --- Ignore me
        uint32_t EXTI_CLKEN_SLEEP:                   1; // bit8
        uint32_t I2C_CLKEN_SLEEP:                    1; // bit9
        uint32_t IWDT_CLKEN_SLEEP:                   1; // bit10
        uint32_t MDM_CLKEN_SLEEP:                    1; // bit11
        uint32_t RF_CLKEN_SLEEP:                     1; // bit12
        uint32_t RSV_NOUSE3:                         3; // bit[15:13] --- Ignore me
        uint32_t AON_CLKEN_SLEEP:                    1; // bit16
        uint32_t APBMISC_CLKEN_SLEEP:                1; // bit17
        uint32_t RSV_END:                           14; // bit[31:18]
    };
    uint32_t Word;
} RCC_APBCLK_EN_SLEEP_TypeDef; //0x034 


//================================
#define RCC_BTMR_CLKEN_SLEEP_POS            0
#define RCC_CTMR_CLKEN_SLEEP_POS            1
#define RCC_ATMR_CLKEN_SLEEP_POS            2
#define RCC_UART1_CLKEN_SLEEP_POS           3
#define RCC_UART2_CLKEN_SLEEP_POS           4
#define RCC_DMAC_CLKEN_SLEEP_POS            6
#define RCC_EXTI_CLKEN_SLEEP_POS            8
#define RCC_I2C_CLKEN_SLEEP_POS             9
#define RCC_IWDT_CLKEN_SLEEP_POS            10
#define RCC_MDM_CLKEN_SLEEP_POS             11
#define RCC_RF_CLKEN_SLEEP_POS              12
#define RCC_AON_CLKEN_SLEEP_POS             16
#define RCC_APBMISC_CLKEN_SLEEP_POS         17
//================================

typedef union //0x038 
{
    struct
    {
        uint32_t RCC_CLKEN_DPSLEEP:                  1; // bit0
        uint32_t SYSCFG_CLKEN_DPSLEEP:               1; // bit1
        uint32_t RSV_NOUSE1:                         2; // bit[3:2] --- Ignore me
        uint32_t SPIM_CLKEN_DPSLEEP:                 1; // bit4
        uint32_t SPIS_CLKEN_DPSLEEP:                 1; // bit5
        uint32_t FSHC_CLKEN_DPSLEEP:                 1; // bit6
        uint32_t ADC_CLKEN_DPSLEEP:                  1; // bit7
        uint32_t RSV_NOUSE2:                         1; // bit8     --- Ignore me
        uint32_t AHB2P1_CLKEN_DPSLEEP:               1; // bit9     --- use for ahb2apb1 bridge
        uint32_t AHB2P2_CLKEN_DPSLEEP:               1; // bit10    --- use for ahb2apb2 bridge
        uint32_t CACHE_CLKEN_DPSLEEP:                1; // bit11
        uint32_t USB_CLKEN_DPSLEEP:                  1; // bit12
        uint32_t RSV_END:                           19; // bit[31:13]
    };
    uint32_t Word;
} RCC_AHBCLK_EN_DPSLEEP_TypeDef; //0x038 


//================================
#define RCC_RCC_CLKEN_DPSLEEP_POS           0
#define RCC_SYSCFG_CLKEN_DPSLEEP_POS        1
#define RCC_SPIM_CLKEN_DPSLEEP_POS          4
#define RCC_SPIS_CLKEN_DPSLEEP_POS          5
#define RCC_FSHC_CLKEN_DPSLEEP_POS          6
#define RCC_ADC_CLKEN_DPSLEEP_POS           7
#define RCC_AHB2P1_CLKEN_DPSLEEP_POS        9
#define RCC_AHB2P2_CLKEN_DPSLEEP_POS        10
#define RCC_CACHE_CLKEN_DPSLEEP_POS         11
#define RCC_USB_CLKEN_DPSLEEP_POS           12
//================================

typedef union //0x03c 
{
    struct
    {
        uint32_t BTMR_CLKEN_DPSLEEP:                 1; // bit0
        uint32_t CTMR_CLKEN_DPSLEEP:                 1; // bit1
        uint32_t ATMR_CLKEN_DPSLEEP:                 1; // bit2
        uint32_t UART1_CLKEN_DPSLEEP:                1; // bit3
        uint32_t UART2_CLKEN_DPSLEEP:                1; // bit4
        uint32_t RSV_NOUSE1:                         1; // bit5       --- Ignore me
        uint32_t DMAC_CLKEN_DPSLEEP:                 1; // bit6
        uint32_t RSV_NOUSE2:                         1; // bit7       --- Ignore me
        uint32_t EXTI_CLKEN_DPSLEEP:                 1; // bit8
        uint32_t I2C_CLKEN_DPSLEEP:                  1; // bit9
        uint32_t IWDT_CLKEN_DPSLEEP:                 1; // bit10
        uint32_t MDM_CLKEN_DPSLEEP:                  1; // bit11
        uint32_t RF_CLKEN_DPSLEEP:                   1; // bit12
        uint32_t RSV_NOUSE3:                         3; // bit[15:13] --- Ignore me
        uint32_t AON_CLKEN_DPSLEEP:                  1; // bit16
        uint32_t APBMISC_CLKEN_DPSLEEP:              1; // bit17
        uint32_t RSV_END:                           14; // bit[31:18]
    };
    uint32_t Word;
} RCC_APBCLK_EN_DPSLEEP_TypeDef; //0x03c 


//================================
#define RCC_BTMR_CLKEN_DPSLEEP_POS          0
#define RCC_CTMR_CLKEN_DPSLEEP_POS          1
#define RCC_ATMR_CLKEN_DPSLEEP_POS          2
#define RCC_UART1_CLKEN_DPSLEEP_POS         3
#define RCC_UART2_CLKEN_DPSLEEP_POS         4
#define RCC_DMAC_CLKEN_DPSLEEP_POS          6
#define RCC_EXTI_CLKEN_DPSLEEP_POS          8
#define RCC_I2C_CLKEN_DPSLEEP_POS           9
#define RCC_IWDT_CLKEN_DPSLEEP_POS          10
#define RCC_MDM_CLKEN_DPSLEEP_POS           11
#define RCC_RF_CLKEN_DPSLEEP_POS            12
#define RCC_AON_CLKEN_DPSLEEP_POS           16
#define RCC_APBMISC_CLKEN_DPSLEEP_POS       17
//================================

typedef union //0x040 
{
    struct
    {
        uint32_t BLE_AHB_RSTREQ:                     1; // bit0
        uint32_t BLE_CRYPT_RSTREQ:                   1; // bit1
        uint32_t BLE_MASTER_RSTREQ:                  1; // bit2
        uint32_t BLE_AHBEN:                          1; // bit3
        uint32_t BLECLK_DIV:                         2; // bit[5:4] --- BLE master clock source select
                                                        // 0: CPU_HCLK,   used when system clock select as HSE clock;
                                                        // 1: CPU_HCLK/2, used when system clock is select as PLL 64M, and hclk_scal config as /2
                                                        // 2: CPU_HCLK/3, used when system clock is select as PLL 48M, 
                                                        // 3: CPU_HCLK/4, used when system clock is select as PLL 64M, 
        uint32_t RSV_END:                           26; // bit[31:6]
    };
    uint32_t Word;
} RCC_BLE_CLKRST_CTRL_TypeDef; //0x040 


//================================
#define RCC_BLE_AHB_RSTREQ_POS              0
#define RCC_BLE_CRYPT_RSTREQ_POS            1
#define RCC_BLE_MASTER_RSTREQ_POS           2
#define RCC_BLE_AHBEN_POS                   3
#define RCC_BLECLK_DIV_LSB                  4
#define RCC_BLECLK_DIV_WIDTH                2
//================================

typedef union //0x044 
{
    struct
    {
        uint32_t POR12_BK_FLG:                       1; // bit0
        uint32_t LVD33_OUT_RST_FLG:                  1; // bit1
        uint32_t BOD12_OUT_RST_FLG:                  1; // bit2
        uint32_t PIN_RSTN_FLG:                       1; // bit3
        uint32_t POR12_CORE_FLG:                     1; // bit4
        uint32_t IWDTRST_FLG:                        1; // bit5
        uint32_t CHIPRST_FLG:                        1; // bit6
        uint32_t SYSRST_FLG:                         1; // bit7
        uint32_t POR12_BK_FLG_CLR:                   1; // bit8
        uint32_t LVD33_OUT_RST_FLG_CLR:              1; // bit9
        uint32_t BOD12_OUT_RST_FLG_CLR:              1; // bit10
        uint32_t PIN_RSTN_FLG_CLR:                   1; // bit11
        uint32_t POR12_CORE_FLG_CLR:                 1; // bit12
        uint32_t IWDTRST_FLG_CLR:                    1; // bit13
        uint32_t CHIPRST_FLG_CLR:                    1; // bit14
        uint32_t SYSRST_FLG_CLR:                     1; // bit15
        uint32_t RSV_END:                           16; // bit[31:16]
    };
    uint32_t Word;
} RCC_CHIP_RST_ST_CLR_TypeDef; //0x044 


//================================
#define RCC_POR12_BK_FLG_POS                0
#define RCC_LVD33_OUT_RST_FLG_POS           1
#define RCC_BOD12_OUT_RST_FLG_POS           2
#define RCC_PIN_RSTN_FLG_POS                3
#define RCC_POR12_CORE_FLG_POS              4
#define RCC_IWDTRST_FLG_POS                 5
#define RCC_CHIPRST_FLG_POS                 6
#define RCC_SYSRST_FLG_POS                  7
#define RCC_POR12_BK_FLG_CLR_POS            8
#define RCC_LVD33_OUT_RST_FLG_CLR_POS       9
#define RCC_BOD12_OUT_RST_FLG_CLR_POS       10
#define RCC_PIN_RSTN_FLG_CLR_POS            11
#define RCC_POR12_CORE_FLG_CLR_POS          12
#define RCC_IWDTRST_FLG_CLR_POS             13
#define RCC_CHIPRST_FLG_CLR_POS             14
#define RCC_SYSRST_FLG_CLR_POS              15
//================================

//================================
//BLOCK RCC top struct define 
typedef struct
{
    __IO  RCC_CLK_EN_ST_TypeDef                  CLK_EN_ST           ; // 0x000,  
    __IO  uint32_t                               AHBCLK_DIV          ; // 0x004, 
                                                                       // ahb clock div value: value arange : 0 ~255
                                                                       // ahb_clk = sys_clk/(ahbclk_div+1);
                                                                       // default is ahb_clk = sys_clk;
    __IO  uint32_t                               APB1CLK_DIV         ; // 0x008, 
                                                                       // apb1 clock div value: value arange : 0 ~255
                                                                       // apb1_clk = ahb_clk/(apb1clk_div+1);
                                                                       // default is apb1_clk = ahb_clk; 
    __IO  uint32_t                               APB2CLK_DIV         ; // 0x00c, 
                                                                       // apb2 clock div value: value arange : 0 ~255
                                                                       // apb2_clk = ahb_clk/(apb2clk_div+1);
                                                                       // default is apb2_clk = ahb_clk;
                                                                       // Note: maske sure the apb2_clk is not large than 16M
                                                                       // that means :
                                                                       // when sys_clk is DPLL 64M, the apb2clk_div must set as 8'h3 
    __IO  RCC_CFG_TypeDef                        CFG                 ; // 0x010,  
    __IO  uint32_t                               STCALIB             ; // 0x014,  
    __IO  RCC_HSE_DET_CTRL_TypeDef               HSE_DET_CTRL        ; // 0x018,  
    __I   uint32_t                               RSV                 ; // 0x01c,  
    __IO  RCC_AHBRST_CTRL_TypeDef                AHBRST_CTRL         ; // 0x020,  
    __IO  RCC_APBRST_CTRL_TypeDef                APBRST_CTRL         ; // 0x024,  
    __IO  RCC_AHBCLK_EN_RUN_TypeDef              AHBCLK_EN_RUN       ; // 0x028, 
                                                                       // this reg is used when CPU is normal running mode 
    __IO  RCC_APBCLK_EN_RUN_TypeDef              APBCLK_EN_RUN       ; // 0x02c, 
                                                                       // this reg is used when CPU is normal running mode 
    __IO  RCC_AHBCLK_EN_SLEEP_TypeDef            AHBCLK_EN_SLEEP     ; // 0x030, 
                                                                       // this reg is used when CPU is sleeping mode 
    __IO  RCC_APBCLK_EN_SLEEP_TypeDef            APBCLK_EN_SLEEP     ; // 0x034, 
                                                                       // this reg is used when CPU is sleeping mode 
    __IO  RCC_AHBCLK_EN_DPSLEEP_TypeDef          AHBCLK_EN_DPSLEEP   ; // 0x038, 
                                                                       // this reg is used when CPU is Deepsleep mode 
    __IO  RCC_APBCLK_EN_DPSLEEP_TypeDef          APBCLK_EN_DPSLEEP   ; // 0x03c, 
                                                                       // this reg is used when CPU is Deepsleep mode 
    __IO  RCC_BLE_CLKRST_CTRL_TypeDef            BLE_CLKRST_CTRL     ; // 0x040,  
    __IO  RCC_CHIP_RST_ST_CLR_TypeDef            CHIP_RST_ST_CLR     ; // 0x044,  
} RCC_TypeDef;


#define RCC  (( RCC_TypeDef  *)     RCC_BASE)

#endif
