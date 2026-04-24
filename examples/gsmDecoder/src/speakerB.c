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
#include "msbc.h"
/*
 * DEFINES
 ****************************************************************************************
 */

//PWM+ -> PWM-, 语音非差分方式, 需要避开差分组引脚.
//PA07 -> PA11 (避开一起使用)
//PA08 -> PA12 (避开一起使用)
//PA09 -> PA13 (避开一起使用)

#ifndef PA_ATMR_P
#define PA_ATMR_P             (7)           // PWM+ PA07
#endif
#define PA_ATMR_N             (10)           // PWM-

#define PWM_ATMR_CH(pa)       (pa-3) // @see pwm_channel

#if ((PA_ATMR_P < 7) || (PA_ATMR_P > 9))
    #error "PWM+ PIN MUST BE BETWEEN PA07 AND PA09 !!!"
#endif

#ifndef SPEAKER_DMA_CHNL
#define SPEAKER_DMA_CHNL             (DMA_CH0)
#define SPEAKER_DMA_CHNL_N           (DMA_CH1)
#endif

#define SPEAKER_DMA_PTR_ATMR   (DMA_PTR_ATMR_CH1 + ((PA_ATMR_P - 7)%4)*4)
#define SPEAKER_DMA_ATMR_CHx_INIT(chidx)    dma_chnl_init(chidx, DMA_PID_ATMR_UP)
#define SPEAKER_DMA_ATMR_CHx_CONF(chidx, buff, len, ccm) \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], DMA_PTR_ATMR_CH1, TRANS_PER_WR(ccm, len, IN_BYTE, IN_BYTE))
#define SPEAKER_DMA_ATMR_CHx_CONF_HALF(chidx, buff, len, ccm) \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], DMA_PTR_ATMR_CH1, TRANS_PER_WR(ccm, len, IN_HALF, IN_HALF))            
#define SPEAKER_DMA_ATMR_CHx_CONF_N(chidx, buff, len, ccm) \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], DMA_PTR_ATMR_CH4, TRANS_PER_WR(ccm, len, IN_BYTE, IN_BYTE))
#define SPEAKER_DMA_ATMR_CHx_CONF_N_HALF(chidx, buff, len, ccm) \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], DMA_PTR_ATMR_CH4, TRANS_PER_WR(ccm, len, IN_HALF, IN_HALF))
//// 7KHz PCM数据 PWM配置(蜂鸣声, 明显)
//#define SPEAKER_PWM_TMR_PSC      (16 - 1)   // 系统不分频   16MHz/16   = 1MHz
//#define SPEAKER_PWM_TMR_ARR      (142 - 1)  // 重载值       16MHz/142 = 7KHz  (音频数据/重载值 = 占空比)
//#define SPEAKER_PWM_TMR_REP      (1 - 1)    // 周期计数     7KHz/1 = 7KHz(采样率-更新率)

// 7KHz PCM数据 PWM配置
//#define SPEAKER_PWM_TMR_PSC      (1 - 1)    // 系统不分频   16MHz/1   = 16MHz
//#define SPEAKER_PWM_TMR_ARR      (254 - 1)  // 重载值       16MHz/254 = 63KHz  (音频数据/重载值 = 占空比)
//#define SPEAKER_PWM_TMR_REP      (9 - 1)    // 周期计数     63KHz/9 = 7KHz(采样率-更新率)

//// 16KHz PCM数据 PWM配置
//#define SPEAKER_PWM_TMR_PSC      (2 - 1)    // 系统分频     16MHz/2   = 8MH
//#define SPEAKER_PWM_TMR_ARR      (250 - 1)  // 重载值       8MHz/250  = 32KHz  (音频数据/重载值 = 占空比)
//#define SPEAKER_PWM_TMR_REP      (2 - 1)    // 周期计数2    32KHz/2 = 16KHz(采样率-更新率)

// 14KHz PCM数据 PWM配置
//#define SPEAKER_PWM_TMR_PSC      (3 - 1)    // 系统分频     64MHz/3   = 21.3MH
//#define SPEAKER_PWM_TMR_ARR      (254 - 1)  // 重载值       21.3MHz/254  = 84KHz (音频数据/重载值 = 占空比)
//#define SPEAKER_PWM_TMR_REP      (6 - 1)    // 周期计数6    84KHz/6 = 14KHz   (采样率)

// 64KHz PCM
#define SPEAKER_PWM_TMR_PSC (1 - 1)    // 64M
#define SPEAKER_PWM_TMR_ARR (1000 - 1) // 32K
#define SPEAKER_PWM_TMR_REP (8 - 1)     // 32K/4=8K

static volatile uint8_t dam_done;
/*
 * FUNCTIONS
 ****************************************************************************************
 */

void pwmInit(void)
{
    dma_init();
    
    GPIO_DIR_SET_HI(BIT(PA_ATMR_P));
    GPIO_DIR_SET_HI(BIT(PA_ATMR_N));
    
    iom_ctrl(PA_ATMR_P, IOM_SEL_TIMER | IOM_DRV_LVL1);
    iom_ctrl(PA_ATMR_N, IOM_SEL_TIMER | IOM_DRV_LVL1);
    
    pwm_chnl_cfg_t chnl_conf;
    chnl_conf.ccmr = PWM_CCMR_MODE1;
    
    // ADTMR
    pwm_init(PWM_ATMR, SPEAKER_PWM_TMR_PSC, SPEAKER_PWM_TMR_ARR);
    
    chnl_conf.duty = 0;//PWM_TMR_ARR/2;
    chnl_conf.ccer = PWM_CCER_SIPH | PWM_CCxDE_BIT; // DMA_EN
    // duty/(arr+1) = 50%(Duty Ratio). 50% high, 50% low
    pwm_chnl_set(PWM_ATMR_CH(PA_ATMR_P), &chnl_conf);// PWM+
    
    pwm_chnl_set(PWM_ATMR_CH(PA_ATMR_N), &chnl_conf);// PWM-
    
    pwm_start(PWM_ATMR);
    ATMR->DMAEN.UDE = 1;
    
    SPEAKER_DMA_ATMR_CHx_INIT(SPEAKER_DMA_CHNL);
    SPEAKER_DMA_ATMR_CHx_INIT(SPEAKER_DMA_CHNL_N);
    
    ATMR->CR1.URS = 1;
    ATMR->RCR = SPEAKER_PWM_TMR_REP;
    
    DMACHNL_INT_EN(SPEAKER_DMA_CHNL); 
    NVIC_EnableIRQ(DMAC_IRQn);  ///< ???DMA???????ж?
    GLOBAL_INT_START();
    
    dam_done = true;
}

void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHCFG->IFLAG0;  
    
    DMACHCFG->IEFR0 &= ~iflag;
    DMACHCFG->ICFR0 = iflag;
    
    if (iflag & (1UL << SPEAKER_DMA_CHNL))
    {
        dam_done = true;
    }

    DMACHCFG->IEFR0 |= iflag;
    
}

// 非差分方式,需要数据转换.
uint16_t buffA[2][160], buffB[2][160];

static uint8_t buffFlag = 0;

void pcmToAB(const uint16_t *data, uint32_t len)
{
        
    for (uint16_t idx = 0; idx < len; idx++)
    {
        if (data[idx] < 502)
        {
            buffA[buffFlag][idx] = 0x01;
        }
        else
        {
            buffA[buffFlag][idx] = ((data[idx] - 500))*2;
        }
        
        if (data[idx] > 498)
        {
            buffB[buffFlag][idx] = 0x01;
        }
        else
        {
            buffB[buffFlag][idx] = ((499 - data[idx]))*2;
        }
    
    }
}

void speakerPlay(int16_t *buff, uint16_t samples)
{
    // 恢复 PWM 控制模式
    iom_ctrl(PA_ATMR_P, IOM_SEL_TIMER | IOM_DRV_LVL1);
    iom_ctrl(PA_ATMR_N, IOM_SEL_TIMER | IOM_DRV_LVL1);
    pwm_start(PWM_ATMR);
    
//    debug("Frame: ");
//    for (int i = 0; i < samples; i++)
//    {
//        debug("0x%04x ", (uint16_t)buff[i]);
//    }
//    debug("\r\n");  
    
    pcmToAB(buff, samples);
//    debug("buffA: ");
//    for (int i = 0; i < samples; i++)
//    {
//        debug("0x%04x ", buffA[buffFlag][i]);
//    }
//    debug("\r\n"); 
//    debug("buffB: ");
//    for (int i = 0; i < samples; i++)
//    {
//        debug("0x%04x ", buffB[buffFlag][i]);
//    }
//    debug("\r\n");
    
    while(!dam_done);
    SPEAKER_DMA_ATMR_CHx_CONF_HALF(SPEAKER_DMA_CHNL, buffA[buffFlag], samples, CCM_BASIC);
    SPEAKER_DMA_ATMR_CHx_CONF_N_HALF(SPEAKER_DMA_CHNL_N, buffB[buffFlag], samples, CCM_BASIC);
    ATMR->DMAEN.UDE = 1;
    dam_done = false;
    ++buffFlag;
    buffFlag = buffFlag%2; 
}
void speakerStop(void)
{
    pwm_stop(PWM_ATMR);
    // 关闭电源(INIT GPIO Default Out Put)
    iom_ctrl(PA_ATMR_P, IOM_SEL_GPIO | IOM_DRV_LVL1);
    iom_ctrl(PA_ATMR_N, IOM_SEL_GPIO | IOM_DRV_LVL1);
}
