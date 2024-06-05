#include "drvs.h"
#include "sftmr.h"
#include "beeper.h"
#include "ke_api.h"

#if (DBG_BEEPER)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#ifndef BEEPER_PAD
#define BEEPER_PAD             (13) // PA13  CTMR IO PA0~PA19
#endif

#ifndef BEEPER_IDLE_LEVEL
#define BEEPER_IDLE_LEVEL      (0)          // 空闲 0:低电平  1:高电平
#endif

#define BEEPER_PWM_TMR_PSC     ((80 * (SYS_CLK + 1)) - 1)   // 200KHz  16MHz/80 80分频

#define BEEPER_PWM_4_7Khz      (42 - 1)  // 4.7KHz 重载值
#define BEEPER_PWM_4_2Khz      (47 - 1)  // 4.2KHz 重载值
#define BEEPER_PWM_3_7Khz      (54 - 1)  // 3.7KHz 重载值
#define BEEPER_PWM_3_2Khz      (62 - 1)  // 3.2KHz 重载值
#define BEEPER_PWM_2_7Khz      (74 - 1)  // 2.7KHz 重载值
#define BEEPER_PWM_2_2Khz      (90 - 1)  // 2.2KHz 重载值
#define BEEPER_PWM_1_7Khz      (118 - 1)  // 1.7KHz 重载值
#define BEEPER_PWM_1_2Khz      (166 - 1)  // 1.2KHz 重载值
#define BEEPER_PWM_1_1Khz      (182 - 1)  // 1.1KHz 重载值

#define BEEPER_STOP_0MS        (0)   // 停止播放0ms
#define BEEPER_IDLE_5MS        (5)   // 空闲5ms
#define BEEPER_IDLE_10MS       (10)  // 空闲10ms
#define BEEPER_IDLE_80MS       (80)  // 空闲80ms
#define BEEPER_IDLE_600MS      (600) // 空闲600ms
#define BEEPER_PLAY_100MS      (100) // 频率播放时间100ms

#define BEEPER_REPEAT_2T       (2)   // 重复播放2次
#define BEEPER_REPEAT_6T       (6)   // 重复播放6次

#define BEEPER_MUSIC1_NUM      (5)   // 频率数量
#define BEEPER_MUSIC2_NUM      (4)   // 频率数量+2
#define BEEPER_MUSIC3_NUM      (1)   // 频率数量
#define BEEPER_MUSIC4_NUM      (4)   // 频率数量
#define BEEPER_MUSIC5_NUM      (4)   // 频率数量

typedef struct
{
    uint8_t freq;
    uint8_t idle;
} beeper_freq_t;

typedef struct
{
    beeper_freq_t MUSIC1[BEEPER_MUSIC1_NUM];
    beeper_freq_t MUSIC2[BEEPER_MUSIC2_NUM];
    beeper_freq_t MUSIC3[BEEPER_MUSIC3_NUM];
    beeper_freq_t MUSIC4[BEEPER_MUSIC4_NUM];
    beeper_freq_t MUSIC5[BEEPER_MUSIC5_NUM];
} MUSIC_t;

typedef struct
{
    uint8_t MUSICID;
    uint8_t PLAYIDX;
    uint8_t IDLEIDX;
    uint8_t REPEAT;
} beeper_t;

const MUSIC_t BEEPER_M =
{
    .MUSIC1  = {{BEEPER_PWM_2_7Khz, BEEPER_IDLE_10MS}, \
                {BEEPER_PWM_3_2Khz, BEEPER_IDLE_10MS}, \
                {BEEPER_PWM_3_7Khz, BEEPER_IDLE_10MS}, \
                {BEEPER_PWM_4_2Khz, BEEPER_IDLE_10MS}, \
                {BEEPER_PWM_4_7Khz, BEEPER_STOP_0MS}},
    .MUSIC2  = {{BEEPER_PWM_2_7Khz, BEEPER_IDLE_10MS}, \
                {BEEPER_PWM_2_7Khz, BEEPER_IDLE_80MS}, \
                {BEEPER_PWM_2_7Khz, BEEPER_IDLE_10MS}, \
                {BEEPER_PWM_2_7Khz, BEEPER_STOP_0MS}},
    .MUSIC3  = {{BEEPER_PWM_2_7Khz, BEEPER_STOP_0MS}},
    .MUSIC4  = {{BEEPER_PWM_1_2Khz, BEEPER_IDLE_10MS}, \
                {BEEPER_PWM_1_7Khz, BEEPER_IDLE_10MS}, \
                {BEEPER_PWM_2_2Khz, BEEPER_IDLE_10MS}, \
                {BEEPER_PWM_2_7Khz, BEEPER_STOP_0MS}},
    .MUSIC5  = {{BEEPER_PWM_2_7Khz, BEEPER_IDLE_80MS}, \
                {BEEPER_PWM_1_1Khz, BEEPER_IDLE_5MS},  \
                {BEEPER_PWM_2_7Khz, BEEPER_IDLE_5MS},  \
                {BEEPER_PWM_1_1Khz, BEEPER_STOP_0MS}},
};

beeper_t BEEPER =
{
    .MUSICID = MUSIC_OFF,
    .PLAYIDX = 0x00,
    .IDLEIDX = 0x00,
    .REPEAT  = 0x00,
};
/*
 * FUNCTIONS
 ****************************************************************************************
 */

//// 初始化PWM系统分频200KHz,自动重载值182
static void pwmInit(void)
{
    gpio_set_hiz(BEEPER_PAD);

    iom_ctrl(BEEPER_PAD, IOM_SEL_CSC);
    csc_output(BEEPER_PAD, CSC_CTMR_CH1);
    
    pwm_init(PWM_CTMR, BEEPER_PWM_TMR_PSC, BEEPER_PWM_1_1Khz);
    
    pwm_chnl_cfg_t chnl_conf;
    chnl_conf.duty = 0;
    
    #if (BEEPER_IDLE_LEVEL)
        chnl_conf.ccer = PWM_CCER_SIPL; //
    #else
        chnl_conf.ccer = PWM_CCER_SIPH; //
    #endif

    chnl_conf.ccmr = PWM_CCMR_MODE1;
    pwm_chnl_set(PWM_CTMR_CH1, &chnl_conf);

    pwm_start(PWM_CTMR);
}

// 空闲
void pwmIdle(void)
{
    pwm_duty_upd(PWM_CTMR_CH1, 0);
}

// 频率设置
void pwmSetFrequency(uint8_t arr)
{
    CTMR->ARR = arr;
    pwm_duty_upd(PWM_CTMR_CH1, arr/2);
}

tmr_tk_t beeperTmr(tmr_id_t tmid)
{
    uint16_t idle_delay = BEEPER_PLAY_100MS;
    
    DEBUG("PLAYIDX:[%d]", BEEPER.PLAYIDX);
    
    switch(BEEPER.MUSICID)
    {
        case MUSIC1: 
        {
            if (BEEPER.PLAYIDX != BEEPER.IDLEIDX)
            {
                idle_delay = BEEPER_M.MUSIC1[BEEPER.IDLEIDX].idle;
                
                BEEPER.IDLEIDX = BEEPER.PLAYIDX;
                pwmIdle();
            }
            else
            {
                pwmSetFrequency(BEEPER_M.MUSIC1[BEEPER.PLAYIDX++].freq);
            }
        } break;
        
        case MUSIC2: 
        {
            if (BEEPER.PLAYIDX != BEEPER.IDLEIDX)
            {
                idle_delay = BEEPER_M.MUSIC2[BEEPER.IDLEIDX].idle;
                
                BEEPER.IDLEIDX = BEEPER.PLAYIDX;
                pwmIdle();
            }
            else
            {
                pwmSetFrequency(BEEPER_M.MUSIC2[BEEPER.PLAYIDX++].freq);
            }
        } break;
        
        case MUSIC3: 
        {
            if (BEEPER.PLAYIDX != BEEPER.IDLEIDX)
            {
                idle_delay = BEEPER_M.MUSIC3[BEEPER.IDLEIDX].idle;
                
                BEEPER.IDLEIDX = BEEPER.PLAYIDX;
                pwmIdle();
            }
            else
            {
                pwmSetFrequency(BEEPER_M.MUSIC3[BEEPER.PLAYIDX++].freq);
            }

        } break;
        
        case MUSIC4: 
        {
            if (BEEPER.PLAYIDX != BEEPER.IDLEIDX)
            {
                idle_delay = BEEPER_M.MUSIC4[BEEPER.IDLEIDX].idle;
                
                BEEPER.IDLEIDX = BEEPER.PLAYIDX;
                pwmIdle();
            }
            else
            {
                pwmSetFrequency(BEEPER_M.MUSIC4[BEEPER.PLAYIDX++].freq);
            }

        } break;
        
        case MUSIC5: 
        {
            if (BEEPER.PLAYIDX != BEEPER.IDLEIDX)
            {
                idle_delay = BEEPER_M.MUSIC5[BEEPER.IDLEIDX].idle;
                
                BEEPER.IDLEIDX = BEEPER.PLAYIDX;
                pwmIdle();
            }
            else
            {
                pwmSetFrequency(BEEPER_M.MUSIC5[BEEPER.PLAYIDX++].freq);
            }
        } break;
        
        default :
        {
            idle_delay = BEEPER_STOP_0MS;  // 关闭
        }break;
    }

    if (!idle_delay) 
    {
        if (BEEPER.REPEAT)
        {       
            BEEPER.REPEAT--;
            BEEPER.PLAYIDX = 0x00;
            BEEPER.IDLEIDX = 0x00;
            idle_delay = BEEPER_IDLE_600MS;
        }
        else
        {
            BEEPER.MUSICID = MUSIC_OFF;
        }
    }

    #if (USE_APP_TIMER)
    return idle_delay;
    #else
    return _MS(idle_delay);
    #endif
}

// 蜂鸣器播放
#if (USE_APP_TIMER)
void beeperPlay(uint8_t musicId, uint16_t appTimeId)
#else
void beeperPlay(uint8_t musicId)
#endif
{
    DEBUG("PLAY MUSIC:[%d]", musicId);
    BEEPER.MUSICID = musicId;
    BEEPER.PLAYIDX = 0x00;
    BEEPER.IDLEIDX = 0x00;
    BEEPER.REPEAT  = 0x00;
    
    if (BEEPER.MUSICID == MUSIC5)
        BEEPER.REPEAT = BEEPER_REPEAT_6T;
    
    #if (USE_APP_TIMER)
    ke_timer_set(appTimeId, TASK_APP, 20);
    #else
    sftmr_start(0, beeperTmr);
    #endif
}

// 获取当前播放内容
uint8_t beeperGet(void)
{
    return BEEPER.MUSICID;
}

uint16_t beeperAPPTmr(void)
{
    return beeperTmr(NULL);
}

void beeperInit(void)
{
    pwmInit();
//    sftmr_start(20, beeperTmr);
}


