/**
 ****************************************************************************************
 *
 * @file rfctrl.h
 *
 * @brief Header file - RF Controller
 *
 ****************************************************************************************
 */

#ifndef _RF_CTRL_H_
#define _RF_CTRL_H_

#include <stdint.h>
#include "regs.h"
#include "cmsis_compiler.h"
/*
 * DEFINES
 ****************************************************************************************
 */
#define BLE_CH_NB (40)

enum rf_rate
{
    RATE_1Mbps   = 0x0,
    RATE_2Mbps   = 0x1,
    RATE_125Kbps = 0x2,
    RATE_500Kbps = 0x3,
};

extern uint8_t tx_afc_code[BLE_CH_NB], rx_afc_code[BLE_CH_NB];
/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

void rf_ctrl_init(void);
void rf_reset(void);

uint8_t rf_pa_get(void);

__STATIC_INLINE void rf_ctrl_rx_enable(uint8_t chn, uint8_t rate)
{
    RF->PLL_DYM_CTRL.Word   = 0;
    RF->PLL_TAB_OFFSET.Word = (rx_afc_code[chn] << RF_PLL_FREQ_EXT_LSB);
    RF->PLL_DYM_CTRL.Word   = chn | (rate << RF_SW_RATE_LSB) | (0x01UL << RF_SW_RX_EN_POS);
}

__STATIC_INLINE void rf_ctrl_tx_enable(uint8_t chn, uint8_t rate)
{
    uint8_t sw_pa           = rf_pa_get();
    RF->PLL_TAB_OFFSET.Word = (tx_afc_code[chn] << RF_PLL_FREQ_EXT_LSB);
    RF->PLL_DYM_CTRL.Word   = chn | (rate << RF_SW_RATE_LSB) | (0x01UL << RF_SW_TX_EN_POS) |
                            (sw_pa << RF_SW_PA_GAIN_TARGET_LSB);
}

__STATIC_INLINE void rf_ctrl_rtx_disable(void)
{
    RF->PLL_DYM_CTRL.Word = 0;
}

#endif
