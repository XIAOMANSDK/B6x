/**
 ****************************************************************************************
 *
 * @file speakerA.c
 *
 * @brief PWM speaker driver with DMA for GSM audio playback
 *
 * @details
 * Uses ATMR (advanced timer) in PWM mode with DMA to output audio samples
 * through a speaker connected to PA07/PA11 (push-pull pair).
 *
 * PWM configuration for 64 kHz output (8 kHz audio, 8x oversampling):
 * - System clock: 64 MHz
 * - Prescaler: 0 -> 64 MHz timer clock
 * - ARR: 999 -> 64 kHz PWM frequency
 * - Repetition counter: 7 -> 8 kHz effective sample rate
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"
#include "msbc.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// ATMR PWM+ output pin
#ifndef PA_ATMR_P
#define PA_ATMR_P             (7)
#endif

/// ATMR PWM- output pin (complementary, P+4)
#define PA_ATMR_N             (PA_ATMR_P + 4)

/// Map PA to ATMR channel index
#define PWM_ATMR_CH(pa)       (pa - 3)

#if ((PA_ATMR_P < 7) || (PA_ATMR_P > 9))
#error "PWM+ pin must be between PA07 and PA09"
#endif

/// DMA channel for speaker output
#ifndef SPEAKER_DMA_CHNL
#define SPEAKER_DMA_CHNL      (DMA_CH0)
#endif

/// DMA destination address for ATMR duty register
#define SPEAKER_DMA_PTR_ATMR  (DMA_PTR_ATMR_CH1 + ((PA_ATMR_P - 7) % 4) * 4)

/// Initialize DMA channel for ATMR update event
#define SPEAKER_DMA_ATMR_CHx_INIT(chidx)    dma_chnl_init(chidx, DMA_PID_ATMR_UP)

/// Configure DMA transfer (byte mode)
#define SPEAKER_DMA_ATMR_CHx_CONF(chidx, buff, len, ccm) \
    dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], SPEAKER_DMA_PTR_ATMR, \
                  TRANS_PER_WR(ccm, len, IN_BYTE, IN_BYTE))

/// Configure DMA transfer (half-word mode)
#define SPEAKER_DMA_ATMR_CHx_CONF_HALF(chidx, buff, len, ccm) \
    dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], SPEAKER_DMA_PTR_ATMR, \
                  TRANS_PER_WR(ccm, len, IN_HALF, IN_HALF))

/// Timer prescaler: 64 MHz / 1 = 64 MHz
#define SPEAKER_PWM_TMR_PSC   (1 - 1)

/// Auto-reload: 64 MHz / 1000 = 64 kHz PWM
#define SPEAKER_PWM_TMR_ARR   (1000 - 1)

/// Repetition counter: 64 kHz / 8 = 8 kHz sample rate
#define SPEAKER_PWM_TMR_REP   (8 - 1)

/*
 * VARIABLES
 ****************************************************************************************
 */

static volatile bool dma_done;  ///< DMA transfer complete flag

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize PWM speaker with DMA
 *
 * @details
 * Configure ATMR PWM output pins, timer prescaler/ARR, DMA channel,
 * and enable DMA completion interrupt.
 ****************************************************************************************
 */
void pwmInit(void)
{
    dma_init();

    GPIO_DIR_SET_HI(BIT(PA_ATMR_P));
    iom_ctrl(PA_ATMR_P, IOM_SEL_TIMER | IOM_DRV_LVL1);

    GPIO_DIR_SET_HI(BIT(PA_ATMR_N));
    iom_ctrl(PA_ATMR_N, IOM_SEL_TIMER | IOM_DRV_LVL1);

    /* ATMR PWM configuration */
    pwm_chnl_cfg_t chnl_conf;
    chnl_conf.ccmr = PWM_CCMR_MODE1;

    pwm_init(PWM_ATMR, SPEAKER_PWM_TMR_PSC, SPEAKER_PWM_TMR_ARR);

    chnl_conf.duty = SPEAKER_PWM_TMR_ARR / 2;
    chnl_conf.ccer = PWM_CCER_SIPH | PWM_CCxDE_BIT;

    pwm_chnl_set(PWM_ATMR_CH(PA_ATMR_P), &chnl_conf);
    pwm_chnl_set(PWM_ATMR_CH(PA_ATMR_N), &chnl_conf);

    pwm_start(PWM_ATMR);
    ATMR->DMAEN.UDE = 1;

    SPEAKER_DMA_ATMR_CHx_INIT(SPEAKER_DMA_CHNL);

    ATMR->CR1.URS = 1;
    ATMR->RCR = SPEAKER_PWM_TMR_REP;

    DMACHNL_INT_EN(SPEAKER_DMA_CHNL);
    NVIC_EnableIRQ(DMAC_IRQn);
    GLOBAL_INT_START();

    dma_done = true;
}

/**
 ****************************************************************************************
 * @brief DMA interrupt handler
 *
 * @details
 * Check DMA channel flags, signal transfer completion, and clear interrupts.
 ****************************************************************************************
 */
void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHCFG->IFLAG0;

    /* Disable further interrupts before clearing */
    DMACHCFG->IEFR0 &= ~iflag;
    DMACHCFG->ICFR0 = iflag;

    if (iflag & (1UL << SPEAKER_DMA_CHNL))
    {
        dma_done = true;
    }

    /* Re-enable interrupts */
    DMACHCFG->IEFR0 |= iflag;
}

/**
 ****************************************************************************************
 * @brief Play PCM samples through speaker via DMA
 *
 * @param buff   PCM sample buffer (16-bit)
 * @param samples Number of samples to play
 *
 * @details
 * Blocks until previous DMA transfer completes, then starts new transfer
 * in half-word mode to ATMR duty register.
 ****************************************************************************************
 */
void speakerPlay(int16_t *buff, uint16_t samples)
{
    if (samples)
    {
        while (!dma_done)
        {
        }
        SPEAKER_DMA_ATMR_CHx_CONF_HALF(SPEAKER_DMA_CHNL, (uint16_t *)buff, samples, CCM_BASIC);
        ATMR->DMAEN.UDE = 1;
        dma_done = false;
    }
}

/**
 ****************************************************************************************
 * @brief Stop speaker output (set 50% duty = silence)
 ****************************************************************************************
 */
void speakerStop(void)
{
    pwm_duty_upd(PWM_ATMR_CH(PA_ATMR_P), SPEAKER_PWM_TMR_ARR / 2);
}
