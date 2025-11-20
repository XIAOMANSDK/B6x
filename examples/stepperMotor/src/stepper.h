#ifndef _STEPPER_H_
#define _STEPPER_H_

#include <stdint.h>
#include <stdbool.h>

// 初始化（传入 TIM 用于步脉冲定时）.
void stepperInit(void);

// 发起移动：steps 可以为正负，rpm 为目标转速，acc_steps_per_s2 为加速度(steps/s^2).
void stepperMove(int32_t steps, uint8_t rpm, uint32_t accel_steps_per_s2);

// 立刻停止（平滑停止可实现为 decel 到 0）.
void stepperStop(void);

// 查询电机是否处于忙碌状态.
bool stepperIsBusy(void);
#endif
