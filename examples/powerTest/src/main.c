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

void sysInit(void)
{    
    iwdt_disable();
    
    core_release_io_latch();  // �ͷ�IO��ƽ����, ������Ҫ���³�ʼ��.
}

static void devInit(void)
{    
    uint16_t rsn = rstrsn();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
    
    if (rsn & RSN_IO_WKUP_BIT)
    {
        debug("WAKE UP by [IO]\r\n");
    }
}

#define WKUP_IO_MASK 0x80  // PA07 ����RX

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
                wakeup_io_sw(WKUP_IO_MASK, WKUP_IO_MASK);           // ����IO�½��ػ���
                core_pwroff(WKUP_IO_EN_BIT | WKUP_IO_LATCH_N_BIT);  // IO����ʹ�� + IO��ƽ����ʹ��. 1uA
            } break;
            
            case 0x88:
            {
                debug("POWER OFF\r\n");
                wakeup_io_sw(WKUP_IO_MASK, 0);                      // ����IO�����ػ���
                core_pwroff(WKUP_IO_EN_BIT | WKUP_IO_LATCH_N_BIT);  // IO����ʹ�� + IO��ƽ����ʹ��. 1uA
            } break;
            
            default:
            {
            } break;
        };
    }
}
