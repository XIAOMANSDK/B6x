#include "drvs.h"
#include "link_slave.h"
#include "utils.h"

#if (DBG_SLV)
#include "dbg.h"
#define DEBUG(format, ...) debug(format , ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

static void slave_test_proc(void *param, uint32_t flag);

static void slave_logout(const char *line)
{
    DEBUG("Slave %s", line);
}

#if (SYS_CLK == 0)
#define TMS_CTMR_PSC        (1599) // (16 000 000Hz/10 000 - 1), 100us
#elif (SYS_CLK == 1)
#define TMS_CTMR_PSC        (3199) // (32 000 000Hz/10 000 - 1), 100us
#endif
static void timer_init(uint16_t time) //ms
{
    ctmr_init(TMS_CTMR_PSC, time);

    CTMR->EGR.Word = TMR_IR_UI_BIT;
    CTMR->ICR.Word = TMR_IR_UI_BIT;
    CTMR->IER.Word = TMR_IR_UI_BIT;
    // event generation
    //CTMR->EGR.Word = TMR_IR_UI_BIT;

    // conf control (counter start from 0)
    CTMR->CNT = 0;
    CTMR->CR1.Word = TMR_PERIOD_MODE;
}

void slave_test_conf(void)
{
    link_slave_conf_t    slave_conf;
    link_slave_handler_t slave_handler;

    slave_conf.link.access_code = 0x71764129;
    slave_conf.link.chn_list[0] = RF_CHNL_2422;
    slave_conf.link.chn_list[1] = RF_CHNL_INVALID;
    slave_conf.link.chn_list[2] = RF_CHNL_INVALID;
    slave_conf.link.chn_list[3] = RF_CHNL_INVALID;
    slave_conf.link.chn_list[4] = RF_CHNL_INVALID;
    slave_conf.link.chn_list[5] = RF_CHNL_INVALID;
    slave_conf.link.chn_list[6] = RF_CHNL_INVALID;
    slave_conf.link.chn_list[7] = RF_CHNL_INVALID;

    slave_conf.link.rf_rate     = LINK_1Mbps;
    slave_conf.link.rx_pwrup    = 50;
    slave_conf.link.tx_pwrup    = 80;
    slave_conf.link.rx_pathdly  = 26;
    slave_conf.link.tx_pathdly  = 5;
    slave_conf.link.slot_win    = 900;

    slave_conf.address     = 0x12345677;
    slave_conf.tos.feature = LINK_TOS_FEATURE_BEST_EFFORT;
    slave_conf.tos.slots   = 3;
    slave_conf.tos.type    = 0;

    slave_conf.debounce_window = 50;
    slave_conf.tx_retry        = 5;
    slave_conf.rtx_gap         = 50;
    slave_conf.chn_retry       = 5;
    slave_conf.conn_alive      = 8 * 1000;
    slave_conf.keep_alive      = 5 * 1000;
    #if (DBG_MODE)
    slave_conf.log_level       = LINK_LOG_LEVEL_INFO;
    #else
    slave_conf.log_level       = LINK_LOG_LEVEL_NONE;
    #endif

    slave_handler.flag      = 0;
    slave_handler.param     = 0;
    slave_handler.user_proc = slave_test_proc;
    slave_handler.log_out   = slave_logout;

    link_slave_conf(&slave_conf, &slave_handler);
}

#if (CFG_TEST_CIRCLE_DATA)
uint8_t g_key_start = 0, g_xy_idx = 0;
int8_t g_x_offset[] = {-1,-2,-4,-5,-6,-7,-8,-9,-8,-8,-9,-8,-7,-6,-5,-4,-2,-1, 1, 2, 4, 5, 6, 7, 8, 9, 8,8,9,8,7,6,5,4,2,1};
int8_t g_y_offset[] = { 8, 9, 8, 7, 6, 5, 4, 2, 1,-1,-2,-4,-5,-6,-7,-8,-9,-8,-8,-9,-8,-7,-6,-5,-4,-2,-1,1,2,4,5,6,7,8,9,8};
#endif
uint8_t g_mouse_pkt[MOUSE_LEN];
enum mouse_data_pos
{
    #if (MOUSE_LEN == 5)
    MS_POS_RID, // report id
    #endif
    MS_POS_BTN,
    MS_POS_X,
    MS_POS_Y,
    MS_POS_WHEEL,
};

static void slave_test_proc(void *param, uint32_t flag)
{
    int ret;
    timer_init(100);
    ret = link_slave_connect();
    DEBUG("cid:%d\r\n", ret);
    g_xy_idx = 0;
    memset(g_mouse_pkt, 0x00, MOUSE_LEN);

    while (1)
    {
        if(CTMR->CNT >= 10) // 10 * 100us
        {
            CTMR->CNT = 0;
            #if (MOUSE_LEN == 5)
            g_mouse_pkt[MS_POS_RID] = 0x01;
            #endif
            g_mouse_pkt[MS_POS_X] = (g_x_offset[g_xy_idx]);
            g_mouse_pkt[MS_POS_Y] = (g_y_offset[g_xy_idx]);

            g_xy_idx = (g_xy_idx + 1) % sizeof(g_y_offset);

            do
            {
                ret = link_slave_send(g_mouse_pkt, MOUSE_LEN);
            } while (ret == -SLAVE_NERR_BUFF_FULL);
        }
    }
}
