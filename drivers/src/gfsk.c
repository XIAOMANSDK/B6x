#include "regs.h"
#include "rom.h"
#include "gfsk.h"

#if (DBG_GFSK)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

void rf_gfsk_init(void)
{
    RF->FSM_DLY_CTRL0.Word = 0x0045E311;
    RF->FSM_DLY_CTRL1.Word = 0x07120628;
    RF->DATA_DLY_CTRL.Word = 0x0000238B;
    RF->BPFMIX_CTRL.Word   = 0x00437D90;
    RF->AGC_CTRL0.Word     = 0x050BD572;
    RF->AGC_CTRL1.Word     = 0x000966B2;
    RF->AGC_CTRL2.Word     = 0x00E0331A;
    
    //.BG_RES_TRIM = 0x14
    RF->ANA_TRIM.Word      = 0x07DA9351;
    RF->ANAMISC_CTRL1.Word = 0x00000880;
    RF->ANA_PWR_CTRL.Word  = 0x07FF7F08;
    RF->ANA_EN_CTRL.Word   = 0x00072000;
    RF->PLL_ANA_CTRL.Word  = 0x3180FE00;
    RF->RF_RSV             = 0x0000B800;
    
    RF->DIG_CTRL.Word      = 0x000092DB;
}

// 以2400M(dis=21, frach=0)为基准进行计算
// tx_freq_mhz:2402 ~ 2480
void rf_gfsk_tx_freq(uint16_t tx_freq_mhz)
{
    if (tx_freq_mhz > 2480)
        tx_freq_mhz = 2480;
    else if (tx_freq_mhz < 2402)
        tx_freq_mhz = 2402;
    
    uint8_t pll_dis   = ((tx_freq_mhz - 2400) >> 4) + 0x15;
    uint8_t pll_frach = ((tx_freq_mhz - 2400) & 0x0F);

    pll_frach        += (pll_frach & 0x01);
    
    
    RF->PLL_DYM_CTRL.Word = (0x0F << RF_SW_PA_GAIN_TARGET_LSB);
    btmr_delay(16, 200);
    
    RF->PLL_FREQ_CTRL.SW_PLL_DI_S     = (pll_dis & 0x1F);
    RF->PLL_FREQ_CTRL.SW_PLL_FRAC     = (uint32_t)((pll_frach & 0x0F) << 20);
    RF->PLL_DYM_CTRL.Word = ((0x01 << RF_SW_TX_EN_POS) | (0x0F << RF_SW_PA_GAIN_TARGET_LSB)); // soft cfg en
    btmr_delay(16, 400);
}
