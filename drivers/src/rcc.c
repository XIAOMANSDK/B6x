/**
 ****************************************************************************************
 *
 * @file rcc.c
 *
 * @brief 复位和时钟控制(RCC)驱动程序 - 系统时钟配置
 *
 * 该文件包含系统时钟的配置、获取和频率计算等功能的实现。
 * 支持HSI、HSE、LSI、PLL等多种时钟源，可动态切换系统时钟频率。
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

/// CFG.SYSCLK_SW 的值定义
enum sysclk_sw
{
    SCLK_HSI = 1,  /* HSI(内部16MHz RC振荡器) */
    SCLK_HSE = 2,  /* HSE(外部16MHz晶体振荡器) */
    SCLK_PLL = 4,  /* PLL(锁相环输出) */
    SCLK_LSI = 8,  /* LSI(内部32kHz RC振荡器) */
};

/// BLE时钟预分频配置(16MHz固定)，从SYS_CLK_16M~64M --> psc0~3相同顺序
#define BLE_CLK_PSC(sclk)       (((sclk) <= SYS_CLK_64M) ? (sclk) : 0)

/// 选择DPLL_EN 64M或DPLL2_EN 48M
#define DPLL_IS_48M()           (RCC->CFG.DPLL_CLK_SW)

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * @brief 获取总线控制寄存器值
 * 
 * @param[in] idx 总线控制寄存器索引
 * @return uint32_t 寄存器值
 * 
 * @details 根据索引计算寄存器地址并返回其当前值
 */
uint32_t rcc_bus_get(uint8_t idx)
{
    uint32_t reg_addr = RCC_BASE + RCC_AHBRST_CTRL_ADDR_OFFSET + (idx * 4);

    return (*(volatile uint32_t *)reg_addr);
}

/**
 * @brief 使能总线时钟
 * 
 * @param[in] idx 总线控制寄存器索引
 * @param[in] bfs 总线使能位掩码
 * 
 * @details 设置指定总线控制寄存器中的使能位
 */
void rcc_bus_enb(uint8_t idx, uint32_t bfs)
{
    uint32_t reg_addr = RCC_BASE + RCC_AHBRST_CTRL_ADDR_OFFSET + (idx * 4);

    (*(volatile uint32_t *)reg_addr) |= bfs;
}

/**
 * @brief 禁用总线时钟
 * 
 * @param[in] idx 总线控制寄存器索引
 * @param[in] bfs 总线禁用位掩码
 * 
 * @details 清除指定总线控制寄存器中的使能位
 */
void rcc_bus_dis(uint8_t idx, uint32_t bfs)
{
    uint32_t reg_addr = RCC_BASE + RCC_AHBRST_CTRL_ADDR_OFFSET + (idx * 4);

    (*(volatile uint32_t *)reg_addr) &= ~bfs;
}

/**
 * @brief 复位总线
 * 
 * @param[in] idx 总线控制寄存器索引
 * @param[in] bfs 总线复位位掩码
 * 
 * @details 对指定总线执行复位操作(先置位再清除复位位)
 */
void rcc_bus_rst(uint8_t idx, uint32_t bfs)
{
    uint32_t reg_addr = RCC_BASE + RCC_AHBRST_CTRL_ADDR_OFFSET + (idx * 4);

    (*(volatile uint32_t *)reg_addr) |= bfs;
    (*(volatile uint32_t *)reg_addr) &= ~bfs;
}

/**
 * @brief 使能ADC时钟
 * 
 * @details 
 * - 执行ADC模块的完整复位序列
 * - 先禁用ADC时钟，然后置位复位请求，最后释放复位并使能时钟
 */
void rcc_adc_en(void)
{
    // ADC reset: clk=0, rst=1, rst=0, clk=1
    RCC->AHBCLK_EN_RUN.ADC_CLKEN_RUN     = 0;    /* 禁用ADC时钟 */
    RCC->AHBRST_CTRL.ADC_RSTREQ          = 1;    /* 置位ADC复位请求 */
    RCC->AHBRST_CTRL.ADC_RSTREQ          = 0;    /* 清除ADC复位请求 */
    RCC->AHBCLK_EN_RUN.ADC_CLKEN_RUN     = 1;    /* 使能ADC时钟 */
}

/**
 * @brief 使能BLE时钟和复位控制
 * 
 * @details
 * - 使能modem和RF的APB时钟
 * - 使能BLE AHB时钟并执行复位
 * - 配置BLE低功耗时钟源为LSI
 */
void rcc_ble_en(void)
{
    // modem&rf clk enable
    //RCC->APBCLK_EN_RUN.Word |= (APB_MDM_BIT | APB_RF_BIT);
    RCC_APBCLK_EN(APB_MDM_BIT | APB_RF_BIT);     /* 使能modem和RF的APB时钟 */

    // ble clk enable
    RCC->BLE_CLKRST_CTRL.BLE_AHBEN = 1;          /* 使能BLE AHB时钟 */

    // ble reg reset
    RCC->BLE_CLKRST_CTRL.BLE_AHB_RSTREQ = 1;     /* 置位BLE AHB复位请求 */
    RCC->BLE_CLKRST_CTRL.BLE_AHB_RSTREQ = 0;     /* 清除BLE AHB复位请求 */

    #if (EM_PWOFF)
    // em power enable
    AON->PWOFF_CTRL.EM_PD_EN = 0;                /* 禁用EM电源关断 */
    AON->PWOFF_CTRL.EM_ACCESS_EN = 1;            /* 使能EM访问 */
    // wait em power stable
    while (AON->PWOFF_CTRL.EM_PD_ACK);           /* 等待EM电源稳定 */
    #endif

    // ble signal latch release - move to ble_init() / ble_resume()
    //AON->PMU_WKUP_CTRL.BLE_LATCH_N = 1;
    // ble low power source select lsi
    AON->BLE_RTC_CTL.BLELOWCLK_SEL = 0;          /* BLE低功耗时钟选择LSI */
    // ble low power enable
    AON->BLE_RTC_CTL.BLELOWCLK_EN  = 1;          /* 使能BLE低功耗时钟 */
}

/**
 * @brief 使能USB时钟和配置
 * 
 * @details
 * - 使能USB AHB时钟并执行复位
 * - 配置USB数据线GPIO为高阻态
 * - 配置USB PHY控制寄存器
 */
void rcc_usb_en(void)
{
    // clk rst enable
    RCC_AHBCLK_EN(AHB_USB_BIT);                  /* 使能USB AHB时钟 */
    RCC_AHBRST_REQ(AHB_USB_BIT);                 /* USB AHB复位请求 */

    // iopad to hiz
    GPIO_DIR_CLR((1UL << PA_USB_DP) | (1UL << PA_USB_DM));   /* 清除USB数据线方向 */
    iom_ctrl(PA_USB_DP, IOM_HIZ);                /* USB DP引脚设为高阻 */
    iom_ctrl(PA_USB_DM, IOM_HIZ);                /* USB DM引脚设为高阻 */

    // 1.5k pull-up(.USB_PHY_MOD=0, .DIG_USB_PU=3, .DIG_USB_RXEN=1,)
    //SYSCFG->USB_CTRL.DIG_USB_PU   = 3; // FIB result 1 --> 3
    //SYSCFG->USB_CTRL.DIG_USB_RXEN = 1;
    SYSCFG->USB_CTRL.Word = 0x1C;                /* 配置USB PHY控制寄存器 */
}

/**
 * @brief 设置系统时钟源和频率
 *
 * @param[in] sclk_sel 系统时钟选择 @see enum sys_clk_t
 *                     - SYS_CLK_16M: 16MHz HSE时钟
 *                     - SYS_CLK_HSI: 16MHz HSI时钟  
 *                     - SYS_CLK_LSI: 32kHz LSI时钟
 *                     - SYS_CLK_32M: 32MHz PLL时钟
 *                     - SYS_CLK_48M: 48MHz PLL时钟
 *                     - SYS_CLK_64M: 64MHz PLL时钟
 *
 * @details
 * - 根据选择的时钟源配置相应的时钟使能和分频参数
 * - 对于PLL时钟，需要配置DPLL时钟选择和分频器
 * - 设置BLE时钟分频和AHB总线时钟分频
 * - 最后切换系统时钟源
 *
 * @note 
 * - 如果已经处于目标时钟源，则直接返回不重复配置
 * - BLE时钟必须保持16MHz，会根据系统时钟自动分频
 * - PLL时钟需要额外的DPLL配置和使能
 */
void rcc_sysclk_set(uint8_t sclk_sel)
{
    uint8_t clk_sw  = RCC->CFG.SYSCLK_SW;        /* 当前系统时钟源 */
    uint8_t clk_div = 0;                         /* 默认RCC->AHBCLK_DIV = 0 */
    uint8_t ble_div = 0;                         /* 默认RCC->BLE_CLKRST_CTRL.BLECLK_DIV = 0 */

    // 系统时钟切换，如果已经是目标时钟则跳过
    switch (sclk_sel)
    {
        case SYS_CLK_16M:  /* 16MHz HSE时钟 */
        {
            if (clk_sw == SCLK_HSE) return;      /* 已经是HSE时钟，直接返回 */

            clk_sw = SCLK_HSE;
            // 使用外部16MHz晶体振荡器作为时钟源
            //AON->PMU_CTRL.OSC_EN_RUN          = 1;
            //APBMISC->XOSC16M_CTRL.EN_XO16MBUF = 1;
            RCC->CLK_EN_ST.HSE_DIG_EN = 1;       /* 使能HSE数字时钟 */
        } break;

        case SYS_CLK_HSI:  /* 16MHz HSI时钟 */
        {
            if (clk_sw == SCLK_HSI) return;      /* 已经是HSI时钟，直接返回 */

            clk_sw = SCLK_HSI;
            // 使用内部16MHz RC振荡器作为时钟源
            //APBMISC->AON_PMU_CTRL.HSI_EN_RUN = 1;
            RCC->CLK_EN_ST.HSI_DIG_EN = 1;       /* 使能HSI数字时钟 */
        } break;

        case SYS_CLK_LSI:  /* 32kHz LSI时钟 */
        {
            if (clk_sw == SCLK_LSI) return;      /* 已经是LSI时钟，直接返回 */

            clk_sw = SCLK_LSI;
            // 使用内部32kHz RC振荡器作为时钟源
            AON->PMU_CTRL.RC32K_EN_RUN = 1;      /* 使能RC32K振荡器 */
            RCC->CLK_EN_ST.LSI_DIG_EN  = 1;      /* 使能LSI数字时钟 */
        } break;

        default:  /* PLL时钟(32M/48M/64M) */
        {
            clk_sw = SCLK_PLL;
            ble_div = (sclk_sel - SYS_CLK_16M);  /* 计算BLE时钟分频 */

            // 使用DPLL作为时钟源
            if (sclk_sel == SYS_CLK_48M)
            {
                RCC->CFG.DPLL_CLK_SW = 1;        /* 选择48MHz DPLL时钟 */
            }
            else
            {
                clk_div = (sclk_sel == SYS_CLK_32M) ? 1 : 0;  /* 32MHz需要2分频，64MHz不分频 */
                RCC->CFG.DPLL_CLK_SW = 0;        /* 选择64MHz DPLL时钟 */
            }

            APBMISC->DPLL_CTRL.DPLL2_EN = 1;     /* 使能DPLL2 */
            RCC->CLK_EN_ST.DPLL_DIG_EN  = 1;     /* 使能DPLL数字时钟 */
        } break;
    }

    RCC->BLE_CLKRST_CTRL.BLECLK_DIV = ble_div;   /* 配置BLE时钟分频(BLE必须16MHz) */
    RCC->AHBCLK_DIV    = clk_div;                /* 配置AHB总线时钟分频 */
    RCC->CFG.SYSCLK_SW = clk_sw;                 /* 切换系统时钟源 */
}

/**
 * @brief 获取当前系统时钟配置
 *
 * @return uint8_t 当前系统时钟枚举值 @see enum sys_clk_t
 *
 * @details
 * - 读取当前系统时钟源选择寄存器
 * - 根据时钟源和分频设置确定具体的时钟频率
 * - 返回对应的系统时钟枚举值
 *
 * @note
 * - 对于PLL时钟，需要进一步判断是48MHz还是64MHz模式
 * - 64MHz模式下根据AHB分频确定是32MHz还是64MHz
 */
uint8_t rcc_sysclk_get(void)
{
    uint8_t sclk_cur;
    uint8_t clk_sw = RCC->CFG.SYSCLK_SW;         /* 读取当前系统时钟源 */

    if (clk_sw == SCLK_HSE)                      /* HSE时钟源 */
    {
        sclk_cur = SYS_CLK_16M;
    }
    else if (clk_sw == SCLK_PLL)                 /* PLL时钟源 */
    {
        sclk_cur = DPLL_IS_48M() ? SYS_CLK_48M : ((RCC->AHBCLK_DIV == 1) ? SYS_CLK_32M : SYS_CLK_64M);
    }
    else if (clk_sw == SCLK_LSI)                 /* LSI时钟源 */
    {
        sclk_cur = SYS_CLK_LSI;
    }
    else /* SCLK_HSI */                          /* HSI时钟源 */
    {
        sclk_cur = SYS_CLK_HSI;
    }

    return sclk_cur;
}

/**
 * @brief 计算当前系统时钟的实际频率
 *
 * @return uint32_t 当前系统时钟频率(Hz)
 *
 * @details
 * - 根据当前系统时钟源确定基础频率
 * - 考虑AHB总线时钟分频的影响
 * - 返回实际的工作频率
 *
 * @note
 * - 对于PLL时钟，基础频率为48MHz或64MHz
 * - 对于HSI和HSE时钟，基础频率为16MHz
 * - 对于LSI时钟，基础频率为32kHz
 * - AHB分频会进一步降低系统时钟频率
 */
uint32_t rcc_sysclk_freq(void)
{
    uint32_t clk_freq;
    uint8_t clk_sw = RCC->CFG.SYSCLK_SW;         /* 读取当前系统时钟源 */

    if (clk_sw == SCLK_PLL)                      /* PLL时钟源 */
    {
        clk_freq = DPLL_IS_48M() ? SYS_FREQ_48M : SYS_FREQ_64M;
    }
    else if (clk_sw == SCLK_LSI)                 /* LSI时钟源 */
    {
        clk_freq = SYS_FREQ_32K;
    }
    else // SCLK_HSE SCLK_HSI                    /* HSE或HSI时钟源 */
    {
        clk_freq = SYS_FREQ_16M;
    }

    // 预分频时钟
    uint8_t clk_div = (RCC->AHBCLK_DIV & 0xFF);  /* 读取AHB时钟分频值 */
    if (clk_div > 0)
    {
        clk_freq /= (clk_div + 1);               /* 计算分频后的实际频率 */
    }

    return clk_freq;
}

/**
 * @brief 获取Flash控制器时钟选择配置
 *
 * @return uint8_t Flash时钟选择配置
 *
 * @details
 * - 读取Flash控制器时钟选择寄存器
 * - 对于DPLL128时钟源，还需要考虑分频配置
 * - 返回完整的Flash时钟配置值
 */
uint8_t rcc_fshclk_get(void)
{
    uint8_t fclk_sel = RCC->CLK_EN_ST.FSHCCLK_SEL;   /* 读取Flash时钟选择 */

    if (fclk_sel == FSH_CLK_DPLL128)                 /* DPLL128时钟源 */
    {
        fclk_sel += (RCC->CLK_EN_ST.FSHCCLK_DIV_NUM);  /* 包含分频配置 */
    }

    return fclk_sel;
}

/**
 * @brief 获取Flash控制器时钟频率(MHz)
 *
 * @return uint8_t Flash时钟频率(MHz)
 *
 * @details
 * - 根据Flash时钟选择确定基础频率
 * - 对于DPLL128时钟源，需要考虑分频系数
 * - 返回实际的Flash时钟频率
 *
 * @note
 * - FSH_CLK_DPLL128: 128MHz时钟，可配置分频
 * - FSH_CLK_DPLL: 使用DPLL时钟(48MHz或64MHz)
 * - FSH_CLK_HSE16/FSH_CLK_HSI16: 16MHz时钟
 */
uint8_t rcc_fshclk_mhz(void)
{
    uint8_t fclk_mhz;
    uint8_t fclk_sel = RCC->CLK_EN_ST.FSHCCLK_SEL;   /* 读取Flash时钟选择 */

    if (fclk_sel == FSH_CLK_DPLL128)                 /* DPLL128时钟源 */
    {
        fclk_mhz = 128 / (RCC->CLK_EN_ST.FSHCCLK_DIV_NUM + 1);  /* 计算分频后的频率 */
    }
    else if (fclk_sel == FSH_CLK_DPLL)               /* DPLL时钟源 */
    {
        fclk_mhz = (DPLL_IS_48M() ? FSH_FREQ_48MHz : FSH_FREQ_64MHz);  /* 48MHz或64MHz */
    }
    else // FSH_CLK_HSE16 FSH_CLK_HSI16              /* HSE或HSI时钟源 */
    {
        fclk_mhz = FSH_FREQ_16MHz;                   /* 16MHz */
    }

    return fclk_mhz;
}

#if (ROM_UNUSED)
/**
 * @brief 设置Flash控制器时钟
 *
 * @param[in] fclk_sel Flash时钟选择配置
 *
 * @details
 * - 对于DPLL128时钟源，需要配置分频器和分频使能
 * - 分频配置必须在时钟选择之前完成
 * - 支持动态切换Flash时钟频率
 *
 * @note 仅在ROM_UNUSED宏定义时可用
 */
void rcc_fshclk_set(uint8_t fclk_sel)
{
    if (fclk_sel >= FSH_CLK_DPLL128)                /* DPLL128时钟源，需要分频配置 */
    {
        // 注意fshcclk_div_num必须大于0，所以默认2(psc3=42M)
        uint8_t clk_div = (fclk_sel > FSH_CLK_DPLL128) ? (fclk_sel - FSH_CLK_DPLL128) : 2;

        RCC->CLK_EN_ST.FSHCCLK_SEL = FSH_CLK_HSI16; /* 临时切换到HSI16 */

        RCC->CLK_EN_ST.FSHCCLK_DIVEN   = 0;         /* 禁用分频器 */
        RCC->CLK_EN_ST.FSHCCLK_DIV_NUM = clk_div;   /* 设置分频系数 */
        // fshcclk_diven必须在fshcclk_sel之前设置为1
        RCC->CLK_EN_ST.FSHCCLK_DIVEN   = 1;         /* 使能分频器 */

        RCC->CLK_EN_ST.FSHCCLK_SEL = FSH_CLK_DPLL128; /* 切换到DPLL128时钟源 */
    }
    else  /* 其他时钟源直接切换 */
    {
        RCC->CLK_EN_ST.FSHCCLK_SEL = fclk_sel;      /* 直接设置时钟选择 */
    }
}
#endif
