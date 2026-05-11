#include "drvs.h"
#include "link_master.h"
#include "app_user.h"

#if (DBG_MST)
#include "dbg.h"
#define DEBUG(format, ...) debug(format , ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#if (CFG_USB)
#include "usbd.h"
#include "usbd_hid.h"

int8_t pkt_mouse[MOUSE_LEN] = {1,0,0,0,0};
#endif

static void master_test_proc(void *param, uint32_t flag);

static void master_logout(const char *line)
{
    DEBUG("Master %s", line);
}

#if (CFG_USB)
uint8_t g_key_start = 0, g_xy_idx = 0;
int8_t g_x_offset[] = {-1,-2,-4,-5,-6,-7,-8,-9,-8,-8,-9,-8,-7,-6,-5,-4,-2,-1, 1, 2, 4, 5, 6, 7, 8, 9, 8,8,9,8,7,6,5,4,2,1};
int8_t g_y_offset[] = { 8, 9, 8, 7, 6, 5, 4, 2, 1,-1,-2,-4,-5,-6,-7,-8,-9,-8,-8,-9,-8,-7,-6,-5,-4,-2,-1,1,2,4,5,6,7,8,9,8};
void usb_send(void)
{
    //GLOBAL INT DISABLE();
    pkt_mouse[2]+=(g_x_offset[g_xy_idx]<< 1);
    pkt_mouse[3]+=(g_y_offset[g_xy_idx]<< 1);
    //GLOBAL INT RESTORE();
    g_xy_idx=(g_xy_idx + 1) % sizeof(g_y_offset);
    usbd_mouse_report();
    bootDelayUs(980);
}
uint8_t *get_mouse_pkt(void)
{
    if((pkt_mouse[2] == 0) && (pkt_mouse[3] == 0))
        return NULL;
    else
        return (uint8_t *)pkt_mouse;
}

void clear_mouse_pkt(void)
{
    pkt_mouse[2] = 0;
    pkt_mouse[3] = 0;
}
#endif

void master_test_conf(void)
{
    link_master_conf_t    master_conf;
    link_master_handler_t master_handler;

    master_conf.link.access_code = 0x71764129;

    master_conf.link.chn_list[0] = RF_CHNL_2422;
    master_conf.link.chn_list[1] = RF_CHNL_INVALID;
    master_conf.link.chn_list[2] = RF_CHNL_INVALID;
    master_conf.link.chn_list[3] = RF_CHNL_INVALID;
    master_conf.link.chn_list[4] = RF_CHNL_INVALID;
    master_conf.link.chn_list[5] = RF_CHNL_INVALID;
    master_conf.link.chn_list[6] = RF_CHNL_INVALID;
    master_conf.link.chn_list[7] = RF_CHNL_INVALID;

    master_conf.link.rf_rate    = LINK_1Mbps;
    master_conf.link.rx_pwrup   = 50;
    master_conf.link.tx_pwrup   = 80;
    master_conf.link.rx_pathdly = 26 + 30;
    master_conf.link.tx_pathdly = 5;
    master_conf.link.slot_win   = 900;

    master_conf.rssi_median_thresh     = 30;
    master_conf.rssi_peak_thresh       = 40;
    master_conf.conflict_detect_period = 1000;
    master_conf.conflict_detect_thresh = 30;

    master_conf.rx_leading_time = 80;//50;
    master_conf.ack_delay       = 250;
    master_conf.conn_alive      = 10 * 1000;
#if (DBG_MODE)
    master_conf.log_level       = LINK_LOG_LEVEL_INFO;
#else
    master_conf.log_level       = LINK_LOG_LEVEL_NONE;
#endif
    master_handler.flag      = 0;
    master_handler.param     = 0;
    master_handler.user_proc = master_test_proc;
    master_handler.log_out   = master_logout;

    link_master_conf(&master_conf, &master_handler);
}

static void master_test_proc(void *param, uint32_t flag)
{
    link_master_message_t msg;

    #if (CFG_USB)
    usbd_mouse_report();
    #endif

    if (link_master_fetch_message(&msg) < 0)
    {
        return;
    }

    if (msg.type == MASTER_MESSAGE_DATA_RECEIVED)
    {
        #if (CFG_USB)
        pkt_mouse[2] += msg.body.buff.data[2];
        pkt_mouse[3] += msg.body.buff.data[3];
        usbd_mouse_report();
        #endif
        // debug("data recv from %08x, len=%d\r\n", msg.address, msg.body.buff.length);
        // DEBUG("r %d:",msg.body.buff.length);
        // debugHex(msg.body.buff.data, msg.body.buff.length);
        // link_master_piggyback(msg.cid, (uint8_t *)&g_cnt, sizeof(g_cnt));
    }
    else if (msg.type == MASTER_MESSAGE_DATA_SENT)
    {
        // DEBUG("data sent to %8.8x\r\n", msg.address);
    }
    else if (msg.type == MASTER_MESSAGE_CONNECTION_CREATED)
    {
        // uint8_t hello[] = "hello";
        // int     rt      = link_master_piggyback(msg.cid, hello, 6); // sizeof(hello));

        // DEBUG("piggyback %d\r\n", rt);
        // DEBUG("connected from %8.8x, cid=%d\r\n", msg.address, msg.cid);
    }
    else if (msg.type == MASTER_MESSAGE_CONNECTION_CLOSED)
    {
        // DEBUG("disconnected from %8.8x, cid=%d\r\n", msg.address, msg.cid);
        // DEBUG("-d-\r\n");
    }
}
