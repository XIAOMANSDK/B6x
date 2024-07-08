/**
 ****************************************************************************************
 *
 * @file app_gatt.c
 *
 * @brief App SubTask of GATT Message - Example
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#if (GATT_CLI)

#include "drvs.h"
#include "app.h"
#include "gatt.h"
#include "gatt_api.h"
#include "usbd_hid.h"
#include "app_user.h"

#if (DBG_GATT)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */
#if (CFG_USB)
#ifndef NB_PKT_MAX
#define NB_PKT_MAX          32 // 2**n
#endif

//uint8_t pkt_kb[NB_PKT_MAX][KB_LEN];
//volatile uint16_t pkt_sidx, pkt_eidx;
#endif

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

APP_MSG_HANDLER(gatt_cmp_evt)
{
    DEBUG("Cmp_evt(op:%d,sta:0x%02x)", param->operation, param->status);
}

APP_MSG_HANDLER(gatt_mtu_changed_ind)
{
    DEBUG("mtu_chg:%d,seq:%d", param->mtu, param->seq_num);
}

APP_MSG_HANDLER(gatt_disc_svc_ind)
{
    DEBUG("disc_svc(shdl:0x%X,ehdl:0x%X,ulen:%d)", param->start_hdl, param->end_hdl, param->uuid_len);
    debugHex(param->uuid, param->uuid_len);
}

APP_MSG_HANDLER(gatt_disc_svc_incl_ind)
{
    DEBUG("disc_incl(ahdl:0x%X,shdl:0x%X,ehdl:0x%X,ulen:%d)", param->attr_hdl, param->start_hdl, param->end_hdl, param->uuid_len);
    debugHex(param->uuid, param->uuid_len);
}

APP_MSG_HANDLER(gatt_disc_char_ind)
{
    DEBUG("disc_char(ahdl:0x%X,phdl:0x%X,prop:0x%X,ulen:%d)", param->attr_hdl, param->pointer_hdl, param->prop, param->uuid_len);
    debugHex(param->uuid, param->uuid_len);
}

APP_MSG_HANDLER(gatt_disc_char_desc_ind)
{
    DEBUG("disc_desc(ahdl:0x%X,ulen:%d)", param->attr_hdl, param->uuid_len);
    debugHex(param->uuid, param->uuid_len);
}

APP_MSG_HANDLER(gatt_read_ind)
{
    DEBUG("Read_ind(hdl:0x%02x,oft:%d,len:%d)", param->handle, param->offset, param->length);
    debugHex(param->value, param->length);
}

#if (CFG_USB)
//uint8_t *get_kb_pkt(void)
//{
//    uint8_t *pkt = NULL;
//    
//    if (pkt_eidx != pkt_sidx)
//    {
//        pkt = pkt_kb[pkt_eidx];
//        pkt_eidx = (pkt_eidx + 1)  % NB_PKT_MAX;
//    }
//    
//    return pkt;
//}

#endif

APP_MSG_HANDLER(gatt_event_ind)
{
    DEBUG("Evt_ind(typ:%d,hdl:0x%02x,len:%d)", param->type, param->handle, param->length);
    //debugHex(param->value, param->length);
    
#if (CFG_USB)
    if (param->handle == GATT_KB_HDL)
    {
        debugHex(param->value, param->length);
        //xmemcpy(pkt_kb[pkt_sidx], param->value, KB_LEN);
        //pkt_sidx =  (pkt_sidx + 1) % NB_PKT_MAX;
        
        //usbd_kb_report();
        uint8_t hid_data[7];
        xmemset(hid_data, 0, 7);

        hid_data[0] = 5/*report_id*/;
        if (param->value[2] == HID_KEY_F5)
        {
            hid_data[1] = HID_KBD_USAGE_F18;
        }
        else
        {
            hid_data[1] = 0;
        }
        DEBUG("Hid(%X)",hid_data[1]);
        usbd_kb_report(7, hid_data);
    }
    else if (param->handle == GATT_MIC_HDL)
    {
        if (param->length == MIC_LEN)
        {
            GPIO_DAT_SET(GPIO18);
            usbd_mic_push(param->value);
            GPIO_DAT_CLR(GPIO18);
        }
    }
#endif
}

APP_MSG_HANDLER(gatt_event_req_ind)
{
    uint8_t conidx = TASK_IDX(src_id);
    
    DEBUG("Evt_req_ind(typ:%d,hdl:0x%02x,len:%d)", param->type, param->handle, param->length);
    debugHex(param->value, param->length);

    gatt_evt_cfm(conidx, param->handle);
}

/**
 ****************************************************************************************
 * @brief SubTask Handler of GATT Message.
 ****************************************************************************************
 */
APP_SUBTASK_HANDLER(gatt_msg)
{
    switch (msgid)
    {
        case GATT_CMP_EVT:
        {
            APP_MSG_FUNCTION(gatt_cmp_evt);
        } break;
        
        case GATT_MTU_CHANGED_IND:
        {
            APP_MSG_FUNCTION(gatt_mtu_changed_ind);
        } break;
        
        case GATT_DISC_SVC_IND:
        {
            APP_MSG_FUNCTION(gatt_disc_svc_ind);
        } break;
        
        case GATT_DISC_SVC_INCL_IND:
        {
            APP_MSG_FUNCTION(gatt_disc_svc_incl_ind);
        } break;
        
        case GATT_DISC_CHAR_IND:
        {
            APP_MSG_FUNCTION(gatt_disc_char_ind);
        } break;
        
        case GATT_DISC_CHAR_DESC_IND:
        {
            APP_MSG_FUNCTION(gatt_disc_char_desc_ind);
        } break;
        
        case GATT_READ_IND:
        {
            APP_MSG_FUNCTION(gatt_read_ind);
        } break;
        
        case GATT_EVENT_IND:
        {
            APP_MSG_FUNCTION(gatt_event_ind);
        } break;
        
        case GATT_EVENT_REQ_IND:
        {
            APP_MSG_FUNCTION(gatt_event_req_ind);
        } break;
        
        default:
        {
            DEBUG("Unknow MsgId:0x%X", msgid);
        } break;
    }

    return (MSG_STATUS_FREE);
}

#endif //(GATT_CLI)
