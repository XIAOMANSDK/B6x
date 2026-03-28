/**
 ****************************************************************************************
 *
 * @file speaker.c
 *
 * @brief PWM speaker driver for GSM audio output
 *
 * @details This driver uses ATMR (Advanced Timer) to generate PWM output for
 *          audio playback. It uses DMA (Channel 0) to transfer PCM samples
 *          to the PWM duty cycle register, providing interrupt-driven audio output.
 *
 *          Hardware Configuration:
 *          - PWM+ Pin: PA07 (configurable via PA_ATMR_P)
 *          - PWM- Pin: PA11 (differential output, PA_ATMR_P + 4)
 *          - DMA Channel: DMA_CH0
 *          - PWM Frequency: 64kHz carrier / 8 = 8kHz sample rate
 *
 *          DMA Interrupt Handling:
 *          - DMAC_IRQHandler is NOT defined here (moved to dma_handler.c)
 *          - speaker_dma_handler() callback is called from dma_handler.c
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"
#include "msbc.h"
#include "speaker.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// PWM+ -> PWM-, differential IO. PWM differential output.
//PA07 -> PA11
//PA08 -> PA12
//PA09 -> PA13
#ifndef PA_ATMR_P
#define PA_ATMR_P             (8)           // PWM+ PA07
#endif

#if (1) // Enable differential resistor chip, please configure differential.
#define PA_ATMR_N             (PA_ATMR_P+4) // PWM-
#endif

#define PWM_ATMR_CH(pa)       (pa-3) // @see pwm_channel

#if ((PA_ATMR_P < 7) || (PA_ATMR_P > 9))
    #error "PWM+ PIN MUST BE BETWEEN PA07 AND PA09 !!!"
#endif

#ifndef SPEAKER_DMA_CHNL
#define SPEAKER_DMA_CHNL             (DMA_CH0)
#endif

#define SPEAKER_DMA_PTR_ATMR   (DMA_PTR_ATMR_CH1 + ((PA_ATMR_P - 7)%4)*4)
#define SPEAKER_DMA_ATMR_CHx_INIT(chidx)    dma_chnl_init(chidx, DMA_PID_ATMR_UP)
#define SPEAKER_DMA_ATMR_CHx_CONF(chidx, buff, len, ccm) \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], SPEAKER_DMA_PTR_ATMR, TRANS_PER_WR(ccm, len, IN_BYTE, IN_BYTE))

#define SPEAKER_DMA_ATMR_CHx_CONF_HALF(chidx, buff, len, ccm) \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], SPEAKER_DMA_PTR_ATMR, TRANS_PER_WR(ccm, len, IN_HALF, IN_HALF))

// 64KHz PCM to PWM conversion (highest quality)
#define SPEAKER_PWM_TMR_PSC (1 - 1)    // System clock 64MHz/1= 64MHz
#define SPEAKER_PWM_TMR_ARR (1000 - 1) // Auto-reload 64MHz/1000 = 64KHz(Freq/ARR=DutyRatio)
#define SPEAKER_PWM_TMR_REP (8- 1)     // Repeat count 1 64KHz/8 = 8KHz(SampleRate)

volatile uint8_t dam_done;

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize PWM speaker for audio output
 *
 * @details Initializes ATMR peripheral for PWM generation with DMA support:
 *          - Configures GPIO pins for PWM output (PA07/PA11)
 *          - Sets up ATMR for 64kHz PWM carrier frequency
 *          - Configures DMA Channel 0 for automatic sample transfer
 *          - Enables DMA interrupt for completion notification
 *
 *          Timing: 64MHz system clock, PWM carrier 64kHz,
 *                  divided by 8 = 8kHz sample rate (GSM standard)
 ****************************************************************************************
 */
void pwmInit(void)
{
    dma_init();

    GPIO_DIR_SET_HI(BIT(PA_ATMR_P));
    iom_ctrl(PA_ATMR_P, IOM_SEL_TIMER | IOM_DRV_LVL1);

    #if (PA_ATMR_N)
    GPIO_DIR_SET_HI(BIT(PA_ATMR_N));
    iom_ctrl(PA_ATMR_N, IOM_SEL_TIMER | IOM_DRV_LVL1);
    #endif

    pwm_chnl_cfg_t chnl_conf;
    chnl_conf.ccmr = PWM_CCMR_MODE1;

    // ADTMR
    pwm_init(PWM_ATMR, SPEAKER_PWM_TMR_PSC, SPEAKER_PWM_TMR_ARR);

    chnl_conf.duty = SPEAKER_PWM_TMR_ARR/2;  // Duty cycle 50%-mute
    chnl_conf.ccer = PWM_CCER_SIPH | PWM_CCxDE_BIT; // DMA_EN
    // duty/(arr+1) = 50%(Duty Ratio). 50% high, 50% low
    pwm_chnl_set(PWM_ATMR_CH(PA_ATMR_P), &chnl_conf);// PWM+

    #if (PA_ATMR_N)
    pwm_chnl_set(PWM_ATMR_CH(PA_ATMR_N), &chnl_conf);// PWM-
    #endif

    pwm_start(PWM_ATMR);
    ATMR->DMAEN.UDE = 1;

    SPEAKER_DMA_ATMR_CHx_INIT(SPEAKER_DMA_CHNL);

    ATMR->CR1.URS = 1;
    ATMR->RCR = SPEAKER_PWM_TMR_REP;

    DMACHNL_INT_EN(SPEAKER_DMA_CHNL);
    // Note: NVIC_EnableIRQ(DMAC_IRQn) is done in uart1Rb_Init()

    GLOBAL_INT_START();

    dam_done = true;
}

/**
 ****************************************************************************************
 * @brief DMA completion callback for speaker
 * @details Called from unified DMAC_IRQHandler in dma_handler.c
 *          Sets dam_done flag to signal that DMA transfer is complete
 ****************************************************************************************
 */
void speaker_dma_handler(void)
{
    dam_done = true;
}

/**
 ****************************************************************************************
 * @brief Play audio samples via PWM speaker
 *
 * @param buff Pointer to 16-bit PCM samples (signed)
 * @param samples Number of samples to play (typically 160 for GSM frame)
 *
 * @details This function:
 *          1. Waits for previous DMA transfer to complete (dam_done)
 *          2. Configures DMA to transfer samples to PWM duty cycle register
 *          3. Starts DMA transfer
 *          4. Resets dam_done flag for next transfer
 *
 *          Note: Blocks until dam_done is set from previous transfer
 ****************************************************************************************
 */
void speakerPlay(int16_t *buff, uint16_t samples)
{
    if(samples)
    {
        while(!dam_done);
        SPEAKER_DMA_ATMR_CHx_CONF_HALF(SPEAKER_DMA_CHNL, (uint16_t *)buff, samples, CCM_BASIC);
        ATMR->DMAEN.UDE = 1;
        dam_done = false;
    }
}

/**
 ****************************************************************************************
 * @brief Stop speaker output (mute)
 *
 * @details Sets PWM duty cycle to 50% (mute/center position)
 *          This effectively stops audio output by holding at mid-scale
 ****************************************************************************************
 */
void speakerStop(void)
{
    //pwm_duty_upd(PWM_ATMR_CH(PA_ATMR_P), SPEAKER_PWM_TMR_ARR/2);// mute
    pwm_duty_upd(PWM_ATMR_CH(PA_ATMR_P), 0);
}

/**
 ****************************************************************************************
 * @brief Check if speaker DMA transfer is complete
 * @return 1 if DMA is done (ready for next transfer), 0 if busy
 *
 * @details Non-blocking check for DMA completion status.
 *          Used for ping-pong buffer implementation to avoid CPU blocking.
 ****************************************************************************************
 */
uint8_t speaker_is_ready(void)
{
    return dam_done;
}

/**
 ****************************************************************************************
 * @brief Non-blocking speaker play
 *
 * @param buff Pointer to 16-bit PCM samples (signed)
 * @param samples Number of samples to play
 *
 * @details This function assumes dam_done is already set (speaker_is_ready() returned true).
 *          Does NOT wait for previous transfer to complete.
 *          Used for ping-pong buffer implementation to minimize CPU blocking.
 *
 * @note  Call speaker_is_ready() first to ensure DMA is ready
 ****************************************************************************************
 */
void speakerPlay_nonblock(int16_t *buff, uint16_t samples)
{
    if (samples && dam_done)
    {
        SPEAKER_DMA_ATMR_CHx_CONF_HALF(SPEAKER_DMA_CHNL, (uint16_t *)buff, samples, CCM_BASIC);
        ATMR->DMAEN.UDE = 1;
        dam_done = false;
    }
}
