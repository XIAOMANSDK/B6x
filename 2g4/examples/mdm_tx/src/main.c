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
#include "dbg.h"
#include "link_mdm.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define MDM_TEST_DATA_LENGTH (16)

static uint8_t mdm_test_data[MDM_TEST_DATA_LENGTH];

static void mdm_test_conf(void);
static void mdm_test_sender(void *param, uint32_t flag);

/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void sysInit(void)
{
    SYS_CLK_ALTER();

    iwdt_disable();

    puya_enter_dual_read();

    rcc_fshclk_set(FSH_CLK_DPSC42);
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();

    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);

    link_mdm_init(MDM_ROLE_SENDER);

    mdm_test_conf();

    for (int i = 0; i < MDM_TEST_DATA_LENGTH; i++)
    {
        mdm_test_data[i] = i;
    }
}

int main(void)
{
    sysInit();

    devInit();

    GLOBAL_INT_START();

    link_mdm_schedule();
}

static void mdm_test_conf(void)
{
    link_conf_t        conf;
    link_mdm_handler_t handler;

    conf.access_code = 0x71764129;
    conf.chn_list[0] = RF_CHNL_2422;
    conf.chn_list[1] = RF_CHNL_INVALID;
    conf.rf_rate     = LINK_1Mbps;
    conf.rx_pwrup    = 50;
    conf.tx_pwrup    = 80;
    conf.rx_pathdly  = 26 + 30;
    conf.tx_pathdly  = 5;
    conf.slot_win    = 900;

    handler.flag  = 0;
    handler.param = 0;

    handler.user_proc = mdm_test_sender;

    link_mdm_conf(&conf, &handler);
}

static void mdm_test_sender(void *param, uint32_t flag)
{
    int rt = link_mdm_send(mdm_test_data, MDM_TEST_DATA_LENGTH);
    if (rt < 0)
    {
        debug("send failed %d\r\n", rt);
    }
    else
    {
    }
}
