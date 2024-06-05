/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "app.h"
#include "proto.h"
#include "hid_desc.h"

#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#define RELEASE_KEY (0)

#define UART_SEND(len, data) uart_send(UART1_PORT, len, data)

/*
 * DEFINES
 ****************************************************************************************
 */
volatile uint8_t g_kb_leds;
void hids_led_lock(uint8_t leds)
{
    g_kb_leds = leds;
    
    pt_rsp_cmd_sta(PT_CMD_HID_KB_LED, g_kb_leds);
}

/// Uart Data procedure
void uart_proc(struct pt_pkt *pkt, uint8_t status)
{
    #if (DBG_PROC)
    uint16_t len = pkt->len;
    DEBUG("sta:0x%02X, cmd:%X, len:%d", status, pkt->code, len);
    debugHex((uint8_t *)pkt, len+3);
    #endif
    if (status != PT_OK)
    {
        return;
    }
    
    switch (pkt->code)
    {
        case PT_CMD_HID_KB:
        {
            PKT_PARAM(struct pt_cmd_hid_kb);
            
            uint8_t sta = keybd_report_send(app_env.curidx, param->keys_code);
            #if (RELEASE_KEY)
            sta = keybd_report_send(app_env.curidx, NULL);
            #endif
            pt_rsp_cmd_sta(PT_CMD_HID_KB, sta);
        } break;
        
        case PT_CMD_HID_KB_LED:
        {
            pt_rsp_cmd_sta(PT_CMD_HID_KB_LED, g_kb_leds);
        } break;
        
        case PT_CMD_HID_MEDIA:
        {
            PKT_PARAM(struct pt_cmd_hid_media);

            uint8_t sta = media_report_send(app_env.curidx, param->mkeys_code);
            #if (RELEASE_KEY)
            sta = media_report_send(app_env.curidx, NULL);
            #endif
            pt_rsp_cmd_sta(PT_CMD_HID_MEDIA, sta);
        } break;
        
        case PT_CMD_HID_SYSTEM:
        {
            PKT_PARAM(struct pt_cmd_hid_system);
            
            uint8_t sta = system_report_send(app_env.curidx, param->sys_code);
            #if (RELEASE_KEY)
            sta = system_report_send(app_env.curidx, NULL);
            #endif
            pt_rsp_cmd_sta(PT_CMD_HID_SYSTEM, sta);
        } break;
        
        case PT_CMD_HID_MOUSE:
        {
            PKT_PARAM(struct pt_cmd_hid_mouse);
            
            uint8_t sta = mouse_report_send(app_env.curidx, param->mouse_code);
            pt_rsp_cmd_sta(PT_CMD_HID_MOUSE, sta);
        } break;
        
        case PT_CMD_GET_APP_STA:
        {
            uint8_t app_sta = app_state_get();
            pt_rsp_cmd_sta(PT_CMD_GET_APP_STA, app_sta);
        } break;
        
        case PT_CMD_SLP:
        {
            pt_rsp_cmd_sta(PT_CMD_SLP, PT_OK);

            wakeup_io_sw(WAKEUP_IO_MASK, WAKEUP_IO_MASK);
            core_pwroff(CFG_WKUP_IO_EN | WKUP_IO_LATCH_N_BIT);
        } break;
        
        case PT_CMD_RST:
        {
            pt_rsp_cmd_sta(PT_CMD_RST, PT_OK);
            NVIC_SystemReset();
        } break;
        
        default:
        {
        } break;
    }
}

void user_procedure(void)
{
    proto_schedule();
}
