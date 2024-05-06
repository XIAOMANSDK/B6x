/**
 ****************************************************************************************
 *
 * @file sysdbg.c
 *
 * @brief System Debug Observation
 *
 ****************************************************************************************
 */

#include "sysdbg.h"
#include "gpio.h"
#include "reg_csc.h"
#include "reg_mdm.h"
#include "reg_rf.h"
#include "reg_syscfg.h"


/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void sysdbg_iomap(uint32_t pio)
{
    pio &= SYS_DBG_PIO_MSK;
    
    for (uint8_t i = 2; i < 20; i++)
    {
        if (pio & (1UL << i))
        {
            iom_ctrl(i, IOM_SEL_DEBUG);//CSC->CSC_PIO[i].Word = IOM_SEL_DEBUG;
        }
        else if (CSC->CSC_PIO[i].Word == IOM_SEL_DEBUG)
        {
            iom_ctrl(i, IOM_SEL_GPIO); //CSC->CSC_PIO[i].Word = IOM_SEL_GPIO;
        }
    }
}

void sysdbg_anatst(uint8_t tsten, uint8_t atsel)
{
    // Note: pu, pd, oe, ie dsiable, gpio mode
    if (tsten & ATEN_AT0_BIT)
    {
        GPIO_DIR_CLR(1UL << PA_AT0);
        iom_ctrl(PA_AT0, IOM_ANALOG);
        
        RF->ANAMISC_CTRL1.AT0_SEL = atsel & 0x0F;
    }
    
    if (tsten & ATEN_AT1_BIT)
    {
        GPIO_DIR_CLR(1UL << PA_AT1);
        iom_ctrl(PA_AT1, IOM_ANALOG);
        
        RF->ANAMISC_CTRL1.AT1_SEL = (atsel >> 4) & 0x0F;
    }
    
    RF->ANAMISC_CTRL1.TSTEN_CBPF = (tsten & ATEN_CBPF_BIT) ? 1 : 0;
    RF->ANAMISC_CTRL1.TSTEN_RSSI = (tsten & ATEN_RSSI_BIT) ? 1 : 0;
}

void sysdbg_select(uint8_t dbgsel, uint32_t diag)
{
    if (dbgsel == SYS_DBG_RF)
    {
        // 2'b01: tx debug port, 2'b10: rx debug port, 2'b11: calib_cnt_debug
        RF->DIG_CTRL.RF_DBG_SEL = (diag & 0x03);
    }
    else if (dbgsel == SYS_DBG_BLE)
    {
        // BB DIAG1 0x83, 0x8300, 0x8383
        (*(volatile uint32_t *)0x50000050) = diag;
    }
    else if (dbgsel == SYS_DBG_MDM)
    {
        MDM->REG0.DEBUG_MODE = diag;
    }
    else if (dbgsel == SYS_DBG_USB)
    {
        SYSCFG->DBG_CTRL.USB_DBG_SEL = diag;
    }
    else
    {
        return; // not support
    }
    
    SYSCFG->DBG_CTRL.SYS_DEBUG_SEL = dbgsel;
}

#if 0

/***********************************************************/
// tx debug map
// IO  | Debug
// 3:0  fsm_pa_gain_vb[3:0]
// 4    fast_lock_done
// 5    pll_lock
// 6    fsm_out_tx_en
// 7    fsm_cal_clken
// 8    fsm_gain_cal_en
// 9    fsm_afc_en
// 10   fsm_tx_ldo_en
// 11   fsm_en_pll
// 12   fsm_in_tx_en

// rx debug map
// IO  | Debug
// 0    fsm_en_agc
// 1    fsm_cal_clken
// 2    fsm_en_pa
// 3    fsm_en_lna
// 4    fast_lock_done
// 5    pll_lock
// 6    fsm_bpf_cal_en
// 7    fsm_out_rx_en
// 8    fsm_afc_en
// 9    fsm_rx_ldo_en
// 10   fsm_en_pll
// 11   fsm_in_rx_en
/***********************************************************/
void sysdbg_sel_rf(uint8_t txrx)
{
    // 2'b01: tx debug port, 2'b10: rx debug port
    RF->DIG_CTRL.RF_DBG_SEL = (txrx & 0x03);
    
    SYSCFG->DBG_CTRL.SYS_DEBUG_SEL = SYS_DBG_RF;
}

/***********************************************************/
// Modem debug(debug_mode = 1) bit Map
// IO | func
// 0   mclk
// 1   reset_n
// 3:2 rate
// 4   tx_en
// 5   tx_data
// 6   tx_valid
// 7   receive_start
// 8   vb_decoder_flush
// 9   sync_pulse
// 10  rx_valid
// 11  rx_data
// 12  tx_invert
// 13  rx_invert
// 14  acc_invert
// 15  iq_invert
/***********************************************************/
// Modem debug(debug_mode = 3) bit Map
// IO | func
// 0   mclk
// 2:1 rate
// 3   receive_start
// 4   vb_decoder_flush
// 5   i_in
// 6   q_in
// 7   sync_pulse
// 8   rx_valid
// 9   rx_data
// 10  demod_ready
// 11  demod_clk
// 12  demod_data
/***********************************************************/
void sysdbg_sel_mdm(uint8_t mode)
{
    MDM->REG0.DEBUG_MODE = mode;
    
    SYSCFG->DBG_CTRL.SYS_DEBUG_SEL = SYS_DBG_MDM;
}

void sysdbg_sel_ble(uint32_t diag)
{
    // BB DIAG1 0x83, 0x8300, 0x8383
    (*(volatile uint32_t *)0x50000050) = diag;
    
    SYSCFG->DBG_CTRL.SYS_DEBUG_SEL = SYS_DBG_BLE;
}

#endif
