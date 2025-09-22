/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "regs.h"
#include "dbg.h"

/*
 * DEFINES
 ****************************************************************************************
 */

__RETENTION uint32_t waupIOFalling; // SRAM2 powerOff不掉电, 存储下降沿唤醒的IO

#define WKUP_IO_MASK    0x00038080  // PA07 串口RX, KEY PA15/PA16/PA17. 选择需要唤醒所有IO.
#define WKUP_IO_FALLING 0x00018080  // PA07 串口RX, KEY PA15/PA16.      选择下降沿唤醒的IO.

void sysInit(void)
{    
    iwdt_disable();
    
    core_release_io_latch();  // 释放IO电平保持, 功能需要重新初始化.
}

static void devInit(void)
{    
    uint16_t rsn = rstrsn();
    
    uint32_t waupIONow;
    
    if (rsn & RSN_IO_WKUP_BIT)
    {
        for (uint8_t idx = 0; idx < 20; idx++)
        {
            if (BIT(idx) & WKUP_IO_MASK)
                iom_ctrl(idx, IOM_SEL_GPIO | IOM_INPUT);
        }
        
        waupIONow = GPIO_PIN_GET() & WKUP_IO_MASK;
    }
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);

    if (rsn & RSN_IO_WKUP_BIT)
    {
        debug("waupIONow1 [0x%06X]\r\n", waupIONow);
        
        waupIONow ^= waupIOFalling;
        
        debug("waupIONow2 [0x%06X]\r\n", waupIONow);
        
        if (waupIONow)
        {
            for (uint8_t idx = 0; idx < 20; idx++)
            {
                if (BIT(idx) & waupIONow)
                    debug("WAKE UP by IO PA[%02d]\r\n", idx);  //开发板KEY0/1/2, 可以准确判断.
            }        
        }
        else
        {
            debug("WAKE UP by IO PA[unknow]\r\n");  //串口 Rx上电平变换快,无法准确判断.
        }
    }
}

int main(void)
{
    sysInit();
    devInit();

    while(1)
    {
        uint8_t cmd = uart_getc(0);  // 1.5mA
        debug("cmd:%x\r\n", cmd);
        switch (cmd)
        {
            case 0x66:
            {
                debug("POWER OFF\r\n");
                waupIOFalling = WKUP_IO_FALLING;
                debug("waupIOALL [0x%06X]\r\n", WKUP_IO_MASK);
                debug("waupIOFalling [0x%06X]\r\n", waupIOFalling);
                
                wakeup_io_sw(WKUP_IO_MASK, WKUP_IO_FALLING);           // 配置IO唤醒方式
                core_pwroff(WKUP_IO_EN_BIT | WKUP_IO_LATCH_N_BIT);  // IO唤醒使能 + IO电平保持使能. 1uA
            } break;
            
            default:
            {
            } break;
        };
    }
}
