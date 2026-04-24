/**
 ****************************************************************************************
 *
 * @file sadc_test.c
 *
 * @brief SADC (ADC) API usage demo.
 *
 * @details This example demonstrates SADC module usage, including basic ADC reading
 *          and DMA transfer modes.
 *          Flow:
 *          1. Initialize GPIO pins as analog input
 *          2. Configure SADC analog parameters
 *          3. Optionally use DMA for data transfer or direct ADC read
 *          4. In DMA mode, data is sent via UART output
 *          5. In non-DMA mode, ADC values are read and printed directly
 *
 * @note The SADC module is a 10-bit ADC supporting 10 input channels.
 *        Different sampling modes support single-shot, continuous, and PCM audio sampling.
 *        DMA mode enables efficient data transfer with reduced CPU overhead.
 *        The analog front-end supports PGA and microphone bias circuit.
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
    #define DMA_CH_SADC            (0)                    ///< DMA channel number
    #define ADC_SW_AUTO            (0)                    ///< ADC auto channel switch: 0=single, ((2<<1)|0x01)=3-channel
    #define ADC_CH_CTRL            ((SADC_CH_AIN7<<0) | (SADC_CH_AIN8<<4) | (SADC_CH_AIN9<<8))

    #define GPIO_RX_PING           GPIO12                 ///< Ping buffer indicator GPIO
    #define GPIO_RX_PONG           GPIO13                 ///< Pong buffer indicator GPIO

    #if (SAMP_ADTMR)
    #define PWM_TMR_PSC            (0)                    ///< PWM timer prescaler
    #define PWM_TMR_ARR            (250 - 1)              ///< PWM timer ARR: 249=64K, 999=16K
    #define PWM_DUTY_DEFAULT       (50)                   ///< Default PWM duty cycle (%)
    #define PA_ADTMR_CH0           (PA14)                 ///< ADTMR channel 0 pin
    #define PA_ADTMR_CH1           (PA15)                 ///< ADTMR channel 1 pin
    #endif

    #if (SAMP_PCM)
    #define SAMP_NUM                (128)                 ///< PCM sample count
    static int16_t pcm_buff[SAMP_NUM * 2];                ///< PCM double buffer
    #else
    #define SAMP_NUM                (64)                  ///< ADC sample count
    static uint16_t adc_buff[SAMP_NUM];                   ///< ADC data buffer
    #endif
#endif //(DMA_USED)

#define ADC_READ_COUNT             (10)                   ///< Number of ADC reads in non-DMA mode

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Send data via UART byte-by-byte.
 *
 * @param[in] data Data pointer
 * @param[in] len  Data length
 ****************************************************************************************
 */
static void uart_tx_send(const uint8_t *data, uint16_t len)
{
    while (len--)
    {
        uart_putc(0, *data++);
    }
}

#if (SAMP_ADTMR)
/**
 ****************************************************************************************
 * @brief Configure ADTMR PWM to trigger ADC sampling.
 ****************************************************************************************
 */
static void adtmr_pwm(void)
{
    RCC_APBCLK_EN(APB_ATMR_BIT);
    iom_ctrl(PA_ADTMR_CH0, IOM_SEL_TIMER);

    pwm_init(PWM_ATMR, PWM_TMR_PSC, PWM_TMR_ARR);

    pwm_chnl_cfg_t chnl_conf;
    chnl_conf.duty = PWM_DUTY_DEFAULT;
    chnl_conf.ccer = PWM_CCER_SIPH | PWM_CCxDE_BIT;
    chnl_conf.ccmr = PWM_CCMR_MODE1;
    pwm_chnl_set(PWM_ATMR_CH0, &chnl_conf);

    pwm_start(PWM_ATMR);
}
#endif

/**
 ****************************************************************************************
 * @brief SADC test main function.
 *
 * @details Demonstrates SADC basic usage and DMA transfer.
 ****************************************************************************************
 */
void sadcTest(void)
{
    // Configure GPIO as input
    GPIO_DIR_CLR(GPIO08 | GPIO09 | GPIO10);

    // Configure ADC input pins as analog mode
    iom_ctrl(PA08, IOM_ANALOG);  // AIN7
    iom_ctrl(PA09, IOM_ANALOG);  // AIN8
    iom_ctrl(PA10, IOM_ANALOG);  // AIN9

    // SADC initialization
    #if (SAMP_PCM)
    sadc_init(SADC_ANA_DFLT | SADC_INBUF_BYPSS_BIT);  // PCM mode, bypass input buffer
    #else
    sadc_init(SADC_ANA_DFLT);  // Default analog config
    #endif

#if (DMA_USED)
    // Configure indicator GPIOs as output
    GPIO_DIR_SET_LO(GPIO_RX_PING | GPIO_RX_PONG);

    dma_init();
    DMA_SADC_INIT(DMA_CH_SADC);

    #if (SAMP_PCM)
    // PCM mode config
    DMA_SADC_PCM_CONF(DMA_CH_SADC, pcm_buff, SAMP_NUM, CCM_PING_PONG);
    DMA_SADC_PCM_CONF(DMA_CH_SADC | DMA_CH_ALT, pcm_buff + SAMP_NUM, SAMP_NUM, CCM_PING_PONG);
    sadc_conf((SADC_CR_DFLT & ~(SADC_CR_HPF_COEF_MSK | SADC_CR_CLK_DIV_MSK))
              | SADC_CR_HPF(3) | SADC_CR_CLK(0));
    sadc_pcm(SADC_MIC_DFLT & (~SADC_PGA_VOL_MSK | SADC_PGA_VOL(0)));
    #elif (SAMP_ADTMR)
    // ADTMR triggered mode config
    DMA_SADC_AUX_CONF(DMA_CH_SADC, adc_buff, SAMP_NUM, CCM_BASIC);
    sadc_atmr(ADC_SW_AUTO, ADC_CH_CTRL);
    #else
    // Normal DMA mode config
    DMA_SADC_AUX_CONF(DMA_CH_SADC, adc_buff, SAMP_NUM, CCM_BASIC);
    sadc_dma(ADC_SW_AUTO, ADC_CH_CTRL);
    #endif

    // Main loop - process DMA transfer events
    while (1)
    {
        if (dma_chnl_done(DMA_CH_SADC))
        {
            #if (SAMP_PCM)
            if (dma_chnl_reload(DMA_CH_SADC))
            {
                // Ping buffer
                GPIO_DAT_SET(GPIO_RX_PING);
                uart_tx_send((const uint8_t *)&pcm_buff, SAMP_NUM * 2);
                GPIO_DAT_CLR(GPIO_RX_PING);
            }
            else
            {
                // Pong buffer
                GPIO_DAT_SET(GPIO_RX_PONG);
                uart_tx_send((const uint8_t *)&pcm_buff[SAMP_NUM], SAMP_NUM * 2);
                GPIO_DAT_CLR(GPIO_RX_PONG);
            }
            #else
            debug("DMA-Read:\r\n");
            debugHex(adc_buff, SAMP_NUM);
            #endif
        }
    }
#else // !(DMA_USED)
    // Non-DMA mode - direct ADC read
    for (uint8_t i = 0; i < ADC_READ_COUNT; i++)
    {
        uint16_t adc_data = sadc_read(SADC_CH_AIN8, 0);
        debug("adc_data:0x%x\r\n", adc_data);
    }

    while (1) {}
#endif
}
