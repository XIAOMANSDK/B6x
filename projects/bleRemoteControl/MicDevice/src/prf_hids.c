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
#include "hid_desc.h"   // application-specific, #include "prf_hids.h"
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


/*
 * DEFINITIONS
 ****************************************************************************************
 */

/// Support Boot Protocol Mode or Not
#if !defined(HID_BOOT_SUP)
    #define HID_BOOT_SUP            (0)
    #define HID_BOOT_KB             (0)
    #define HID_BOOT_MOUSE          (0)
#endif

/// Support Report Protocol Mode or Not
#if !defined(HID_REPORT_SUP)
    #define HID_REPORT_SUP          (1)
    #define HID_RPT_KB              (1)
    #define HID_RPT_MEDIA           (1)
    #define HID_RPT_SYSTEM          (1)
    #define HID_RPT_MOUSE           (1)
    #define HID_RPT_PTP             (0)
#endif

/// Support Number of HID Connections
#if !defined(HID_CONN_MAX)
    #define HID_CONN_MAX            (1) // always one, BLE_CONNECTION_MAX
#endif

/// Macro for Client Config value operation
#define HID_RPT_NTF_GET(conidx, ntf_idx)  \
    ((hids_env.conn[conidx].rpt_ntfs >> (ntf_idx)) & PRF_CLI_START_NTF)

#define HID_RPT_NTF_CLR(conidx, ntf_idx)  \
    hids_env.conn[conidx].rpt_ntfs &= ~(PRF_CLI_START_NTF << (ntf_idx))

#define HID_RPT_NTF_SET(conidx, ntf_idx, conf)  \
    hids_env.conn[conidx].rpt_ntfs = (hids_env.conn[conidx].rpt_ntfs & ~(PRF_CLI_START_NTF << (ntf_idx))) | ((conf) << (ntf_idx))


/**
 ****************************************************************************************
 * @section ENVIRONMENT DEFINITION
 ****************************************************************************************
 */

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

    // Protocol Mode(0-BOOT, 1-REPORT)
    HID_IDX_PROTO_MODE_CHAR,
    HID_IDX_PROTO_MODE_VAL,
    
    // Report Map
    HID_IDX_REPORT_MAP_CHAR,
    HID_IDX_REPORT_MAP_VAL,
    
    #if (HID_BOOT_KB)
    // Boot Keyboard Input Report
    HID_IDX_BOOT_KB_IN_RPT_CHAR,
    HID_IDX_BOOT_KB_IN_RPT_VAL,
    HID_IDX_BOOT_KB_IN_RPT_NTF_CFG,
    // Boot Keyboard Output Report
    HID_IDX_BOOT_KB_OUT_RPT_CHAR,
    HID_IDX_BOOT_KB_OUT_RPT_VAL,
    #endif //(HID_BOOT_KB)

    #if (HID_REPORT_SUP)
    #if (HID_RPT_KB)
    // Keyboard Report IN
    HID_IDX_KB_IN_RPT_CHAR,
    HID_IDX_KB_IN_RPT_VAL,
    HID_IDX_KB_IN_RPT_NTF_CFG,
    HID_IDX_KB_IN_RPT_REF,
    // Keyboard Report OUT
    HID_IDX_KB_OUT_RPT_CHAR,
    HID_IDX_KB_OUT_RPT_VAL,
    HID_IDX_KB_OUT_RPT_REF,
    #endif //(HID_RPT_KB)
    
    #if (HID_RPT_MEDIA)
    // Media IN Report
    HID_IDX_MEDIA_IN_RPT_CHAR,
    HID_IDX_MEDIA_IN_RPT_VAL,
    HID_IDX_MEDIA_IN_RPT_NTF_CFG,
    HID_IDX_MEDIA_IN_RPT_REF,
    #endif //(HID_RPT_MEDIA)

    #if (HID_RPT_SYSTEM)
    // System IN Report
    HID_IDX_SYS_IN_RPT_CHAR,
    HID_IDX_SYS_IN_RPT_VAL,  
    HID_IDX_SYS_IN_RPT_REF,   
    HID_IDX_SYS_IN_RPT_NTF_CFG,
    #endif //(HID_RPT_SYSTEM)

    #if (HID_RPT_MOUSE)
    // Mouse IN Report
    HID_IDX_MOUSE_IN_RPT_CHAR,
    HID_IDX_MOUSE_IN_RPT_VAL,
    HID_IDX_MOUSE_IN_RPT_REF,
    HID_IDX_MOUSE_IN_RPT_NTF_CFG, 
    #endif //(HID_RPT_MOUSE)

    #if (HID_RPT_PTP)
    // TouchPad IN Report
    HID_IDX_TP_IN_RPT_CHAR,
    HID_IDX_TP_IN_RPT_VAL,
    HID_IDX_TP_IN_RPT_REF,
    HID_IDX_TP_IN_RPT_NTF_CFG,
    // MaxCnt FEAT Report
    HID_IDX_MAXCNT_FEAT_RPT_CHAR,  
    HID_IDX_MAXCNT_FEAT_RPT_VAL,
    HID_IDX_MAXCNT_FEAT_RPT_REF,
    // PTPHQA FEAT Report
    HID_IDX_PTPHQA_FEAT_RPT_CHAR,
    HID_IDX_PTPHQA_FEAT_RPT_VAL,
    HID_IDX_PTPHQA_FEAT_RPT_REF,  
    #endif //(HID_RPT_PTP)
    
    #if (HID_RPT_MIC)
    // MIC IN Report
    HID_IDX_MIC_IN_RPT_CHAR,
    HID_IDX_MIC_IN_RPT_VAL,           
    HID_IDX_MIC_IN_RPT_NTF_CFG,
    HID_IDX_MIC_IN_RPT_REF,
    #endif 
    #endif //(HID_REPORT_SUP)

    // Max Index, *NOTE* Minus 1(Svc Decl) is .nb_att
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
        
    #if (HID_BOOT_KB)
    // Boot Keyboard Input Report Char. Declaration and Value and CCC Descriptor
    ATT_ELMT_DECL_CHAR( HID_IDX_BOOT_KB_IN_RPT_CHAR ),
    ATT_ELMT( HID_IDX_BOOT_KB_IN_RPT_VAL,   ATT_CHAR_BOOT_KB_IN_REPORT,     PROP_RD | PROP_NTF | PROP_WR, HID_BOOT_REPORT_MAX_LEN ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( HID_IDX_BOOT_KB_IN_RPT_NTF_CFG ),

    // Boot Keyboard Output Report Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( HID_IDX_BOOT_KB_OUT_RPT_CHAR ),
    ATT_ELMT( HID_IDX_BOOT_KB_OUT_RPT_VAL,  ATT_CHAR_BOOT_KB_OUT_REPORT,    PROP_RD | PROP_WR | PROP_WC, HID_BOOT_REPORT_MAX_LEN ),
    #endif //(HID_BOOT_KB)

    #if (HID_REPORT_SUP)
    #if (HID_RPT_KB)
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
    #endif //(HID_RPT_KB)
    
    #if (HID_RPT_MEDIA)
    // Media IN Report Char. Declaration and Value, Report Ref. and CCC Descriptor
    ATT_ELMT_DECL_CHAR( HID_IDX_MEDIA_IN_RPT_CHAR ),
    ATT_ELMT( HID_IDX_MEDIA_IN_RPT_VAL,     ATT_CHAR_REPORT,                PROP_RD | PROP_NTF | PROP_WR, HID_REPORT_MAX_LEN ),
    ATT_ELMT_DESC_REPORT_REF( HID_IDX_MEDIA_IN_RPT_REF ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( HID_IDX_MEDIA_IN_RPT_NTF_CFG ),
    #endif //(HID_RPT_MEDIA)

    #if (HID_RPT_SYSTEM)
    // System IN Report Char. Declaration and Value, Report Ref. and CCC Descriptor
    ATT_ELMT_DECL_CHAR( HID_IDX_SYS_IN_RPT_CHAR ),
    ATT_ELMT( HID_IDX_SYS_IN_RPT_VAL,       ATT_CHAR_REPORT,                PROP_RD | PROP_NTF, HID_REPORT_MAX_LEN ),
    ATT_ELMT_DESC_REPORT_REF( HID_IDX_SYS_IN_RPT_REF ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( HID_IDX_SYS_IN_RPT_NTF_CFG ),
    #endif //(HID_RPT_SYSTEM)

    #if (HID_RPT_MOUSE)
    // Mouse IN Report Char. Declaration and Value, Report Ref. and CCC Descriptor
    ATT_ELMT_DECL_CHAR( HID_IDX_MOUSE_IN_RPT_CHAR ),
    ATT_ELMT( HID_IDX_MOUSE_IN_RPT_VAL,     ATT_CHAR_REPORT,                PROP_RD | PROP_NTF, HID_REPORT_MAX_LEN ),
    ATT_ELMT_DESC_REPORT_REF( HID_IDX_MOUSE_IN_RPT_REF ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( HID_IDX_MOUSE_IN_RPT_NTF_CFG ),
    #endif //(HID_RPT_MOUSE)

    #if (HID_RPT_PTP)
    // TouchPad IN Report Char. Declaration and Value, Report Ref. and CCC Descriptor
    ATT_ELMT_DECL_CHAR( HID_IDX_TP_IN_RPT_CHAR ),
    ATT_ELMT( HID_IDX_TP_IN_RPT_VAL,        ATT_CHAR_REPORT,                PROP_RD | PROP_NTF, HID_REPORT_MAX_LEN ),
    ATT_ELMT_DESC_REPORT_REF( HID_IDX_TP_IN_RPT_REF ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( HID_IDX_TP_IN_RPT_NTF_CFG ),

    // Max Count Feature Report Char. Declaration and Value, Report Ref. Descriptor
    ATT_ELMT_DECL_CHAR( HID_IDX_MAXCNT_FEAT_RPT_CHAR ),
    ATT_ELMT( HID_IDX_MAXCNT_FEAT_RPT_VAL,  ATT_CHAR_REPORT,                PROP_RD | PROP_WR,  HID_REPORT_MAX_LEN ),
    ATT_ELMT_DESC_REPORT_REF( HID_IDX_MAXCNT_FEAT_RPT_REF ),

    // PTPHQA Feature Report Char. Declaration and Value, Report Ref. Descriptor
    ATT_ELMT_DECL_CHAR( HID_IDX_PTPHQA_FEAT_RPT_CHAR ),
    ATT_ELMT( HID_IDX_PTPHQA_FEAT_RPT_VAL,  ATT_CHAR_REPORT,                PROP_RD | PROP_WR,  HID_REPORT_MAX_LEN ),
    ATT_ELMT_DESC_REPORT_REF( HID_IDX_PTPHQA_FEAT_RPT_REF ),
    #endif //(HID_RPT_PTP)

    #if (HID_RPT_MIC)
    // MIC IN
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MIC_IN_RPT_CHAR),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MIC_IN_RPT_VAL, ATT_CHAR_REPORT, PROP_RD | PROP_NTF | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Client Characteristic Configuration Descriptor
    ATT_ELMT(HID_IDX_MIC_IN_RPT_NTF_CFG, ATT_DESC_CLIENT_CHAR_CFG, PROP_RD | PROP_WR | PROP(WC), 0),    
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MIC_IN_RPT_REF , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    #endif
    #endif //(HID_REPORT_SUP)
};

/// Service Description
const struct svc_decl hid_svc_db = 
{
    .uuid   = ATT_SVC_HID, 
    .info   = SVC_UUID(16) | SVC_SEC(UNAUTH),
    .atts   = hid_atts,
    .nb_att = HID_IDX_NB - 1,
};


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @section SVC FUNCTIONS
 ****************************************************************************************
 */

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
        #if (HID_BOOT_SUP)
        #if (HID_BOOT_KB)
        case RPT_IDX_BOOT_KB:
        {
            att_idx = HID_IDX_BOOT_KB_IN_RPT_VAL;
        } break;
        #endif //(HID_BOOT_KB)

        #if (HID_BOOT_MOUSE)
        case RPT_IDX_BOOT_MOUSE:
        {
            att_idx = HID_IDX_BOOT_MOUSE_IN_RPT_VAL;
        } break;
        #endif //(HID_BOOT_MOUSE)
        #endif //(HID_BOOT_SUP)

        #if (HID_REPORT_SUP)
        #if (HID_RPT_KB)
        case RPT_IDX_KB:
        {
            att_idx = HID_IDX_KB_IN_RPT_VAL;
        } break;
        #endif //(HID_RPT_KB)
        
        #if (HID_RPT_MEDIA)
        case RPT_IDX_MEDIA:
        {
            att_idx = HID_IDX_MEDIA_IN_RPT_VAL;
        } break;
        #endif //(HID_RPT_MEDIA)
        
        #if (HID_RPT_SYSTEM)
        case RPT_IDX_SYSTEM:
        {
            att_idx = HID_IDX_SYS_IN_RPT_VAL;
        } break;
        #endif //(HID_RPT_SYSTEM)
        
        #if (HID_RPT_MOUSE)
        case RPT_IDX_MOUSE:
        {
            att_idx = HID_IDX_MOUSE_IN_RPT_VAL;
        } break;
        #endif //(HID_RPT_MOUSE)
        
        #if (HID_RPT_PTP)
        case RPT_IDX_TP:
        {
            att_idx = HID_IDX_TP_IN_RPT_VAL;
        } break;
        #endif //(HID_RPT_PTP)
        
        #if (HID_RPT_MIC)
        case RPT_IDX_MIC:
        {
            att_idx = HID_IDX_MIC_IN_RPT_VAL;
        } break;
        #endif
        #endif //(HID_REPORT_SUP)

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
    
/// Confirm ATTS_WRITE_REQ
static void hids_att_write_cfm(uint8_t conidx, uint8_t att_idx, uint16_t handle, const struct atts_write_ind *ind)
{
    uint8_t status = LE_SUCCESS;

    switch (att_idx)
    {
        case HID_IDX_HID_CTRL_PT_VAL:
        {
            ASSERT_ERR(ind->length == HID_CTRL_PT_SIZE);

            DEBUG("    HID_CTRL_PT:%d", ind->value[0]);
            hids_env.conn[conidx].ctrl_pt = ind->value[0];
        } break;
    
        #if (HID_BOOT_SUP)
        case HID_IDX_PROTO_MODE_VAL:
        {
            ASSERT_ERR(ind->length == HID_PROTO_MODE_SIZE);

            DEBUG("    PROTO_MODE:%d", ind->value[0]);
            hids_env.conn[conidx].proto_mode = ind->value[0];
        } break;
        
        #if (HID_BOOT_KB)
        case HID_IDX_BOOT_KB_IN_RPT_NTF_CFG:
        {
            status = hids_rpt_ntf_cfg(conidx, RPT_IDX_BOOT_KB, ind);
        } break;

        case HID_IDX_BOOT_KB_OUT_RPT_VAL:
        {
            ASSERT_ERR(ind->length == HID_KB_OUT_RPT_SIZE);

            DEBUG("    Led_Lock(b0:num,b1:caps,b2:scroll):0x%02x", ind->value[0]);
            hids_env.conn[conidx].led_lock = ind->value[0];
            hids_led_lock(ind->value[0]);
        } break;
        #endif //(HID_BOOT_KB)

        #endif //(HID_BOOT_SUP)
        
        #if (HID_REPORT_SUP)
        #if (HID_RPT_KB)
        case HID_IDX_KB_IN_RPT_NTF_CFG:
        {
            status = hids_rpt_ntf_cfg(conidx, RPT_IDX_KB, ind);
        } break;

        case HID_IDX_KB_OUT_RPT_VAL:
        {
            ASSERT_ERR(ind->length == HID_KB_OUT_RPT_SIZE);

            DEBUG("    Led_Lock(b0:num,b1:caps,b2:scroll):0x%02x", ind->value[0]);
            hids_env.conn[conidx].led_lock = ind->value[0];
            hids_led_lock(ind->value[0]);
        } break;
        #endif //(HID_RPT_KB)
        
        #if (HID_RPT_MEDIA)
        case HID_IDX_MEDIA_IN_RPT_NTF_CFG:
        {
            status = hids_rpt_ntf_cfg(conidx, RPT_IDX_MEDIA, ind);
        } break;
        #endif //(HID_RPT_MEDIA)

        #if (HID_RPT_SYSTEM)
        case HID_IDX_SYS_IN_RPT_NTF_CFG:
        {
            status = hids_rpt_ntf_cfg(conidx, RPT_IDX_SYSTEM, ind);
        } break;
        #endif //(HID_RPT_SYSTEM)

        #if (HID_RPT_MOUSE)
        case HID_IDX_MOUSE_IN_RPT_NTF_CFG:
        {
            status = hids_rpt_ntf_cfg(conidx, RPT_IDX_MOUSE, ind);
        } break;
        #endif //(HID_RPT_MOUSE)

        #if (HID_RPT_TP)
        case HID_IDX_TP_IN_RPT_NTF_CFG:
        {
            status = hids_rpt_ntf_cfg(conidx, RPT_IDX_TP, ind);
        } break;
        
        case HID_IDX_MAXCNT_FEAT_RPT_VAL:
        case HID_IDX_PTPHQA_FEAT_RPT_VAL:
        {
            // maybe do something
        } break;
        #endif //(HID_RPT_TP)
        
        #if (HID_RPT_MIC)
        case HID_IDX_MIC_IN_RPT_NTF_CFG:
        {
            status = hids_rpt_ntf_cfg(conidx, RPT_IDX_MIC, ind);
        } break;
        #endif //(HID_MIC)
        #endif //(HID_REPORT_SUP)

        default:
        {
            status = PRF_ERR_APP_ERROR;
        } break;
    }
    
    // Send write confirm, if no more data.
    if (!ind->more)
        gatt_write_cfm(conidx, status, handle);
}

const uint8_t hid_report_feature6[] =
{
    0x27, 0x17, 0x32, 0xb8, 0x36, 0x04, 0x64, 0x19, 0xa2, 0x00, 0x00, 0x00,    
};

/// Confirm ATTS_READ_REQ
static void hids_att_read_cfm(uint8_t conidx, uint8_t att_idx, uint16_t handle)
{
    uint16_t length = 0;
    
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
            const uint8_t *report_map = hid_get_report_map(&length);
            
            DEBUG("  Read REPORT_MAP(size:%d)", length);
            gatt_read_cfm(conidx, LE_SUCCESS, handle, length, report_map);
        } break;
        
        #if (HID_BOOT_SUP)
        case HID_IDX_PROTO_MODE_VAL:
        {
            DEBUG("  Read PROTO_MODE(mode:%d)", hids_env.conn[conidx].proto_mode);
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_PROTO_MODE_SIZE, &(hids_env.conn[conidx].proto_mode));
        } break;

        #if (HID_BOOT_KB)
        case HID_IDX_BOOT_KB_IN_RPT_VAL:
        {
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_BOOT_KB_RPT_SIZE, NULL); // zero array
        } break;

        case HID_IDX_BOOT_KB_IN_RPT_NTF_CFG:
        {
            uint16_t ntf_cfg = HID_RPT_NTF_GET(conidx, RPT_IDX_BOOT_KB);
            gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg); 
        } break;

        case HID_IDX_BOOT_KB_OUT_RPT_VAL:
        {
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_KB_OUT_RPT_SIZE, &(hids_env.conn[conidx].led_lock));
        } break;
        #endif //(HID_BOOT_KB)
        #endif //(HID_BOOT_SUP)
        
        #if (HID_REPORT_SUP)
        #if (HID_RPT_KB)
        case HID_IDX_KB_IN_RPT_VAL:
        {
            gatt_read_cfm(conidx, LE_SUCCESS, handle, RPT_LEN_KB, NULL); // zero array
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
        #endif //(HID_RPT_KB)
        
        #if (HID_RPT_MEDIA)
        case HID_IDX_MEDIA_IN_RPT_VAL:
        {
            gatt_read_cfm(conidx, LE_SUCCESS, handle, RPT_LEN_MEDIA, NULL); // zero array
        } break;

        case HID_IDX_MEDIA_IN_RPT_REF:
        {
            struct hid_report_ref refer;
            
            refer.report_id   = RPT_ID_MEDIA;
            refer.report_type = HID_REPORT_INPUT;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MEDIA_IN_RPT_NTF_CFG:
        {
            uint16_t ntf_cfg = HID_RPT_NTF_GET(conidx, RPT_IDX_MEDIA);
            gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg); 
        } break;
        #endif //(HID_RPT_MEDIA)
        
        #if (HID_RPT_SYSTEM)
        case HID_IDX_SYS_IN_RPT_VAL:
        {
            gatt_read_cfm(conidx, LE_SUCCESS, handle, RPT_LEN_SYSTEM, NULL); // zero array
        } break;
        
        case HID_IDX_SYS_IN_RPT_REF:
        {
            struct hid_report_ref refer;
            
            refer.report_id   = RPT_ID_SYSTEM;
            refer.report_type = HID_REPORT_INPUT;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_SYS_IN_RPT_NTF_CFG:
        {
            uint16_t ntf_cfg = HID_RPT_NTF_GET(conidx, RPT_IDX_SYSTEM);
            gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg); 
        } break;
        #endif //(HID_RPT_SYSTEM)
        
        #if (HID_RPT_MOUSE)
        case HID_IDX_MOUSE_IN_RPT_VAL:
        {
            gatt_read_cfm(conidx, LE_SUCCESS, handle, RPT_LEN_MOUSE, NULL); // zero array
        } break;
        
        case HID_IDX_MOUSE_IN_RPT_REF:
        {
            struct hid_report_ref refer;
            
            refer.report_id   = RPT_ID_MOUSE;
            refer.report_type = HID_REPORT_INPUT;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MOUSE_IN_RPT_NTF_CFG:
        {
            uint16_t ntf_cfg = HID_RPT_NTF_GET(conidx, RPT_IDX_MOUSE);
            gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg); 
        } break;
        #endif //(HID_RPT_MOUSE)
        
        #if (HID_RPT_PTP)
        case HID_IDX_TP_IN_RPT_VAL:
        {
            gatt_read_cfm(conidx, LE_SUCCESS, handle, RPT_LEN_TP, NULL); // zero array
        } break;
        
        case HID_IDX_TP_IN_RPT_REF:
        {
            struct hid_report_ref refer;
            
            refer.report_id   = RPT_ID_TP;
            refer.report_type = HID_REPORT_INPUT;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_TP_IN_RPT_NTF_CFG:
        {
            uint16_t ntf_cfg = HID_RPT_NTF_GET(conidx, RPT_IDX_TP);
            gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg); 
        } break;
        
        case HID_IDX_MAXCNT_FEAT_RPT_VAL:
        {
            uint8_t finger = PTP_MAX_FINGER_CNT;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint8_t), &finger);
        } break;
        
        case HID_IDX_MAXCNT_FEAT_RPT_REF:
        {
            struct hid_report_ref refer;
            
            refer.report_id   = RPT_ID_MAXCNT;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;
        
        case HID_IDX_PTPHQA_FEAT_RPT_VAL:
        {
            const uint8_t *ptp_hqa = hid_get_ptphqa_blob(&length);
            gatt_read_cfm(conidx, LE_SUCCESS, handle, length, ptp_hqa);
        } break;
        
        case HID_IDX_PTPHQA_FEAT_RPT_REF:
        {
            struct hid_report_ref refer;
            
            refer.report_id   = RPT_ID_PTPHQA;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;
        #endif //(HID_RPT_PTP)

        #if (HID_RPT_MIC)
        case HID_IDX_MIC_IN_RPT_REF:
        {
            struct hid_report_ref refer;
            refer.report_id = 6;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;
        #endif //(HID_RPT_MIC)
        #endif //(HID_REPORT_SUP)
        
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
    DEBUG("svc_func(cid:%d,op:0x%x,hdl:0x%x,att:%d)", conidx, opcode, handle, att_idx);

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
            
            if (!SADC->CTRL.SADC_DMAC_EN)
            {
                ble_latency_applied(true);
            }
            
            DEBUG("  cmp_evt(op:0x%x,sta:0x%x,nb:%d)", evt->operation, evt->status, hids_env.nb_pkt);
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
 * @section API FUNCTIONS
 ****************************************************************************************
 */

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
    debug("KB_HDL:%d   MEDIA_HDL:%d   MIC_HDL:%d\r\n", hids_get_rpt_handle(RPT_IDX_KB), hids_get_rpt_handle(RPT_IDX_MEDIA), hids_get_rpt_handle(RPT_IDX_MIC));
    return status;
}

/**
 ****************************************************************************************
 * @brief Show LED Lock of Keyboard Output, User Implement! (__weak func)
 *
 * @param[in] leds  Bits of Led_Lock(b0:num,b1:caps,b2:scroll)
 ****************************************************************************************
 */
__weak void hids_led_lock(uint8_t leds)
{
    // todo LED play...
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


#endif //(PRF_HIDS)
