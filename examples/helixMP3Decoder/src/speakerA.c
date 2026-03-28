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
#define SPEAKER_PWM_TMR_REP (4- 1)     // 周期计数 1 64KHz/4 = 16KHz(采样率)

static volatile uint8_t dam_done;
/*
 * FUNCTIONS
 ****************************************************************************************
 */

////16Khz 内插到64KHz PCM数据输出，供PWM使用
////线性内插（Linear Interpolation） + 一阶低通滤波器（LPF）
///* 滤波系数控制：Alpha 值越大，滤波越强（声音变闷），Alpha 越小，镜像噪声越多 */
///* 建议范围 4 到 8 */
//#define FILTER_ALPHA_SHIFT 6 

//typedef struct {
//    int32_t last_out;
//    int16_t last_sample;
//} Resampler_t;

///**
// * @brief 一阶 IIR 低通滤波器 (定点实现)
// */
//static inline int16_t simple_lpf(Resampler_t *r, int16_t input) {
//    // 公式: y[n] = y[n-1] + alpha * (x[n] - y[n-1])
//    r->last_out += (input - (r->last_out >> 8)) * (256 >> (FILTER_ALPHA_SHIFT - 4));
//    return (int16_t)(r->last_out >> 8);
//}

///**
// * @brief 4倍升采样核心函数
// * @param input_sample 原始 16kHz 的一个采样点
// * @param output_buf   存放生成的 4 个 64kHz 采样点的数组, 同时16bitPCM转成11bitPCM,丢掉低6bit, 1bit符号位转换
// */
//static void resample_16to64_linear(Resampler_t *r, int16_t* input_sample, int16_t *output_buf, int16_t samples_to_read) {
//    
//    int16_t i;
//    for(i = 0; i < samples_to_read; i++){
//        int16_t a = r->last_sample;
//        int16_t b = input_sample[i];
//        
//        
//        /* 线性内插公式: P(i) = a + (b - a) * i / 4 */
//        /* 计算出 4 个点 */
//        int16_t diff = b - a;
//        
//        // 点 0 (靠近 a)
//        output_buf[i*4 + 0] = simple_lpf(r, a);
//        output_buf[i*4 + 0] = (output_buf[i*4 + 0] + 0x8000) >> 6;
//        // 点 1
//        output_buf[i*4 + 1] = simple_lpf(r, a + (diff >> 2));
//        output_buf[i*4 + 1] = (output_buf[i*4 + 1] + 0x8000) >> 6;
//        // 点 2
//        output_buf[i*4 + 2] = simple_lpf(r, a + (diff >> 1));
//        output_buf[i*4 + 2] = (output_buf[i*4 + 2] + 0x8000) >> 6;
//        // 点 3
//        output_buf[i*4 + 3] = simple_lpf(r, a + (diff >> 2) + (diff >> 1));
//        output_buf[i*4 + 3] = (output_buf[i*4 + 3] + 0x8000) >> 6;

//        r->last_sample = input_sample[i];
//    }
//}
//static Resampler_t resampler = {0, 0};


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
    
    DMACHNL_INT_EN(SPEAKER_DMA_CHNL); 
    NVIC_EnableIRQ(DMAC_IRQn);  ///< 使能DMA控制器中断
    GLOBAL_INT_START();
    
    dam_done = true;
}

/**
 ****************************************************************************************
 * @brief DMA中断服务函数
 *
 * @details
 * 处理DMA通道完成中断：
 * - 读取中断标志寄存器
 * - 禁用相关中断
 * - 清除中断标志
 * - 根据通道标识调用相应处理函数
 * - 重新使能中断
 ****************************************************************************************
 */

void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHCFG->IFLAG0;  ///< 读取DMA中断标志寄存器
    uint16_t length;
    
    // 禁用已触发的中断
    DMACHCFG->IEFR0 &= ~iflag;
    // 清除中断标志
    DMACHCFG->ICFR0 = iflag;
    
    // 处理中断
    if (iflag & (1UL << SPEAKER_DMA_CHNL))
    {
        dam_done = true;
    }
    
    // 重新使能中断
    DMACHCFG->IEFR0 |= iflag;
    
}

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

void speakerStop(void)
{
    pwm_duty_upd(PWM_ATMR_CH(PA_ATMR_P), SPEAKER_PWM_TMR_ARR/2);// 静音
}

