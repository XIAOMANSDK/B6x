/**
 ****************************************************************************************
 *
 * @file sadc_test.c
 *
 * @brief SADC API usage demo.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#if (DMA_USED)
    #define DMA_CH_SADC            (0)
    #define ADC_SW_AUTO            (0) // 0 - single, ((2 << 1) | 0x01) - 3ch switch 
    #define ADC_CH_CTRL            ((SADC_CH_AIN7<<0) | (SADC_CH_AIN8<<4) | (SADC_CH_AIN9<<8))

    #define GPIO_RX_PING           GPIO12
    #define GPIO_RX_PONG           GPIO13
    #if (SAMP_ADTMR)
    #define PWM_TMR_PSC            (0)
    #define PWM_TMR_ARR            (250 - 1) //249 64K, 999 16K
    #define PA_ADTMR_CH0           (PA14)
    #define PA_ADTMR_CH1           (PA15)
    #endif

    #if (SAMP_PCM)
    #define SAMP_NUM                (128)
    int16_t pcm_buff[SAMP_NUM*2];   // ping-pong
    #else
    #define SAMP_NUM                (64)
    uint16_t adc_buff[SAMP_NUM];
    #endif
#endif //(DMA_USED)

void uartTxSend(const uint8_t *data, uint16_t len)
{
    while(len--)
    {
        uart_putc(0, *data++);
    }
}
/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (SAMP_ADTMR)
static void adtmr_pwm(void)
{
    RCC_APBCLK_EN(APB_ADTMR1_BIT);
    iom_ctrl(PA_ADTMR_CH0, IOM_SEL_TIMER);

    pwm_init(PWM_ADTMR, PWM_TMR_PSC, PWM_TMR_ARR);
    
    pwm_chnl_cfg_t chnl_conf;
    chnl_conf.duty = 50;
    chnl_conf.ccer = PWM_CCER_SIPH | PWM_CCxDE_BIT;
    chnl_conf.ccmr = PWM_CCMR_MODE1;
    pwm_chnl_set(PWM_ADTMR_CH0, &chnl_conf);
    
    pwm_start(PWM_CTMR);
}
#endif

void sadcTest(void)
{
    // Analog Enable
    GPIO_DIR_CLR(GPIO08 | GPIO09 | GPIO10);

    // ADC
    iom_ctrl(PA08, IOM_ANALOG);
    iom_ctrl(PA09, IOM_ANALOG);
    iom_ctrl(PA10, IOM_ANALOG);
    
    // sadc init
    #if (SAMP_PCM)
    sadc_init(SADC_ANA_DFLT | SADC_INBUF_BYPSS_BIT);
    #else
    sadc_init(SADC_ANA_DFLT);
    #endif
    
#if (DMA_USED) 
    GPIO_DIR_SET_LO(GPIO_RX_PING | GPIO_RX_PONG);
    
    dma_init();
    
    DMA_SADC_INIT(DMA_CH_SADC);
    #if (SAMP_PCM)
    DMA_SADC_PCM_CONF(DMA_CH_SADC, pcm_buff, SAMP_NUM, CCM_PING_PONG);
    DMA_SADC_PCM_CONF(DMA_CH_SADC | DMA_CH_ALT, pcm_buff+SAMP_NUM, SAMP_NUM, CCM_PING_PONG);
    // .SADC_CLK_PH=1, .SADC_HPF_COEF=3, .SADC_AUX_CLK_DIV=0, .SADC_CONV_MODE=0
    sadc_conf((SADC_CR_DFLT & ~(SADC_CR_HPF_COEF_MSK|SADC_CR_CLK_DIV_MSK)) | SADC_CR_HPF(3) | SADC_CR_CLK(0));
    sadc_pcm(SADC_MIC_DFLT & (~SADC_PGA_VOL_MSK | SADC_PGA_VOL(0)));
    #elif (SAMP_ADTMR)
    DMA_SADC_AUX_CONF(DMA_CH_SADC, adc_buff, SAMP_NUM, CCM_BASIC);
    sadc_adtmr(ADC_SW_AUTO, ADC_CH_CTRL);
    #else
    DMA_SADC_AUX_CONF(DMA_CH_SADC, adc_buff, SAMP_NUM, CCM_BASIC);
    sadc_dma(ADC_SW_AUTO, ADC_CH_CTRL);
    #endif
    
    while (1)
    {
        if (dma_chnl_done(DMA_CH_SADC))
        {
            #if (SAMP_PCM)
            if (dma_chnl_reload(DMA_CH_SADC))
            {
                // Ping done pulse
                GPIO_DAT_SET(GPIO_RX_PING);
//                debug("PCM-Ping:\r\n"); 
                uartTxSend((uint8_t *)&pcm_buff, SAMP_NUM*2);                
                GPIO_DAT_CLR(GPIO_RX_PING);                
            }
            else
            {
                // Pong done pulse
                GPIO_DAT_SET(GPIO_RX_PONG);
//                debug("PCM-Pong:\r\n");
                uartTxSend((uint8_t *)&pcm_buff[SAMP_NUM], SAMP_NUM*2);                
                GPIO_DAT_CLR(GPIO_RX_PONG);                            
            }
            #else
            debug("DMA-Read:\r\n");
            debugHex(adc_buff, SAMP_NUM);
            #endif
        }
    }
#else // !(DMA_USED)
    for (uint8_t i = 0; i < 10; i++)
    {
        uint16_t adc_data = sadc_read(SADC_CH_AIN8, 0);
        debug("adc_data:0x%x\r\n", adc_data);
    }
    
    while (1);
#endif
}
