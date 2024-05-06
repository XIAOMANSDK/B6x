/**
 ****************************************************************************************
 *
 * @file core.h
 *
 * @brief Header file - Core State and Mode Configuration Driver
 *
 ****************************************************************************************
 */

#ifndef _CORE_H_
#define _CORE_H_

#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

/// Bits field of Reset Source/Reason
enum rst_src_bfs
{
    // Reset Reason
    RSN_POR12_BK_BIT             = (1 << 0),
    RSN_LVD33_OUT_RST_BIT        = (1 << 1),
    RSN_BOD12_OUT_RST_BIT        = (1 << 2),
    RSN_PIN_RSTN_BIT             = (1 << 3),
    RSN_POR12_CORE_BIT           = (1 << 4),
    RSN_IWDTRST_BIT              = (1 << 5),
    RSN_CHIPRST_BIT              = (1 << 6),
    RSN_SYSRST_BIT               = (1 << 7),
    
    // Wakeup Source from poweroff state
    RSN_IO_WKUP_BIT              = (1 << 8),
    RSN_BLE_WKUP_BIT             = (1 << 9),
    RSN_RTC_WKUP_BIT             = (1 << 10),
    RSN_INT_WKUP_BIT             = (1 << 11),
};

/// Exit poweroff via BLE Wakeup 
#define RSN_IS_BLE_WKUP(rsn)       (((rsn) & (RSN_BLE_WKUP_BIT | RSN_POR12_CORE_BIT)) == (RSN_BLE_WKUP_BIT | RSN_POR12_CORE_BIT))

/// clock stable time for exit from deepsleep
#define CFG_PMU_CLK_STB(time)      ((time) << APBMISC_CLK_STB_TIME_LSB)
/// default value for pmu contrl
#define CFG_PMU_DFLT_CNTL          (CFG_PMU_CLK_STB(0x3F))

/// Bits field of Wakeup Control
enum wkup_ctrl_bfs
{
    // BLE SEL - bit[1:0] (deepsleep or poweroff)
    WKUP_BLE_SEL_LSB             = 0,
    WKUP_BLE_SEL_MSK             = (0x03 << WKUP_BLE_SEL_LSB),
    
    // BLE LATCH_N - bit2 (only poweroff)
    WKUP_BLE_LATCH_N_POS         = 2,
    WKUP_BLE_LATCH_N_BIT         = (1 << WKUP_BLE_LATCH_N_POS),
    
    // Wakeup IO EN - bit3 (deepsleep or poweroff)
    WKUP_IO_EN_POS               = 3,
    WKUP_IO_EN_BIT               = (1 << WKUP_IO_EN_POS),

    // RTC EN - bit4 (deepsleep or poweroff)
    WKUP_RTC_EN_POS              = 4,
    WKUP_RTC_EN_BIT              = (1 << WKUP_RTC_EN_POS),

    // IO LATCH_N - bit5 (only poweroff)
    WKUP_IO_LATCH_N_POS          = 5,
    WKUP_IO_LATCH_N_BIT          = (1 << WKUP_IO_LATCH_N_POS),
};

#define CFG_WKUP_PWROFF_MSK        (0x1B)
#define CFG_WKUP_DEEPSL_MSK        (0x1B)

/// Bits field of Wakeup Status
enum wkup_st_bfs
{
    // IO_WKUP_ST - bit0 (deepsleep or poweroff)
    WKUP_ST_IO_POS               = 0,
    WKUP_ST_IO_BIT               = (1 << WKUP_ST_IO_POS),
    
    // BLE_WKUP_ST - bit1 (deepsleep or poweroff)
    WKUP_ST_BLE_POS              = 1,
    WKUP_ST_BLE_BIT              = (1 << WKUP_ST_BLE_POS),
    
    // RTC_WKUP_ST - bit2 (deepsleep or poweroff)
    WKUP_ST_RTC_POS              = 2,
    WKUP_ST_RTC_BIT              = (1 << WKUP_ST_RTC_POS),
    
    // AON_PMU_INT - bit3 (deepsleep or poweroff)
    WKUP_ST_AON_POS              = 3,
    WKUP_ST_AON_BIT              = (1 << WKUP_ST_AON_POS),
    
    // BOD12_OUT12_ST - bit8 (deepsleep or poweroff)
    WKUP_ST_BOD12_POS            = 8,
    WKUP_ST_BOD12_BIT            = (1 << WKUP_ST_BOD12_POS),
    
    // LVD33_OUT12_ST - bit9 (deepsleep or poweroff)
    WKUP_ST_LVD33_POS            = 9,
    WKUP_ST_LVD33_BIT            = (1 << WKUP_ST_LVD33_POS),
};

#define CFG_WKUP_ST_MSK            (0x030F)
#define CFG_WKUP_ST_CLR_MSK        (0xC00000)

/// BLE as wakeup source
#define CFG_WKUP_BLE_EN            (1 << WKUP_BLE_SEL_LSB)
/// RTC as wakeup source
#define CFG_WKUP_RTC_EN            (WKUP_RTC_EN_BIT)
/// IO as wakeup source 
#define CFG_WKUP_IO_EN             (WKUP_IO_EN_BIT)

/// Bits field of LDO Control
enum ldo_ctrl_bfs
{
    // BOD ctrl - bit[2:0], bit3, bit4
    LDO_BOD_TRIM_LSB             = 0,
    LDO_BOD_TRIM_MSK             = (0x07 << LDO_BOD_TRIM_LSB),
    LDO_BOD_EN_POS               = 3,
    LDO_BOD_EN_BIT               = (1 << LDO_BOD_EN_POS),
    LDO_BOD_RSTEN_POS            = 4,
    LDO_BOD_RSTEN_BIT            = (1 << LDO_BOD_RSTEN_POS),
    
    // LDO12 ibsel - bit[9:5]
    LDO_IBSEL_LSB                = 5,
    LDO_IBSEL_MSK                = (0x1F << LDO_IBSEL_LSB),
    
    // LVD ctrl - bit10, bit[13:11], bit14
    LDO_LVD_EN_POS               = 10,
    LDO_LVD_EN_BIT               = (1 << LDO_LVD_EN_POS),
    LDO_LVD_SEL_LSB              = 11,
    LDO_LVD_SEL_MSK              = (0x07 << LDO_LVD_SEL_LSB),
    LDO_LVD_RSTEN_POS            = 14,
    LDO_LVD_RSTEN_BIT            = (1 << LDO_LVD_RSTEN_POS),
    
    // ANA resv - bit[24:16]
    LDO_ANA_RESV_LSB             = 16,
    LDO_ANA_RESV_MSK             = (0x1FF << LDO_ANA_RESV_LSB),
    
    // MISC ctrl - bit[29:25]
    LDO_TESTA_XO_POS             = 25,
    LDO_TESTA_XO_BIT             = (1 << LDO_TESTA_XO_POS),
    LDO_BOD_TESTEN_POS           = 26,
    LDO_BOD_TESTEN_BIT           = (1 << LDO_BOD_TESTEN_POS),
    LDO_BOD_INTEN_POS            = 27,
    LDO_BOD_INTEN_BIT            = (1 << LDO_BOD_INTEN_POS),
    LDO_LVD_INTEN_POS            = 28,
    LDO_LVD_INTEN_BIT            = (1 << LDO_LVD_INTEN_POS),
    LDO_VBK_TESTEN_POS           = 29,
    LDO_VBK_TESTEN_BIT           = (1 << LDO_VBK_TESTEN_POS),

    // Mask for EN bits
    LDO_BOD_ENB_MSK              = (LDO_BOD_EN_BIT | LDO_BOD_RSTEN_BIT | LDO_BOD_INTEN_BIT),
    LDO_LVD_ENB_MSK              = (LDO_LVD_EN_BIT | LDO_LVD_RSTEN_BIT | LDO_LVD_INTEN_BIT),
};

#define CFG_BOD_TRIM(val)          ((val) << LDO_BOD_TRIM_LSB)
#define CFG_BOD_EN(en, rst, irq)   \
    (((en) << LDO_BOD_EN_POS) | ((rst) << LDO_BOD_RSTEN_POS) | ((irq) << LDO_BOD_INTEN_POS))

#define CFG_LVD_SEL(val)           ((val) << LDO_LVD_SEL_LSB)
#define CFG_LVD_EN(en, rst, irq)   \
    (((en) << LDO_LVD_EN_POS) | ((rst) << LDO_LVD_RSTEN_POS) | ((irq) << LDO_LVD_INTEN_POS))


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Get ldo register value.
 *
 * @param[in] ctrl   PMU_ANA_CTRL[23:0] & ANAMISC_CTRL[29:24].
 ****************************************************************************************
 */
void core_ldoset(uint32_t ctrl);

/**
 ****************************************************************************************
 * @brief Get ldo register value.
 *
 * @return value. PMU_ANA_CTRL[23:0] & ANAMISC_CTRL[29:24].
 ****************************************************************************************
 */
uint32_t core_ldoget(void);

/**
 ****************************************************************************************
 * @brief Core Sleep Mode.
 *
 * @param[in] cfg_wkup  Bits field of value @see enum wkup_ctrl_bfs.
 *
 * @return wake-up status @see wkup_st_bfs.
 ****************************************************************************************
 */
uint16_t core_sleep(uint16_t cfg_wkup);

/**
 ****************************************************************************************
 * @brief Set IO wake-up sleep or power off.
 *
 * @param[in] wkup_en  IO that supports wake-up(one bit one io).
 * @param[in] pad_pu   IO that pull-up(one bit one io).
 *
 * @note pad_pu Corresponding bit 1:Falling edge wake-up, 0: Rising edge wake-up.
 ****************************************************************************************
 */
void wakeup_io_sw(uint32_t wkup_en, uint32_t pad_pu);

/**
 ****************************************************************************************
 * @brief Core Power Off Mode.
 *
 * @param[in] cfg_wkup  Bits field of value @see enum wkup_ctrl_bfs.
 *
 * @note PA19 Restore PULL-UP and Input Enable.
 ****************************************************************************************
 */
void core_pwroff(uint16_t cfg_wkup);

/**
 ****************************************************************************************
 * @brief Release IO Latch.
 ****************************************************************************************
 */
void core_release_io_latch(void);

/**
 ****************************************************************************************
 * @brief Get IC 128bits Unique ID.
 *
 * @param[in]  size        Length of Unique ID Buffer.
 * @param[out] unique_id   Pointer of Unique ID buffer.
 *
 * @note size must be less than or equal to 16.
 * @note IC Must have Flash.
 ****************************************************************************************
 */
void core_unique_id(uint8_t size, uint8_t *unique_id);
#endif // _CORE_H_
