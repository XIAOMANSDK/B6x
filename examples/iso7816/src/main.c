/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief ISO7816 smart card (ESAM) test
 *
 * @details
 * Test flow:
 * 1. Initialize system clock, debug UART
 * 2. Enable global interrupts
 * 3. Power on ESAM module, initialize ISO7816 interface
 * 4. Send GET CHALLENGE command (00 84 00 00 08) and read response
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

#define TEST_BUF_SIZE           50
#define ESAM_CMD_LEN            5
#define ESAM_RSP_DELAY_MS       30

/*
 * VARIABLES
 ****************************************************************************************
 */

static uint8_t test_buff[TEST_BUF_SIZE];

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief System clock configuration
 ****************************************************************************************
 */
static void sysInit(void)
{
    SYS_CLK_ALTER();
}

/**
 ****************************************************************************************
 * @brief Device and peripheral initialization
 ****************************************************************************************
 */
static void devInit(void)
{
    uint16_t rsn = rstrsn();

    iwdt_disable();

    dbgInit();
    debug("ISO7816(rsn:0x%X)...\r\n", rsn);
}

/**
 ****************************************************************************************
 * @brief ESAM card test -- power on, send command, read response
 ****************************************************************************************
 */
void esamTest(void)
{
    uint8_t test_data[ESAM_CMD_LEN] = {0x00, 0x84, 0x00, 0x00, 0x08};

    debug("Esam Test Start...\r\n");

    // Power on ESAM via IO control
    GPIO_DIR_SET_HI(BIT(PA_ESAM_VCC));
    iom_ctrl(PA_ESAM_VCC, IOM_DRV_LVL1);

    iso7816Init();

    bootDelayMs(ESAM_RSP_DELAY_MS);
    uint16_t rxLen = iso7816RbLen();
    iso7816RbRead(test_buff, rxLen);
    debug("Esam Power on Data:\r\n");
    debugHex(test_buff, rxLen);

    // Send GET CHALLENGE command
    debug("Esam CMD Send:\r\n");
    debugHex(test_data, sizeof(test_data));
    iso7816Send(ESAM_CMD_LEN, test_data);
    bootDelayMs(ESAM_RSP_DELAY_MS);

    rxLen = iso7816RbLen();
    iso7816RbRead(test_buff, rxLen);
    debug("Esam CMD Response:\r\n");
    debugHex(test_buff, rxLen);

    debug("Esam Test END\r\n");
}

/**
 ****************************************************************************************
 * @brief Application entry point
 ****************************************************************************************
 */
int main(void)
{
    sysInit();
    devInit();

    GLOBAL_INT_START();

    debug("...iso7816 Test Start...\r\n");

    esamTest();

    debug("...iso7816 Test End...\r\n\r\n");
    while (1);
}
