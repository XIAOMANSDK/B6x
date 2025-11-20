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
Fcc_Config_t Fcc_Config;
/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
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


uint8_t* get_config(void)
{
    return((uint8_t*)&Fcc_Config);
}
bool Set_Config(uint8_t data,uint8_t offset)
{
    if(offset>CONFIG_OFFSET_MAX)
        return(0);
    if((offset==PA_OFFSET)&&(data>0x0f))
        return(0);
    if((offset>PA_OFFSET)&&(offset<VCO_ADJ_OFFSET)&&(data>0x3f))
        return(0);
    if((offset==BG_RES_TRIM_OFFSET)&&(data>0x1f))
        return(0);
    uint8_t *p = (uint8_t*)&Fcc_Config;    
//    if(p[offset]==data)return(0);
    p[offset]=data;
    return(1);
}
static void devInit(void)
{
    uint16_t rsn = rstrsn();

    uart1Rb_Init(); //dbgInit();
    debug("Start(rsn:%X)...\r\n", rsn);
    Fcc_Config_p *config =( Fcc_Config_p*)(0x18000000+CONFIG_STORE_OFFESET);
    memcpy((uint8_t*)&Fcc_Config,(uint8_t *)config,sizeof(Fcc_Config_t));
    debugHex((uint8_t*)&Fcc_Config,sizeof(Fcc_Config_t));
    if(Fcc_Config.tx_power==0xff)Fcc_Config.tx_power =0x0b;
    if(Fcc_Config.cap==0xff)Fcc_Config.cap =0x14;
    if(Fcc_Config.vco_adj==0xff)Fcc_Config.vco_adj =0x03;
    if(Fcc_Config.bg_res_trim!=0xff)RF->ANA_TRIM.BG_RES_TRIM = Fcc_Config.bg_res_trim;
    //uint8_t *pagef00 = (uint8_t *)(0x18000000+0xf00);
    //debug("PAGE INFO\r\n");
    //debugHex(pagef00,256);
    #if (LED_PLAY)
    sftmr_init();
    leds_init();
    leds_play(LED_FAST_BL);
    #endif //(LED_PLAY)

    fcc_init();
    
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = Fcc_Config.cap;
    
    rf_pa_set(Fcc_Config.tx_power);
    if(Fcc_Config.PLL_DAC_ADJ00!=0xff)RF->PLL_DAC_TAB0.PLL_DAC_ADJ00 = Fcc_Config.PLL_DAC_ADJ00;//Fcc_Config.PLL_DAC_ADJ00 =0;
    if(Fcc_Config.PLL_DAC_ADJ12!=0xff)RF->PLL_DAC_TAB1.PLL_DAC_ADJ12 = Fcc_Config.PLL_DAC_ADJ12;//Fcc_Config.PLL_DAC_ADJ12 =0;
    if(Fcc_Config.PLL_DAC_ADJ05!=0xff)RF->PLL_DAC_TAB2.PLL_DAC_ADJ05 = Fcc_Config.PLL_DAC_ADJ05;//Fcc_Config.PLL_DAC_ADJ05 =0;
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
