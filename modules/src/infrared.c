#include "drvs.h"
#include "infrared.h"

#if (DBG_IR)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#ifndef IR_TX_PAD
#define IR_TX_PAD             (9) // PA9  ATMR IO PA7~PA13
#endif

#if ((IR_TX_PAD < 7) || (IR_TX_PAD > 13))
    #error "IR PIN MUST BE BETWEEN PA07 AND PA13 !!!"
#endif

#ifndef IR_TX_ADR
#define IR_TX_ADR              0xFF00    // 地址0x00, 地址反码0xFF
#endif

#ifndef IR_TX_PWM_DMA_CHNL
#define IR_TX_PWM_DMA_CHNL           (DMA_CH0)
#endif

#ifndef IR_TX_IDLE_LEVEL
#define IR_TX_IDLE_LEVEL       0          // 空闲 0:低电平  1:高电平
#endif

#define IR_TX_DMA_PTR_ATMR   (DMA_PTR_ATMR_CH1 + ((IR_TX_PAD - 7)%4)*4)
#define IR_DMA_ATMR_CHx_INIT(chidx)    dma_chnl_init(chidx, DMA_PID_ATMR_UP)
#define IR_DMA_ATMR_CHx_CONF(chidx, buff, len, ccm) \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], IR_TX_DMA_PTR_ATMR, TRANS_PER_WR(ccm, len, IN_BYTE, IN_BYTE))

#define IR_TX_PWM_TMR_PSC      (2 - 1)    // 8MHz  16MHz/2 二分频
#define IR_TX_PWM_TMR_ARR      (210 - 1)  // 38KHz 重载值
#define IR_TX_PWM_TMR_00P      0          // 38KHz 占空比0%
#define IR_TX_PWM_TMR_50P      106        // 38KHz 占空比50%
#define IR_TX_PWM_TMR_REP      20         // 38KHz 周期计数26.256us*21 = 560us 

#define IR_TX_HDL_LEN          16
#define IR_TX_HDH_LEN          8
#define IR_TX_ADR_LEN          (32 + (__builtin_popcount(IR_TX_ADR)*2))         // 非标准,变长.bit:0--2, bit:1--4. 32+X
#define IR_TX_CMD_LEN          48
#define IR_TX_END_LEN          2

#define IR_TX_SED_LEN         (IR_TX_HDL_LEN + IR_TX_HDH_LEN + IR_TX_ADR_LEN + IR_TX_CMD_LEN + IR_TX_END_LEN)
#define IR_TX_REP_LEN         (IR_TX_HDL_LEN + IR_TX_HDH_LEN)

/* 红外编码,对应的比较值结构体 */
typedef struct 
{
    uint8_t HDL[IR_TX_HDL_LEN];   // 引导码L 9ms=560us*16
    uint8_t HDH[IR_TX_HDH_LEN];   // 引导码H 4.5ms=560us*8
    uint8_t ADR[IR_TX_ADR_LEN];   // 地址 + 地址反码 22.4ms = 560us*8*(2+4)
    uint8_t CMD[IR_TX_CMD_LEN];   // 命令 + 命令反码 22.4ms = 560us*8*(2+4)
    uint8_t END[IR_TX_END_LEN];   // 结束 560us*2
}ir_tx_t;

ir_tx_t  IR_TX;  //

/*
 * FUNCTIONS
 ****************************************************************************************
 */

// 初始化PWM系统分频38KHz,自动重载值210
static void PwmInit(void)
{
    gpio_set_hiz(IR_TX_PAD);
    dma_init();

    iom_ctrl(IR_TX_PAD, IOM_SEL_TIMER);
    pwm_init(PWM_ATMR, IR_TX_PWM_TMR_PSC, IR_TX_PWM_TMR_ARR);
    
    pwm_chnl_cfg_t chnl_conf;
    chnl_conf.duty = 0;
    
    #if (IR_TX_IDLE_LEVEL)
        chnl_conf.ccer = PWM_CCER_SIPL | PWM_CCxDE_BIT; // DMA_EN    
    #else
        chnl_conf.ccer = PWM_CCER_SIPH | PWM_CCxDE_BIT; // DMA_EN
    #endif

    chnl_conf.ccmr = PWM_CCMR_MODE1;
    pwm_chnl_set((IR_TX_PAD - 3) /*PWM_ATMR_CH3P*/, &chnl_conf);

    pwm_start(PWM_ATMR);
    ATMR->DMAEN.UDE = 1;
    
    IR_DMA_ATMR_CHx_INIT(IR_TX_PWM_DMA_CHNL);

    ATMR->CR1.URS = 1;
    ATMR->RCR = IR_TX_PWM_TMR_REP;
}

//红外固定数据初始化
void irDataInit(void)
{
    uint8_t addrIdx = 0;
    
    DEBUG("SIZI:%d : %d : %d", sizeof(IR_TX.ADR), sizeof(IR_TX), __builtin_popcount(IR_TX_ADR));

    memset(IR_TX.HDL, IR_TX_PWM_TMR_50P, IR_TX_HDL_LEN);
    memset(IR_TX.HDH, IR_TX_PWM_TMR_00P, IR_TX_HDH_LEN);
    
    IR_TX.END[0] = IR_TX_PWM_TMR_50P;
    IR_TX.END[1] = IR_TX_PWM_TMR_00P;

    for(uint8_t idx = 0; idx < 16; idx++)
    {
        if (IR_TX_ADR & (1UL << idx))
        {
            // 1
            IR_TX.ADR[addrIdx++] = IR_TX_PWM_TMR_50P;
            IR_TX.ADR[addrIdx++] = IR_TX_PWM_TMR_00P;
            IR_TX.ADR[addrIdx++] = IR_TX_PWM_TMR_00P;
            IR_TX.ADR[addrIdx++] = IR_TX_PWM_TMR_00P;
        }
        else
        {
            // 0
            IR_TX.ADR[addrIdx++] = IR_TX_PWM_TMR_50P;
            IR_TX.ADR[addrIdx++] = IR_TX_PWM_TMR_00P;
        }
    }
    
//    // 地址反码
//    for(uint8_t idx = 0; idx < 8; idx++)
//    {
//        if (IR_TX_ADR & (1 << idx))
//        {
//            // 0
//            IR_TX.ADR[addrIdx++] = IR_TX_PWM_TMR_50P;
//            IR_TX.ADR[addrIdx++] = IR_TX_PWM_TMR_00P;
//        }
//        else
//        {
//            // 1
//            IR_TX.ADR[addrIdx++] = IR_TX_PWM_TMR_50P;
//            IR_TX.ADR[addrIdx++] = IR_TX_PWM_TMR_00P;
//            IR_TX.ADR[addrIdx++] = IR_TX_PWM_TMR_00P;
//            IR_TX.ADR[addrIdx++] = IR_TX_PWM_TMR_00P;
//        }
//    }
}

//设置红外CMD
void irCmdSet(uint8_t cmd)
{
    uint8_t cmdIdx = 0;
    
    for(uint8_t idx = 0; idx < 8; idx++)
    {
        if (cmd & (1 << idx))
        {
            // 1
            IR_TX.CMD[cmdIdx++] = IR_TX_PWM_TMR_50P;
            IR_TX.CMD[cmdIdx++] = IR_TX_PWM_TMR_00P;
            IR_TX.CMD[cmdIdx++] = IR_TX_PWM_TMR_00P;
            IR_TX.CMD[cmdIdx++] = IR_TX_PWM_TMR_00P;
        }
        else
        {
            // 0
            IR_TX.CMD[cmdIdx++] = IR_TX_PWM_TMR_50P;
            IR_TX.CMD[cmdIdx++] = IR_TX_PWM_TMR_00P;
        }
    }
    
    // CMD反码
    for(uint8_t idx = 0; idx < 8; idx++)
    {
        if (cmd & (1 << idx))
        {
            // 0
            IR_TX.CMD[cmdIdx++] = IR_TX_PWM_TMR_50P;
            IR_TX.CMD[cmdIdx++] = IR_TX_PWM_TMR_00P;
        }
        else
        {
            // 1
            IR_TX.CMD[cmdIdx++] = IR_TX_PWM_TMR_50P;
            IR_TX.CMD[cmdIdx++] = IR_TX_PWM_TMR_00P;
            IR_TX.CMD[cmdIdx++] = IR_TX_PWM_TMR_00P;
            IR_TX.CMD[cmdIdx++] = IR_TX_PWM_TMR_00P;
        }
    }
}

void irDmaSet(uint16_t length)
{
    IR_DMA_ATMR_CHx_CONF(IR_TX_PWM_DMA_CHNL, (uint8_t *)&IR_TX, length, CCM_BASIC);
    while(!dma_chnl_done(IR_TX_PWM_DMA_CHNL));
}

//红外发送接口
void irCmdSend(uint8_t cmd)
{
    IR_TX.HDH[4] = IR_TX_PWM_TMR_00P;
    irCmdSet(cmd);
    irDmaSet(IR_TX_SED_LEN);
}

//重复码
void irCmdRepeat(void)
{
    IR_TX.HDH[4] = IR_TX_PWM_TMR_50P;
    irDmaSet(IR_TX_REP_LEN);
}

void irInit(void)
{
    //  ATMR IO PA7~PA13
    PwmInit();
    irDataInit();
}

