/**
 ****************************************************************************************
 *
 * @file prf_hids.c
 *
 * @brief HID Service Profile - Template Source.
 *
 * < Implementation according to user's application-specific >
 ****************************************************************************************
 */

#if (PRF_HIDS)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "prf.h"
#include "prf_hids.h"
#include "app_user.h"
#include "dbg.h"
#include "bledef.h"
#if (DBG_HIDS)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
//#define debugHex(dat,len)
#endif

/// Support Number of HID Connections
#define HID_CONN_MAX            (1) // always one, BLE_CONNECTION_MAX

/// Macro for Client Config value operation
#define HID_RPT_NTF_GET(conidx, ntf_idx)  \
    ((hids_env.conn[conidx].rpt_ntfs >> (ntf_idx)) & PRF_CLI_START_NTF)

#define HID_RPT_NTF_CLR(conidx, ntf_idx)  \
    hids_env.conn[conidx].rpt_ntfs &= ~(PRF_CLI_START_NTF << (ntf_idx))

#define HID_RPT_NTF_SET(conidx, ntf_idx, conf)  \
    hids_env.conn[conidx].rpt_ntfs = (hids_env.conn[conidx].rpt_ntfs & ~(PRF_CLI_START_NTF << (ntf_idx))) | ((conf) << (ntf_idx))


typedef struct hid_conn_tag
{
    // Client Config of peer devices, max_bits=RPT_IDX_NB
    uint8_t  rpt_ntfs;
    // keyboard Locks (bit0:num lock, bit1:caps lock, bit2:scroll lock)
    uint8_t  led_lock;
    // Control Point key @see enum hid_ctrl_pt
    uint8_t  ctrl_pt;
    // Boot or Report mode, @see enum hid_proto_mode
    uint8_t  proto_mode;
} hid_conn_t;

/// HIDS Server Environment Sariable
typedef struct hids_env_tag
{
    // Service Start Handle
    uint16_t   start_hdl;
    // Number of notify pkt
    uint8_t    nb_pkt;
    // Connection Info
    hid_conn_t conn[HID_CONN_MAX];
} hids_env_t;

/// Global Variable Declarations
__VAR_ENV hids_env_t hids_env;

/**
 ****************************************************************************************
 * @section ATTRIBUTES DEFINITION
 ****************************************************************************************
 */
/// HID Attributes Index
enum hid_att_idx
{
    // Service Declaration, *MUST* Start at 0
    HID_IDX_SVC,

    // HID Information
    HID_IDX_HID_INFO_CHAR,
    HID_IDX_HID_INFO_VAL,

    // HID Control Point
    HID_IDX_HID_CTRL_PT_CHAR,
    HID_IDX_HID_CTRL_PT_VAL,

    // Report Map
    HID_IDX_REPORT_MAP_CHAR,            //5
    HID_IDX_REPORT_MAP_VAL,
    
    // Protocol Mode(0-BOOT, 1-REPORT)
    HID_IDX_PROTO_MODE_CHAR,
    HID_IDX_PROTO_MODE_VAL,
    
    // Boot Keyboard Input Report
    HID_IDX_BOOT_KB_IN_RPT_CHAR,
    HID_IDX_BOOT_KB_IN_RPT_VAL,         //10
    HID_IDX_BOOT_KB_IN_RPT_NTF_CFG,
    // Boot Keyboard Output Report
    HID_IDX_BOOT_KB_OUT_RPT_CHAR,
    HID_IDX_BOOT_KB_OUT_RPT_VAL,

    // Keyboard Report IN
    // TAG: hid notify handle = 46            Report ID 1
    HID_IDX_KB_IN_RPT_CHAR,
    HID_IDX_KB_IN_RPT_VAL,
    HID_IDX_KB_IN_RPT_NTF_CFG,          //16
    HID_IDX_KB_IN_RPT_REF,
    // Keyboard Report OUT
    HID_IDX_KB_OUT_RPT_CHAR,
    HID_IDX_KB_OUT_RPT_VAL,
    HID_IDX_KB_OUT_RPT_REF,

    // Media IN Report                         Report ID 2
    HID_IDX_MEDIA_IN_RPT_CHAR,
    HID_IDX_MEDIA_IN_RPT_VAL,           //25
    HID_IDX_MEDIA_IN_RPT_NTF_CFG,
    HID_IDX_MEDIA_IN_RPT_REF,
    
    HID_IDX_NB,
};
/// Attributes Description
const att_decl_t hid_atts[] =
{
    // HID Information Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( HID_IDX_HID_INFO_CHAR ),
    ATT_ELMT( HID_IDX_HID_INFO_VAL,         ATT_CHAR_HID_INFO,              PROP_RD,    HID_INFO_SIZE ),
    
    // HID Control Point Char. Declaration
    ATT_ELMT_DECL_CHAR( HID_IDX_HID_CTRL_PT_CHAR ),
    ATT_ELMT( HID_IDX_HID_CTRL_PT_VAL,      ATT_CHAR_HID_CTRL_PT,           PROP_WC,    HID_CTRL_PT_SIZE ),

    // Protocol Mode Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( HID_IDX_PROTO_MODE_CHAR ),
    ATT_ELMT( HID_IDX_PROTO_MODE_VAL,       ATT_CHAR_PROTOCOL_MODE,         PROP_RD | PROP_WC,  HID_PROTO_MODE_SIZE ),

    // Report Map Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( HID_IDX_REPORT_MAP_CHAR ),
    ATT_ELMT( HID_IDX_REPORT_MAP_VAL,       ATT_CHAR_REPORT_MAP,            PROP_RD,    HID_REPORT_MAP_MAX_LEN),

    // #if (HID_BOOT_KB)
    // Boot Keyboard Input Report Char. Declaration and Value and CCC Descriptor
    ATT_ELMT_DECL_CHAR( HID_IDX_BOOT_KB_IN_RPT_CHAR ),
    ATT_ELMT( HID_IDX_BOOT_KB_IN_RPT_VAL,   ATT_CHAR_BOOT_KB_IN_REPORT,     PROP_RD | PROP_NTF | PROP_WR, HID_BOOT_REPORT_MAX_LEN ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( HID_IDX_BOOT_KB_IN_RPT_NTF_CFG ),

    // Boot Keyboard Output Report Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( HID_IDX_BOOT_KB_OUT_RPT_CHAR ),
    ATT_ELMT( HID_IDX_BOOT_KB_OUT_RPT_VAL,  ATT_CHAR_BOOT_KB_OUT_REPORT,    PROP_RD | PROP_WR | PROP_WC, HID_BOOT_REPORT_MAX_LEN ),
    // #endif //(HID_BOOT_KB)

    // #if (HID_RPT_KB)
    // Keyboard IN Report Char. Declaration and Value, Report Ref. and CCC Descriptor
    ATT_ELMT_DECL_CHAR( HID_IDX_KB_IN_RPT_CHAR ),
    ATT_ELMT( HID_IDX_KB_IN_RPT_VAL,        ATT_CHAR_REPORT,                PROP_RD | PROP_NTF | PROP_WR, HID_REPORT_MAX_LEN ),
    ATT_ELMT_DESC_REPORT_REF( HID_IDX_KB_IN_RPT_REF ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( HID_IDX_KB_IN_RPT_NTF_CFG ),
    
    // Keyboard OUT Report Char. Declaration and Value, Report Ref. Descriptor
    ATT_ELMT_DECL_CHAR( HID_IDX_KB_OUT_RPT_CHAR ),
    // Report Characteristic Value
    ATT_ELMT( HID_IDX_KB_OUT_RPT_VAL,       ATT_CHAR_REPORT,                PROP_RD | PROP_WR | PROP_WC, HID_REPORT_MAX_LEN ),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT_DESC_REPORT_REF( HID_IDX_KB_OUT_RPT_REF ),
    // #endif //(HID_RPT_KB)

    // Keyboard IN Report Char. Declaration and Value, Report Ref. and CCC Descriptor
    ATT_ELMT_DECL_CHAR( HID_IDX_MEDIA_IN_RPT_CHAR ),
    ATT_ELMT( HID_IDX_MEDIA_IN_RPT_VAL,        ATT_CHAR_REPORT,                PROP_RD | PROP_NTF | PROP_WR, HID_REPORT_MAX_LEN ),
    ATT_ELMT_DESC_REPORT_REF( HID_IDX_MEDIA_IN_RPT_REF ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( HID_IDX_MEDIA_IN_RPT_NTF_CFG ),
};

/// Service Description
const struct svc_decl hid_svc_db = 
{
    .uuid   = ATT_SVC_HID, 
    .info   = SVC_UUID(16),
    .atts   = hid_atts,
    .nb_att = HID_IDX_NB - 1,
};

static const uint8_t hid_report_map[] = {
0x05, 0x01,            // Usage Page (Generic Desktop Ctrls)
0x09, 0x06,            // Usage (Keyboard)
0xA1, 0x01,            // Collection (Application)
    0x85, RPT_ID_KB,   //   Report ID (1)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x1E,        //   Logical Minimum (30)
    0x25, 0xF1,        //   Logical Maximum (-15)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x1E,        //   Usage Minimum (0x1E)
    0x29, 0xF1,        //   Usage Maximum (0xF1)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
        0x05, 0x0C,        // Usage Page (Consumer)
        0x09, 0x01,        // Usage (Consumer Control)
        0xA1, 0x01,        // Collection (Application)
        0x85, RPT_ID_MEDIA,//   Report ID (2)
        0x75, 0x10,        //   Report Size (16)
        0x95, 0x02,        //   Report Count (2)
        0x15, 0x01,        //   Logical Minimum (1)
        0x26, 0x8C, 0x02,  //   Logical Maximum (652)
        0x19, 0x01,        //   Usage Minimum (Consumer Control)
        0x2A, 0x8C, 0x02,  //   Usage Maximum (AC Send)
        0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              // End Collection
};


/// Retrieve attribute index form handle
static uint8_t hids_get_att_idx(uint16_t handle)
{
    ASSERT_ERR((handle >= hids_env.start_hdl) && (handle < hids_env.start_hdl + HID_IDX_NB));

    return (handle - hids_env.start_hdl);
}

/// Retrieve Report attribute handle from rpt_idx (@see rpt_ntf_idx) or ATT_INVALID_HDL
static uint16_t hids_get_rpt_handle(uint8_t rpt_idx)
{
    uint8_t att_idx = 0;
    
    switch (rpt_idx)
    {
        case RPT_IDX_BOOT_KB:
        {
            att_idx = HID_IDX_BOOT_KB_IN_RPT_VAL;
        } break;
        
        case RPT_IDX_KB:
        {
            att_idx = HID_IDX_KB_IN_RPT_VAL;
        } break;

        case RPT_IDX_MEDIA:
        {
            att_idx = HID_IDX_MEDIA_IN_RPT_VAL;
        } break;

        default:
        {
            att_idx = 0; // error index
        } break;
    }

    return (att_idx) ? (hids_env.start_hdl + att_idx) : ATT_INVALID_HDL;
}

/// update configuration if value for stop or NTF start
static uint8_t hids_rpt_ntf_cfg(uint8_t conidx, uint8_t rpt_idx, const struct atts_write_ind *ind)
{
    uint8_t status = PRF_ERR_APP_ERROR;
    
    if ((!ind->more) && (ind->length == sizeof(uint16_t)))
    {
        uint16_t cli_cfg = read16p(ind->value);

        //if ((cli_cfg == PRF_CLI_STOP_NTFIND) || (cli_cfg == PRF_CLI_START_NTF))
        if (cli_cfg <= PRF_CLI_START_NTF)
        {
            DEBUG("    RPT_NTF_CFG(idx:%d,cfg:%d)", rpt_idx, cli_cfg);

            HID_RPT_NTF_SET(conidx, rpt_idx, cli_cfg);
            status = LE_SUCCESS;
        }
    }
    
    return status;
}

extern struct key_env_tag key_env;

/// Confirm ATTS_WRITE_REQ
static void hids_att_write_cfm(uint8_t conidx, uint8_t att_idx, uint16_t handle, const struct atts_write_ind *ind)
{
    uint8_t status = LE_SUCCESS;

    switch (att_idx)
    {
        case HID_IDX_KB_IN_RPT_NTF_CFG:
        {
            status = hids_rpt_ntf_cfg(conidx, RPT_IDX_KB, ind);
        } break;
        
        case HID_IDX_MEDIA_IN_RPT_NTF_CFG:
        {
            status = hids_rpt_ntf_cfg(conidx, RPT_IDX_MEDIA, ind);
        } break;

        default:
        {
            status = PRF_ERR_APP_ERROR;
        } break;
    }
    
    // Send write confirm, if no more data.
    if (!ind->more)
        gatt_write_cfm(conidx, status, handle);
}

/// Confirm ATTS_READ_REQ
static void hids_att_read_cfm(uint8_t conidx, uint8_t att_idx, uint16_t handle)
{
    switch (att_idx)
    {
        case HID_IDX_HID_INFO_VAL:
        {
            struct hid_info_tag hid_info;
        
            hid_info.bcdHID       = HID_INFO_BCDHID;
            hid_info.bCountryCode = HID_INFO_BCODE;
            hid_info.flags        = HID_INFO_FLAGS;
            DEBUG("  Read HID_INFO(bcd:0x%04X)", hid_info.bcdHID);
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_INFO_SIZE, (uint8_t *)&hid_info);
        } break;

        case HID_IDX_REPORT_MAP_VAL:
        {
            DEBUG("  Read REPORT_MAP(size:%d)", sizeof(hid_report_map));
            gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(hid_report_map), hid_report_map);
        } break;

        // #if (HID_RPT_KB)
        case HID_IDX_KB_IN_RPT_VAL:
        {
//            gatt_read_cfm(conidx, LE_SUCCESS, handle, RPT_LEN_KB, NULL); // zero array
            gatt_read_cfm(conidx, LE_SUCCESS, handle, RPT_LEN_MEDIA, NULL); // zero array
        } break;

        case HID_IDX_KB_IN_RPT_REF:
        {
            struct hid_report_ref refer;
            
            refer.report_id   = RPT_ID_KB;
            refer.report_type = HID_REPORT_INPUT;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_KB_IN_RPT_NTF_CFG:
        {
            uint16_t ntf_cfg = HID_RPT_NTF_GET(conidx, RPT_IDX_KB);
            gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg); 
        } break;

        case HID_IDX_KB_OUT_RPT_VAL:
        {
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_KB_OUT_RPT_SIZE, &(hids_env.conn[conidx].led_lock));
        } break;

        case HID_IDX_KB_OUT_RPT_REF:
        {
            struct hid_report_ref refer;
            
            refer.report_id   = RPT_ID_KB;
            refer.report_type = HID_REPORT_OUTPUT;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;
        // #endif //(HID_RPT_KB)

        case HID_IDX_MEDIA_IN_RPT_REF:
        {
            struct hid_report_ref refer;
            refer.report_id = RPT_ID_MEDIA;
            refer.report_type = HID_REPORT_INPUT;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;
        
        case HID_IDX_MEDIA_IN_RPT_NTF_CFG:
        {
            uint16_t ntf_cfg = HID_RPT_NTF_GET(conidx, RPT_IDX_MEDIA);
            gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg); 
        } break;

        default:
        {
            // error response
            gatt_read_cfm(conidx, PRF_ERR_APP_ERROR, handle, 0, NULL);
        } break;
    }
}

/// Handles reception of the atts request from peer device
static void hids_svc_func(uint8_t conidx, uint8_t opcode, uint16_t handle, const void *param)
{
    uint8_t att_idx = hids_get_att_idx(handle);
    
    ASSERT_ERR(coindx < HID_CONN_MAX);
    // DEBUG("svc_func(condix:%d,opcode:0x%x,hdl=0x%x %d,att_idx=%d)", conidx, opcode, handle, handle, att_idx);

    switch (opcode)
    {
        case ATTS_READ_REQ:
        {
            hids_att_read_cfm(conidx, att_idx, handle);
        } break;
        
        case ATTS_WRITE_REQ:
        {
            const struct atts_write_ind *ind = param;

            DEBUG("  write_req(hdl:0x%x,att:%d,wr:0x%x,len:%d)", handle, att_idx, ind->wrcode, ind->length);
            hids_att_write_cfm(conidx, att_idx, handle, ind);
        } break;
        
        case ATTS_INFO_REQ:
        {
            uint16_t length = ATT_MAX_LEN_GET(att_idx, hid_atts);
            
            // Send length-info confirm for prepWR judging.
            DEBUG("  info_cfm(hdl:0x%x,att:%d,len:%d)", handle, att_idx, length);
            gatt_info_cfm(conidx, LE_SUCCESS, handle, length);
        } break;
        
        case ATTS_CMP_EVT:
        {
            const struct atts_cmp_evt *evt = param;

            hids_env.nb_pkt++; // release
            
            debug("%d ", hids_env.nb_pkt);
            // DEBUG("  cmp_evt(op:0x%x,sta:0x%x,nb:%d)", evt->operation, evt->status, hids_env.nb_pkt);
            // add 'if' to avoid warning #117-D: "evt" never referenced
            if (evt->operation == GATT_NOTIFY)
            {
                // Notify result
            }
        } break;

        default:
        {
            // nothing to do
        } break;
    }
}

/**
 ****************************************************************************************
 * @brief Add HID Service Profile in the DB
 *        Customize via pre-define @see HID_START_HDL
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t hids_prf_init(void)
{
    uint8_t status = LE_SUCCESS;

    // Init Environment
    hids_env.start_hdl  = HID_START_HDL;
    hids_env.nb_pkt     = HID_NB_PKT_MAX;
    
    for (uint8_t conidx = 0; conidx < HID_CONN_MAX; conidx++)
    {
        hids_env.conn[conidx].rpt_ntfs   = RPT_NTF_ALL;
        hids_env.conn[conidx].led_lock   = 0;
        hids_env.conn[conidx].proto_mode = HID_REPORT_PROTOCOL_MODE;
        hids_env.conn[conidx].ctrl_pt    = HID_CTRL_PT_EXIT_SUSPEND;
    }

    // Create Service in database
    status = attmdb_svc_create(&hids_env.start_hdl, NULL, &hid_svc_db, hids_svc_func);
    DEBUG("svc_init(sta:0x%X,shdl:%d,atts:%d)", status, hids_env.start_hdl, HID_IDX_NB-1);

    return status;
}

/**
 ****************************************************************************************
 * @brief Enable HID Notification Configurations.
 *
 * @param[in] conidx    Connection index
 * @param[in] rpt_ntf   Notification Config Bits @see enum rpt_ntf_idx
 ****************************************************************************************
 */
void hids_set_ccc(uint8_t conidx, uint8_t rpt_ntf)
{
    //if (gapc_get_conhdl(conidx) != GAP_INVALID_CONHDL)
    {
        // update configuration
        hids_env.conn[conidx].rpt_ntfs  = rpt_ntf;
    }
}

/**
 ****************************************************************************************
 * @brief Send HID Report to Host peer.
 *
 * @param[in] conidx   Connection Index
 * @param[in] rep_idx  Report Index
 * @param[in] rep_len  Report Length
 * @param[in] rep_val  Report Value
 *
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
uint8_t hids_report_send(uint8_t conidx, uint8_t rep_idx, uint16_t rep_len, const uint8_t* rep_val)
{
    uint8_t status = PRF_ERR_REQ_DISALLOWED;
    
    if ((rep_len > 0) && (hids_env.nb_pkt > 0))
    {
        if (HID_RPT_NTF_GET(conidx, rep_idx) == PRF_CLI_START_NTF)
        {
            uint16_t handle = hids_get_rpt_handle(rep_idx);
            
            if (handle != ATT_INVALID_HDL)
            {
                hids_env.nb_pkt--; // allocate
                DEBUG("hid_ntf_send(len:%d,nb:%d)", rep_len, hids_env.nb_pkt);
//                debugHex(rep_val, rep_len);
                gatt_ntf_send(conidx, handle, rep_len, rep_val);
                status = LE_SUCCESS;
            }
        }
        else
        {
            status = PRF_ERR_NTF_DISABLED;
        }
    }
    
    return status;
}

//int ble_hid_kb_report_send(uint16_t keycode)
//{
//    if (hids_env.nb_pkt == 0)
//        return PRF_ERR_REQ_DISALLOWED;

//    uint8_t report[RPT_LEN_MEDIA] = {0};
//    
//    report[2] = (keycode & 0xFF);
//    report[3] = ((keycode >> 8) & 0xFF);
//    
//    uint16_t handle = hids_env.start_hdl + HID_IDX_KB_IN_RPT_VAL;
//    DEBUG("kb report hd=%d keycode=0x%04X \r\n", handle, keycode);
//    // debugHex(report, sizeof(report));

//    hids_env.nb_pkt--;
//    gatt_ntf_send(app_env.curidx, handle, sizeof(report), report);

//    return 0;
//}

int ble_hid_kb_report_send(uint16_t keycode)
{
    if (hids_env.nb_pkt == 0)
        return PRF_ERR_REQ_DISALLOWED;

    uint8_t report[RPT_LEN_MEDIA] = {0};
    
    report[0] = (keycode & 0xFF);
    report[1] = ((keycode >> 8) & 0xFF);
    
    uint16_t handle = hids_env.start_hdl + HID_IDX_MEDIA_IN_RPT_VAL;
    DEBUG("kb report hd=%d keycode=0x%04X \r\n", handle, keycode);
    // debugHex(report, sizeof(report));

    hids_env.nb_pkt--;
    gatt_ntf_send(app_env.curidx, handle, sizeof(report), report);

    return 0;
}
#endif //(PRF_HIDS)
