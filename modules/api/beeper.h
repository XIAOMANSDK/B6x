/*******************************************************************************
  * @file  beeper.h
  * @author  WHL
  * @version  V1.0
  * @date  04/20/2024
  ******************************************************************************/ 
#ifndef _BEEPER_H_
#define _BEEPER_H_
#include <stdint.h>

#define USE_APP_TIMER       (1)  // 1: APP Task messages,  0: sftmr

enum MUSICID
{
    MUSIC1, // 开机/绑定成功       "滴 滴 滴 滴 滴"
    MUSIC2, // 关机                "滴 滴" + 2
    MUSIC3, // 双击                "滴"
    MUSIC4, // 恢复出厂/移除设备   "滴 滴 滴 滴"
    MUSIC5, // 播放声音            "滴          滴滴滴" + 6
    
    MUSIC_OFF, // 停止或播放完毕
};

// 初始化IO, PWM配置
void beeperInit(void);

#if (USE_APP_TIMER)
// musicId:see @MUSICID
// appTimeId:see @app_msg_id
void beeperPlay(uint8_t musicId, uint16_t appTimeId);

// 返回下次需要处理的时间ms
uint16_t beeperAPPTmr(void);
#else
void beeperPlay(uint8_t musicId);
#endif

// 过去当前播放的 MUSICID.
uint8_t beeperGet(void);
#endif
