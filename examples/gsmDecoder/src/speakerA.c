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

//PWM+ -> PWM-, �̶����IO. PWM��ַ�ʽ.
//PA07 -> PA11
//PA08 -> PA12
//PA09 -> PA13
#ifndef PA_ATMR_P
#define PA_ATMR_P             (7)           // PWM+ PA07
#endif

#if (1) // ��ʹ�ò�ֹ���оƬ,���迪��������.
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

//// 10KHz PCM���� PWM���� (��Ͳ�����)
////SPEAKER_PWM_TMR ARR 255������һ�룬128��0x80��128/255=ռ�ձ�Ϊ50%��������ֵ����
//#define SPEAKER_PWM_TMR_PSC (4 - 1)   //.	ϵͳ����Ƶ	64MHz/4=16MHz
//#define SPEAKER_PWM_TMR_ARR (255 - 1) //	����ֵ	16MHz/255 = 62.7KHz(��Ƶ����/����ֵ=ռ�ձ�)	
//#define SPEAKER_PWM_TMR_REP (6- 1)    // ���ڼ��� 6	62.7KHz/6 = 10.54KHz(������)

//// 16KHz PCM���� PWM����
//#define SPEAKER_PWM_TMR_PSC      (8 - 1)    // ϵͳ��Ƶ     64MHz/8   = 8MH
//#define SPEAKER_PWM_TMR_ARR      (250 - 1)  // ����ֵ       8MHz/250  = 32KHz (��Ƶ����/����ֵ = ռ�ձ�)
//#define SPEAKER_PWM_TMR_REP      (2 - 1)    // ���ڼ���2    32KHz/2 = 16KHz   (������)

//// 32KHz PCM���� PWM����
//#define SPEAKER_PWM_TMR_PSC (1 - 1)    //.ϵͳ����Ƶ	64MHz/1= 64MHz
//#define SPEAKER_PWM_TMR_ARR (2048 - 1) //����ֵ	64MHz/2048 = 32KHz(��Ƶ����/����ֵ=ռ�ձ�)
//#define SPEAKER_PWM_TMR_REP (1- 1)     // ���ڼ��� 1 32KHz/1 = 32KHz(������)

// 64KHz PCM���� PWM���� (��߲�����)
#define SPEAKER_PWM_TMR_PSC (1 - 1)    // ϵͳ����Ƶ 64MHz/1= 64MHz
#define SPEAKER_PWM_TMR_ARR (1000 - 1) // ����ֵ 64MHz/1000 = 64KHz(��Ƶ����/����ֵ=ռ�ձ�)
#define SPEAKER_PWM_TMR_REP (8- 1)     // ���ڼ��� 1 64KHz/8 = 8KHz(������)

static volatile uint8_t dam_done;
/*
 * FUNCTIONS
 ****************************************************************************************
 */

////16Khz �ڲ嵽64KHz PCM�����������PWMʹ��
////�����ڲ壨Linear Interpolation�� + һ�׵�ͨ�˲�����LPF��
///* �˲�ϵ�����ƣ�Alpha ֵԽ���˲�Խǿ���������ƣ���Alpha ԽС����������Խ�� */
///* ���鷶Χ 4 �� 8 */
//#define FILTER_ALPHA_SHIFT 6 

//typedef struct {
//    int32_t last_out;
//    int16_t last_sample;
//} Resampler_t;

///**
// * @brief һ�� IIR ��ͨ�˲��� (����ʵ��)
// */
//static inline int16_t simple_lpf(Resampler_t *r, int16_t input) {
//    // ��ʽ: y[n] = y[n-1] + alpha * (x[n] - y[n-1])
//    r->last_out += (input - (r->last_out >> 8)) * (256 >> (FILTER_ALPHA_SHIFT - 4));
//    return (int16_t)(r->last_out >> 8);
//}

///**
// * @brief 4�����������ĺ���
// * @param input_sample ԭʼ 16kHz ��һ��������
// * @param output_buf   ������ɵ� 4 �� 64kHz �����������, ͬʱ16bitPCMת��11bitPCM,������6bit, 1bit����λת��
// */
//static void resample_16to64_linear(Resampler_t *r, int16_t* input_sample, int16_t *output_buf, int16_t samples_to_read) {
//    
//    int16_t i;
//    for(i = 0; i < samples_to_read; i++){
//        int16_t a = r->last_sample;
//        int16_t b = input_sample[i];
//        
//        
//        /* �����ڲ幫ʽ: P(i) = a + (b - a) * i / 4 */
//        /* ����� 4 ���� */
//        int16_t diff = b - a;
//        
//        // �� 0 (���� a)
//        output_buf[i*4 + 0] = simple_lpf(r, a);
//        output_buf[i*4 + 0] = (output_buf[i*4 + 0] + 0x8000) >> 6;
//        // �� 1
//        output_buf[i*4 + 1] = simple_lpf(r, a + (diff >> 2));
//        output_buf[i*4 + 1] = (output_buf[i*4 + 1] + 0x8000) >> 6;
//        // �� 2
//        output_buf[i*4 + 2] = simple_lpf(r, a + (diff >> 1));
//        output_buf[i*4 + 2] = (output_buf[i*4 + 2] + 0x8000) >> 6;
//        // �� 3
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
    
    chnl_conf.duty = SPEAKER_PWM_TMR_ARR/2;  // ռ�ձ�50%-����
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
    NVIC_EnableIRQ(DMAC_IRQn);  ///< ʹ��DMA�������ж�
    GLOBAL_INT_START();
    
    dam_done = true;
}

/**
 ****************************************************************************************
 * @brief DMA�жϷ�����
 *
 * @details
 * ����DMAͨ������жϣ�
 * - ��ȡ�жϱ�־�Ĵ���
 * - ��������ж�
 * - ����жϱ�־
 * - ����ͨ����ʶ������Ӧ��������
 * - ����ʹ���ж�
 ****************************************************************************************
 */

void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHCFG->IFLAG0;  ///< ��ȡDMA�жϱ�־�Ĵ���
    uint16_t length;
    
    // �����Ѵ������ж�
    DMACHCFG->IEFR0 &= ~iflag;
    // ����жϱ�־
    DMACHCFG->ICFR0 = iflag;
    
    // �����ж�
    if (iflag & (1UL << SPEAKER_DMA_CHNL))
    {
        dam_done = true;
    }
    
    // ����ʹ���ж�
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
    pwm_duty_upd(PWM_ATMR_CH(PA_ATMR_P), SPEAKER_PWM_TMR_ARR/2);// ����
}

