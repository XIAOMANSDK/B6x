/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief 24BYJ48 步进电机驱动示例.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"
#include "stepper.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
 ****************************************************************************************
 */
int32_t step = 4096;

static void stepTest(void)
{
    stepperInit();

    // 示例：顺时针 4096 步（1 转），目标 10 rpm，加速度 200 steps/s^2
    stepperMove(step, 10, 200);

    while (1) {
        // 简单轮询可以查询 busy 状态
        if (!stepperIsBusy()) {
            bootDelayMs(2000);
            
            step *= -1;
            stepperMove(step, 10, 200);
             // 反向回来半圈
            step /= 2;
            if (!step) step = 4096;
        }
    }
}

static void sysInit(void)
{
    // Todo config, if need
    
}

static void devInit(void)
{
    iwdt_disable();
    
    dbgInit();
    debug("stepperMotor Test...\r\n");
}

int main(void)
{
    sysInit();
    devInit();
        
    __enable_irq();
    stepTest();
}
