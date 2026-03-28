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
#include "iso7816.h"

/*
 * DEFINES
 ****************************************************************************************
 */
uint8_t test_buff[50];


/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void sysInit(void)
{    
    // Todo config, if need
    SYS_CLK_ALTER();
}

static void devInit(void)
{
    iwdt_disable();
    
    dbgInit();
}

void esamTest(void)
{
    uint8_t test_data[5] = {0x00, 0x84, 0x00, 0x00, 0x08};
    
    // ESAM测试
    debug("Esam Test Start...\r");
    
    // 使用 IO 给ESAM供电
    GPIO_DIR_SET_HI(BIT(PA_ESAM_VCC));
    iom_ctrl(PA_ESAM_VCC, IOM_DRV_LVL1);
    
    iso7816Init();
    
    bootDelayMs(30);
    uint16_t rxLen = iso7816RbLen();
    iso7816RbRead(test_buff, rxLen);
    debug("Esam Power on Data Receive:\r");  // ESAM 上电数据
    debugHex(test_buff, rxLen);
    
    debug("Esam CMD Data Send:\r");   // ESAM CMD数据
    debugHex(test_data, sizeof(test_data));
    iso7816Send(5, test_data);
    bootDelayMs(30);
    
    rxLen = iso7816RbLen();
    iso7816RbRead(test_buff, rxLen);
    debug("Esam CMD Data Receive:\r");  // ESAM CMD数据
    debugHex(test_buff, rxLen);
    
    debug("Esam Test END...\r\n");
}

int main(void)
{
    sysInit();
    devInit();
    // Global Interrupt Enable
    GLOBAL_INT_START();

    debug("...iso7816 Test Start...\r\n");
    
    esamTest();
    
    debug("...iso7816 Test End...\r\n\r\n");
    while(1);
}
