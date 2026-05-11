/**
 ****************************************************************************************
 *
 * @file sysdbg.h
 *
 * @brief Header file - system debug observation.
 *
 ****************************************************************************************
 */

#ifndef _SYSDBG_H_
#define _SYSDBG_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// sysdbg[0:15] map PA02~05,PA08~19
enum sysdbg_pio
{
    SYS_DBG_PIO00           = (1UL << 2),
    SYS_DBG_PIO01           = (1UL << 3),
    SYS_DBG_PIO02           = (1UL << 4),
    SYS_DBG_PIO03           = (1UL << 5),
    SYS_DBG_PIO04           = (1UL << 8),
    SYS_DBG_PIO05           = (1UL << 9),
    SYS_DBG_PIO06           = (1UL << 10),
    SYS_DBG_PIO07           = (1UL << 11),
    SYS_DBG_PIO08           = (1UL << 12),
    SYS_DBG_PIO09           = (1UL << 13),
    SYS_DBG_PIO10           = (1UL << 14),
    SYS_DBG_PIO11           = (1UL << 15),
    SYS_DBG_PIO12           = (1UL << 16),
    SYS_DBG_PIO13           = (1UL << 17),
    SYS_DBG_PIO14           = (1UL << 18),
    SYS_DBG_PIO15           = (1UL << 19),
    SYS_DBG_PIO_ALL         = 0xFFF3C,
    SYS_DBG_PIO_MSK         = 0xFFF3C,
};

/// sysdbg selection
enum sysdbg_sel
{
    SYS_DBG_RF              = 0,
    SYS_DBG_BLE             = 1,
    SYS_DBG_MDM             = 2,
    SYS_DBG_USB             = 3,
};

/// RF debug port
enum rf_dbg
{
    RF_DBG_TX               = 1,
    RF_DBG_RX               = 2,
    RF_DBG_CAL              = 3,
};

/// Ble debug diagnose
enum ble_dbg
{
    BLE_DBG_TX              = 0x0083,
    BLE_DBG_RX              = 0x8300,
    BLE_DBG_TXRX            = 0x8383,
};

/// Modem debug mode
enum mdm_dbg
{
    MDM_DBG1                = 1,
    MDM_DBG2                = 2,
    MDM_DBG3                = 3,
    MDM_DBG_EXT_TX          = 4,
    MDM_DBG_EXT_RX          = 5,
};

/// USB debug diagnose
enum usb_dbg
{
    USB_DBG1                = 1,
    USB_DBG2                = 2,
};

/// Analog test en 
enum ana_tst_en
{
    ATEN_AT0_BIT            = 0x01,
    ATEN_AT1_BIT            = 0x02,
    ATEN_AT0_AT1            = (ATEN_AT0_BIT | ATEN_AT1_BIT),
    
    ATEN_CBPF_BIT           = 0x04,
    ATEN_RSSI_BIT           = 0x08,
};

/// AT0 AT1 selection
enum ana_tst_sel
{
    // AT0 Mux - bit[3:0]
    AT0_SEL_LSB             = 0,
    AT0_VDD_IF              = (0 << AT0_SEL_LSB),
    AT0_BPF_IOP             = (1 << AT0_SEL_LSB),
    AT0_BPF_QOP             = (2 << AT0_SEL_LSB),
    AT0_VDD_PLL             = (5 << AT0_SEL_LSB),
    AT0_PLL_VCTRL           = (6 << AT0_SEL_LSB),
    AT0_LMT_OUTI            = (10 << AT0_SEL_LSB),
    AT0_PLL_FBCLK_DIV2      = (12 << AT0_SEL_LSB),
    AT0_BPF_CAL_DONE        = (14 << AT0_SEL_LSB),
    AT0_SEL_MSK             = (0x0F << AT0_SEL_LSB),
    // AT1 Mux - bit[7:4]
    AT1_SEL_LSB             = 4,
    AT1_VDD_TX              = (0 << AT1_SEL_LSB),
    AT1_BPF_ION             = (1 << AT1_SEL_LSB),
    AT1_BPF_QON             = (2 << AT1_SEL_LSB),
    AT1_VDD_RX              = (5 << AT1_SEL_LSB),
    AT1_VDD_VCO             = (6 << AT1_SEL_LSB),
    AT1_RSSI_OUT            = (7 << AT1_SEL_LSB),
    AT1_LMT_OUTQ            = (10 << AT1_SEL_LSB),
    AT1_PLL_LOCK            = (13 << AT1_SEL_LSB),
    AT1_AGC_POWER_DET       = (15 << AT1_SEL_LSB),
    AT1_SEL_MSK             = (0x0F << AT1_SEL_LSB),
};


/*
 * MACRO ALIAS
 ****************************************************************************************
 */

// Init all debug pio
#define sys_dbg_init()      sysdbg_iomap(SYS_DBG_PIO_ALL)

// RF debug select, 'txrx' @see enum rf_dbg
#define rf_dbg_sel(txrx)    sysdbg_select(SYS_DBG_RF, txrx)

// BLE debug select, 'diag' @see enum ble_dbg
#define ble_dbg_sel(diag)   sysdbg_select(SYS_DBG_BLE, diag)

// Modem debug select, 'mode' @see enum mdm_dbg
#define mdm_dbg_sel(mode)   sysdbg_select(SYS_DBG_MDM, mode)

// AT0 AT1 enable CBPF observation
#define at0_at1_cbpf_out()  sysdbg_anatst(ATEN_AT0_AT1|ATEN_CBPF_BIT, AT0_BPF_IOP|AT1_BPF_ION)

// AT0 AT1 enable RSSI observation
#define at0_at1_rssi_out()  sysdbg_anatst(ATEN_AT0_AT1|ATEN_RSSI_BIT, AT1_RSSI_OUT)


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Map iopads for debug observation.
 *
 * @param[in] pio  Bits of PAxx @see enum sysdbg_pio
 *
 ****************************************************************************************
 */
void sysdbg_iomap(uint32_t pio);

/**
 ****************************************************************************************
 * @brief Select debug module to observe.
 *
 * @param[in] dbgsel  Debug Selected @see enum sysdbg_sel
 * @param[in] diag    Mode Value @see enum rf_dbg or ble_dbg or mdm_dbg
 *
 ****************************************************************************************
 */
void sysdbg_select(uint8_t dbgsel, uint32_t diag);

/**
 ****************************************************************************************
 * @brief Config AT0 AT1 for RF debug observation.
 *
 * @param[in] tsten  Analog test bits @see enum ana_tst_en
 * @param[in] atsel  Selected func @see enum ana_tst_sel
 *
 ****************************************************************************************
 */
void sysdbg_anatst(uint8_t tsten, uint8_t atsel);


#endif // _SYSDBG_H_
