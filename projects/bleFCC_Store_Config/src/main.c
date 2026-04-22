/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "sftmr.h"
#include "leds.h"
#include "uartRb.h"
#include "dbg.h"
#include "fcc.h"
#include "reg_rf.h"
#include "bledef.h"

/*
 * VARIABLES DEFINITIONS
 ****************************************************************************************
 */

Fcc_Config_t Fcc_Config;

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

extern void user_procedure(void);

static void sysInit(void)
{
    iwdt_disable();

    rcc_ble_en();
    rcc_adc_en();

    rcc_fshclk_set(FSH_CLK_DPSC42);
}

uint8_t *config_get(void)
{
    return (uint8_t *)&Fcc_Config;
}

bool config_set(uint8_t data, uint8_t offset)
{
    if (offset > CONFIG_OFFSET_MAX)
    {
        return false;
    }
    if ((offset == PA_OFFSET) && (data > CFG_PA_MAX))
    {
        return false;
    }
    if ((offset > PA_OFFSET) && (offset < VCO_ADJ_OFFSET) && (data > CFG_ADJ_MAX))
    {
        return false;
    }
    if ((offset == BG_RES_TRIM_OFFSET) && (data > CFG_BG_RES_MAX))
    {
        return false;
    }

    uint8_t *p = (uint8_t *)&Fcc_Config;
    p[offset] = data;
    return true;
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();

    uart1Rb_Init();
    debug("Start(rsn:%X)...\r\n", rsn);

    /* Load config from Flash XIP */
    Fcc_Config_p *config = (Fcc_Config_p *)(FLASH_BASE + CONFIG_STORE_OFFSET);
    memcpy((uint8_t *)&Fcc_Config, (uint8_t *)config, sizeof(Fcc_Config_t));
    debugHex((uint8_t *)&Fcc_Config, sizeof(Fcc_Config_t));

    /* Apply defaults if Flash is erased */
    if (Fcc_Config.tx_power == CFG_ERASED_BYTE)
    {
        Fcc_Config.tx_power = CFG_DEFAULT_TX_POWER;
    }
    if (Fcc_Config.cap == CFG_ERASED_BYTE)
    {
        Fcc_Config.cap = CFG_DEFAULT_CAP;
    }
    if (Fcc_Config.vco_adj == CFG_ERASED_BYTE)
    {
        Fcc_Config.vco_adj = CFG_DEFAULT_VCO_ADJ;
    }
    if (Fcc_Config.bg_res_trim != CFG_ERASED_BYTE)
    {
        RF->ANA_TRIM.BG_RES_TRIM = Fcc_Config.bg_res_trim;
    }

#if (LED_PLAY)
    sftmr_init();
    leds_init();
    leds_play(LED_FAST_BL);
#endif //(LED_PLAY)

    fcc_init();

    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = Fcc_Config.cap;
    rf_pa_set(Fcc_Config.tx_power);

    if (Fcc_Config.PLL_DAC_ADJ00 != CFG_ERASED_BYTE)
    {
        RF->PLL_DAC_TAB0.PLL_DAC_ADJ00 = Fcc_Config.PLL_DAC_ADJ00;
    }
    if (Fcc_Config.PLL_DAC_ADJ12 != CFG_ERASED_BYTE)
    {
        RF->PLL_DAC_TAB1.PLL_DAC_ADJ12 = Fcc_Config.PLL_DAC_ADJ12;
    }
    if (Fcc_Config.PLL_DAC_ADJ05 != CFG_ERASED_BYTE)
    {
        RF->PLL_DAC_TAB2.PLL_DAC_ADJ05 = Fcc_Config.PLL_DAC_ADJ05;
    }
    rf_dac_tab0_set(RF->PLL_DAC_TAB0.Word);
    rf_dac_tab1_set(RF->PLL_DAC_TAB1.Word);
    rf_dac_tab2_set(RF->PLL_DAC_TAB2.Word);
}

int main(void)
{
    sysInit();
    devInit();

    // Global Interrupt Enable
    GLOBAL_INT_START();

    // main loop
    while (1)
    {
#if (LED_PLAY)
        // SoftTimer Polling
        sftmr_schedule();
#endif //(LED_PLAY)

        // User's Procedure
        user_procedure();
    }
}
