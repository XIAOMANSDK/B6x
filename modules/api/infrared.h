/*******************************************************************************
  * @file  IR.h
  * @author  WHL
  * @version  V1.0
  * @date  04/11/2024
  ******************************************************************************/ 
#ifndef _INFRARED_H_
#define _INFRARED_H_
#include <stdint.h>

void irInit(void);

void irCmdSend(uint8_t cmd);  // 110ms
void irCmdRepeat(void);
#endif
