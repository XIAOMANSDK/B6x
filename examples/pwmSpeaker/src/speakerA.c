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
#include "dbg.h"
#include "music.h"
#include "msbc.h"
/*
 * DEFINES
 ****************************************************************************************
 */

//PWM+ -> PWM-, 固定组的IO. PWM差分方式.
//PA07 -> PA11
//PA08 -> PA12
//PA09 -> PA13
#ifndef PA_ATMR_P
#define PA_ATMR_P             (7)           // PWM+ PA07
#endif

#if (1) // 若使用差分功放芯片,则需开启此引脚.
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

//// 10KHz PCM数据 PWM配置 (最低采样率)
////SPEAKER_PWM_TMR ARR 255代表的一半，128，0x80，128/255=占空比为50%，其余数值处于
//#define SPEAKER_PWM_TMR_PSC (4 - 1)   //.	系统不分频	64MHz/4=16MHz
//#define SPEAKER_PWM_TMR_ARR (255 - 1) //	重载值	16MHz/255 = 62.7KHz(音频数据/重载值=占空比)	
//#define SPEAKER_PWM_TMR_REP (6- 1)    // 周期计数 6	62.7KHz/6 = 10.54KHz(采样率)

//// 16KHz PCM数据 PWM配置
//#define SPEAKER_PWM_TMR_PSC      (8 - 1)    // 系统分频     64MHz/8   = 8MH
//#define SPEAKER_PWM_TMR_ARR      (250 - 1)  // 重载值       8MHz/250  = 32KHz (音频数据/重载值 = 占空比)
//#define SPEAKER_PWM_TMR_REP      (2 - 1)    // 周期计数2    32KHz/2 = 16KHz   (采样率)

//// 32KHz PCM数据 PWM配置
//#define SPEAKER_PWM_TMR_PSC (1 - 1)    //.系统不分频	64MHz/1= 64MHz
//#define SPEAKER_PWM_TMR_ARR (2048 - 1) //重载值	64MHz/2048 = 32KHz(音频数据/重载值=占空比)
//#define SPEAKER_PWM_TMR_REP (1- 1)     // 周期计数 1 32KHz/1 = 32KHz(采样率)

// 64KHz PCM数据 PWM配置 (最高采样率)
#define SPEAKER_PWM_TMR_PSC (1 - 1)    // 系统不分频 64MHz/1= 64MHz
#define SPEAKER_PWM_TMR_ARR (1000 - 1) // 重载值 64MHz/1000 = 64KHz(音频数据/重载值=占空比)
#define SPEAKER_PWM_TMR_REP (1- 1)     // 周期计数 1 64KHz/1 = 64KHz(采样率)
/*
 * FUNCTIONS
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
    
    chnl_conf.duty = SPEAKER_PWM_TMR_ARR/2;  // 占空比50%-静音
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
}



// 数据转换/缓存.
int16_t buffA[2][1020];
static uint8_t buffFlag = 0;

// 16bitPCM转成11bitPCM,丢掉低6bit, 1bit符号位转换
void pcmTranslate(uint32_t ptr, uint32_t len)
{
    uint16_t toLen = 1020;
    if (len < 1020) toLen = len;
        
    for (uint16_t idx = 0; idx < toLen; idx++)
    {
        int16_t pcm16 = spealer_64Khz[ptr + idx];
        
        buffA[buffFlag][idx] = ((pcm16 + 0x8000) >> 6);
    }
    
//    uart_send(0, toLen*2, (uint8_t *)&buffA[buffFlag]);
}

void speakerPlay(void)
{
    // 音频总大小
    uint32_t length =  sizeof(spealer_64Khz)/2;
    uint32_t sendCnt = 0;
    
    pcmTranslate(sendCnt, length); // 数据格式转换.
    
    while (length /*&& gpio_get(PA17)*/)
    {
        if (length > 1020)
        {
            SPEAKER_DMA_ATMR_CHx_CONF_HALF(SPEAKER_DMA_CHNL, (uint16_t *)&buffA[buffFlag][0], 1020, CCM_BASIC);
            length -= 1020;
            sendCnt += 1020;
        }
        else
        {
            SPEAKER_DMA_ATMR_CHx_CONF_HALF(SPEAKER_DMA_CHNL, (uint16_t *)&buffA[buffFlag][0], (uint16_t)length, CCM_BASIC);
            length = 0;
        }
        ATMR->DMAEN.UDE = 1;
        
        GPIO_DIR_SET_HI(GPIO01);
        if(length)
        {
            if (buffFlag) buffFlag = 0;
            else  buffFlag = 1;
            pcmTranslate(sendCnt, length); // 数据格式转换.
        }
        GPIO_DIR_SET_LO(GPIO01);
        while(!dma_chnl_done(SPEAKER_DMA_CHNL));
        
        
        ATMR->DMAEN.UDE = 0;
    }
    
    pwm_duty_upd(PWM_ATMR_CH(PA_ATMR_P), SPEAKER_PWM_TMR_ARR/2);// 静音
}

