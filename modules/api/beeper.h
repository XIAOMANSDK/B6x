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
    MUSIC1, // ����/�󶨳ɹ�       "�� �� �� �� ��"
    MUSIC2, // �ػ�                "�� ��" + 2
    MUSIC3, // ˫��                "��"
    MUSIC4, // �ָ�����/�Ƴ��豸   "�� �� �� ��"
    MUSIC5, // ��������            "��          �εε�" + 6
    
    MUSIC_OFF, // ֹͣ�򲥷����
};

// ��ʼ��IO, PWM����
void beeperInit(void);

#if (USE_APP_TIMER)
// musicId:see @MUSICID
// appTimeId:see @app_msg_id
void beeperPlay(uint8_t musicId, uint16_t appTimeId);

// �����´���Ҫ�����ʱ��ms
uint16_t beeperAPPTmr(void);
#else
void beeperPlay(uint8_t musicId);
#endif

// ��ȥ��ǰ���ŵ� MUSICID.
uint8_t beeperGet(void);
#endif
