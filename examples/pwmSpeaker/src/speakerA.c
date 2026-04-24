/**
 ****************************************************************************************
 *
 * @file speakerA.c
 *
 * @brief PWM Speaker driver using ATMR + DMA for audio playback
 *
 * @details
 * Converts 16-bit PCM audio to PWM duty cycle values and plays via DMA.
 * PWM carrier: 64KHz, sample rate: 8KHz (REP=8)
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"
#include "music.h"
#include "msbc.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Differential PWM output: PWM+ -> PWM- (fixed 4-pad offset)
// PA07 -> PA11, PA08 -> PA12, PA09 -> PA13
#ifndef PA_ATMR_P
#define PA_ATMR_P             (7)            ///< PWM+ pin
#endif

#define PA_ATMR_N             (PA_ATMR_P + 4) ///< PWM- pin (differential)

#define PWM_ATMR_CH(pa)       (pa - 3)       ///< @see pwm_channel

#if ((PA_ATMR_P < 7) || (PA_ATMR_P > 9))
    #error "PWM+ PIN MUST BE BETWEEN PA07 AND PA09 !!!"
#endif

#ifndef SPEAKER_DMA_CHNL
#define SPEAKER_DMA_CHNL             (DMA_CH0)
#endif

#define SPEAKER_DMA_PTR_ATMR   (DMA_PTR_ATMR_CH1 + ((PA_ATMR_P - 7) % 4) * 4)

#define SPEAKER_DMA_ATMR_CHx_INIT(chidx)    dma_chnl_init(chidx, DMA_PID_ATMR_UP)

#define SPEAKER_DMA_ATMR_CHx_CONF(chidx, buff, len, ccm) \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], SPEAKER_DMA_PTR_ATMR, \
            TRANS_PER_WR(ccm, len, IN_BYTE, IN_BYTE))

#define SPEAKER_DMA_ATMR_CHx_CONF_HALF(chidx, buff, len, ccm) \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], SPEAKER_DMA_PTR_ATMR, \
            TRANS_PER_WR(ccm, len, IN_HALF, IN_HALF))

/// DMA block size (max for 10-bit N_MINUS_1 field minus margin)
#define SPEAKER_DMA_BLOCK_SIZE    (1020)

/// PCM conversion: 16-bit signed -> 10-bit unsigned
#define SPEAKER_PCM_OFFSET        (0x8000)
#define SPEAKER_PCM_SHIFT         (6)

// Alternative PWM timer configs (uncomment to change sample rate):
// 10KHz: PSC=3,  ARR=255,  REP=6 -> 64MHz/4=16MHz, 16MHz/255=62.7KHz, 62.7KHz/6=10.54KHz
// 16KHz: PSC=7,  ARR=250,  REP=2 -> 64MHz/8=8MHz,  8MHz/250=32KHz,   32KHz/2=16KHz
// 32KHz: PSC=0,  ARR=2047, REP=1 -> 64MHz/2048=32KHz, 32KHz/1=32KHz

// 64KHz carrier, 8KHz sample rate (default)
#define SPEAKER_PWM_TMR_PSC   (1 - 1)     ///< System clock: 64MHz
#define SPEAKER_PWM_TMR_ARR   (1000 - 1)  ///< Carrier: 64MHz/1000 = 64KHz
#define SPEAKER_PWM_TMR_REP   (8 - 1)     ///< Sample rate: 64KHz/8 = 8KHz


/*
 * GLOBALS
 ****************************************************************************************
 */

static int16_t buffA[2][SPEAKER_DMA_BLOCK_SIZE];  ///< PCM double buffer
static uint8_t buffFlag = 0;                       ///< Current buffer index


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Convert 16-bit signed PCM to 10-bit unsigned for PWM duty cycle
 *
 * @param[in] ptr  Sample index offset in audio data
 * @param[in] len  Number of samples to convert
 ****************************************************************************************
 */
static void pcm_translate(uint32_t ptr, uint32_t len)
{
    uint16_t toLen = SPEAKER_DMA_BLOCK_SIZE;
    if (len < SPEAKER_DMA_BLOCK_SIZE) toLen = len;

    for (uint16_t idx = 0; idx < toLen; idx++)
    {
        int16_t pcm16 = speaker_64Khz[ptr + idx];
        buffA[buffFlag][idx] = ((pcm16 + SPEAKER_PCM_OFFSET) >> SPEAKER_PCM_SHIFT);
    }
}

/**
 ****************************************************************************************
 * @brief Initialize PWM speaker (ATMR + DMA)
 *
 * @details Configures ATMR in PWM Mode 1 with 50% idle duty, enables DMA for updates
 ****************************************************************************************
 */
void speaker_pwm_init(void)
{
    dma_init();

    GPIO_DIR_SET_HI(BIT(PA_ATMR_P));
    iom_ctrl(PA_ATMR_P, IOM_SEL_TIMER | IOM_DRV_LVL1);

    GPIO_DIR_SET_HI(BIT(PA_ATMR_N));
    iom_ctrl(PA_ATMR_N, IOM_SEL_TIMER | IOM_DRV_LVL1);

    pwm_chnl_cfg_t chnl_conf;
    chnl_conf.ccmr = PWM_CCMR_MODE1;

    // ATMR: 50% duty = silence center point
    pwm_init(PWM_ATMR, SPEAKER_PWM_TMR_PSC, SPEAKER_PWM_TMR_ARR);

    chnl_conf.duty = SPEAKER_PWM_TMR_ARR / 2;
    chnl_conf.ccer = PWM_CCER_SIPH | PWM_CCxDE_BIT; // DMA_EN
    pwm_chnl_set(PWM_ATMR_CH(PA_ATMR_P), &chnl_conf);
    pwm_chnl_set(PWM_ATMR_CH(PA_ATMR_N), &chnl_conf);

    pwm_start(PWM_ATMR);
    ATMR->DMAEN.UDE = 1;

    SPEAKER_DMA_ATMR_CHx_INIT(SPEAKER_DMA_CHNL);

    ATMR->CR1.URS = 1;
    ATMR->RCR = SPEAKER_PWM_TMR_REP;
}

/**
 ****************************************************************************************
 * @brief Play audio data via PWM+DMA
 *
 * @details Translates PCM and streams via DMA in blocks using double buffering
 ****************************************************************************************
 */
void speaker_play(void)
{
    uint32_t length = sizeof(speaker_64Khz) / 2;
    uint32_t sendCnt = 0;

    pcm_translate(sendCnt, length);

    while (length)
    {
        if (length > SPEAKER_DMA_BLOCK_SIZE)
        {
            SPEAKER_DMA_ATMR_CHx_CONF_HALF(SPEAKER_DMA_CHNL,
                (uint16_t *)&buffA[buffFlag][0], SPEAKER_DMA_BLOCK_SIZE, CCM_BASIC);
            length -= SPEAKER_DMA_BLOCK_SIZE;
            sendCnt += SPEAKER_DMA_BLOCK_SIZE;
        }
        else
        {
            SPEAKER_DMA_ATMR_CHx_CONF_HALF(SPEAKER_DMA_CHNL,
                (uint16_t *)&buffA[buffFlag][0], (uint16_t)length, CCM_BASIC);
            length = 0;
        }

        ATMR->DMAEN.UDE = 1;

        // Prepare next buffer while DMA transfers current
        GPIO_DIR_SET_HI(GPIO01);
        if (length)
        {
            buffFlag ^= 1;
            pcm_translate(sendCnt, length);
        }
        GPIO_DIR_SET_LO(GPIO01);

        while (!dma_chnl_done(SPEAKER_DMA_CHNL));

        ATMR->DMAEN.UDE = 0;
    }

    // Return to silence (50% duty)
    pwm_duty_upd(PWM_ATMR_CH(PA_ATMR_P), SPEAKER_PWM_TMR_ARR / 2);
}
