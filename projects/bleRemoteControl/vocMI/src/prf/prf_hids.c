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
#include "pt.h"
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

// 主机回应语音是否可以开始发送的flag
uint8_t voice_start_flag;

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
    // TAG: hid notify handle = 46
    HID_IDX_KB_IN_RPT_CHAR,
    HID_IDX_KB_IN_RPT_VAL,
    HID_IDX_KB_IN_RPT_NTF_CFG,          //16
    HID_IDX_KB_IN_RPT_REF,
    // Keyboard Report OUT
    HID_IDX_KB_OUT_RPT_CHAR,
    HID_IDX_KB_OUT_RPT_VAL,
    HID_IDX_KB_OUT_RPT_REF,
    
    // Keyboard Report IN 1                    Report ID 3
    HID_IDX_KB_IN_REPORT_CHAR1,
    HID_IDX_KB_IN_REPORT_VAL1,
    HID_IDX_KB_IN_REPORT_NTF_CFG1,
    HID_IDX_KB_IN_REPORT_REP_REF1,
    
    // Fetrure0                                Report ID 4
    // TAG: write voice control handle
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR0, //25
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL0,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF0,

    // Fetrure1                                Report ID 5
    // TAG: notify voice control handle
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR1,
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL1,
    HID_IDX_MAX_CNT_FEATURE_REPORT_NTF_CFG1, //30
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF1,
    
    // Fetrure2                                Report ID 6
    // TAG: voice handle 1
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR2,
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL2,
    HID_IDX_MAX_CNT_FEATURE_REPORT_NTF_CFG2,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF2,//35

    // Fetrure3                                Report ID 7
    // TAG: voice handle 2
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR3, 
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL3,
    HID_IDX_MAX_CNT_FEATURE_REPORT_NTF_CFG3,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF3,
    
    // Fetrure4                                Report ID 8
    // TAG: voice handle 3
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR4, //40
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL4,
    HID_IDX_MAX_CNT_FEATURE_REPORT_NTF_CFG4,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF4,
    
    // Keyboard Report OUT 1                   Report ID 9
    HID_IDX_KB_OUT_REPORT_CHAR1,
    HID_IDX_KB_OUT_REPORT_VAL1,  //45
    HID_IDX_KB_OUT_REPORT_REP_REF1, 
    
    // Fetrure5                                Report ID 10
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR5,
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL5,
    HID_IDX_MAX_CNT_FEATURE_REPORT_NTF_CFG5,    
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF5, //50

    // Fetrure6                                Report ID 224
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR6, 
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL6,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF6,

    // Fetrure7                                Report ID 225
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR7, 
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL7,  //55
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF7,
    
    // Fetrure8                                Report ID 226
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR8, 
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL8,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF8,

    // Fetrure9                                Report ID 227
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR9, //60
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL9,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF9,
    
    // Fetrure10                                Report ID 228
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR10, 
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL10,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF10, //65

    // Fetrure11                                Report ID 229
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR11, 
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL11,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF11,
    
    // Fetrure12                                Report ID 230
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR12, 
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL12, //70
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF12,

    // Fetrure13                                Report ID 231
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR13,
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL13,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF13,
    
    // Fetrure14                                Report ID 232
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR14, //75
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL14,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF14,

    // Fetrure15                                Report ID 233
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR15, 
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL15,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF15,  //80
    
    // Fetrure16                                Report ID 234
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR16, 
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL16,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF16,    

    // Fetrure17                                Report ID 235
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR17, 
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL17,   //85
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF17,  
    
    // Fetrure18                                Report ID 236
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR18, 
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL18,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF18,  

    // Fetrure19                                Report ID 237
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR19, //90
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL19,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF19,  

    // Fetrure20                                Report ID 238
    HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR20, 
    HID_IDX_MAX_CNT_FEATURE_REPORT_VAL20,
    HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF20,  //95

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

    // #if (VOICE)
    // Keyboard IN 1
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_KB_IN_REPORT_CHAR1),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_KB_IN_REPORT_VAL1, ATT_CHAR_REPORT, PROP_RD | PROP_NTF | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Client Characteristic Configuration Descriptor
    ATT_ELMT(HID_IDX_KB_IN_REPORT_NTF_CFG1, ATT_DESC_CLIENT_CHAR_CFG, PROP_RD | PROP_WR | PROP(WC), 0),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_KB_IN_REPORT_REP_REF1 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    
    // Max Count Feature0
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR0),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL0, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF0 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),

    // Max Count Feature1
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR1),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL1, ATT_CHAR_REPORT, PROP_RD | PROP_NTF | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Client Characteristic Configuration Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_NTF_CFG1, ATT_DESC_CLIENT_CHAR_CFG, PROP_RD | PROP_WR | PROP(WC), 0),    
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF1 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    
    // Max Count Feature2
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR2),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL2, ATT_CHAR_REPORT, PROP_RD | PROP_NTF | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Client Characteristic Configuration Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_NTF_CFG2, ATT_DESC_CLIENT_CHAR_CFG, PROP_RD | PROP_WR | PROP(WC), 0),    
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF2 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),

    // Max Count Feature3
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR3),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL3, ATT_CHAR_REPORT, PROP_RD | PROP_NTF | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Client Characteristic Configuration Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_NTF_CFG3, ATT_DESC_CLIENT_CHAR_CFG, PROP_RD | PROP_WR | PROP(WC), 0),    
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF3 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    
    // Max Count Feature4
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR4),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL4, ATT_CHAR_REPORT, PROP_RD | PROP_NTF | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Client Characteristic Configuration Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_NTF_CFG4, ATT_DESC_CLIENT_CHAR_CFG, PROP_RD | PROP_WR | PROP(WC), 0),    
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF4 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),

    // Keyboard OUT1
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_KB_OUT_REPORT_CHAR1),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_KB_OUT_REPORT_VAL1, ATT_CHAR_REPORT, PROP_RD | PROP_WR | PROP(WC), HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_KB_OUT_REPORT_REP_REF1 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    
    // Max Count Feature5
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR5),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL5, ATT_CHAR_REPORT, PROP_RD | PROP_NTF | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Client Characteristic Configuration Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_NTF_CFG5, ATT_DESC_CLIENT_CHAR_CFG, PROP_RD | PROP_WR | PROP(WC), 0),    
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF5 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    
    // Max Count Feature6
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR6),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL6, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF6 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),


    // Max Count Feature7
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR7),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL7, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF7 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    
    
    // Max Count Feature8
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR8),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL8, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF8 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),


    // Max Count Feature9
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR9),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL9, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF9 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),

    // Max Count Feature10
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR10),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL10, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF10 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),


    // Max Count Feature11
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR11),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL11, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF11 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    
    
    // Max Count Feature12
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR12),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL12, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF12 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),


    // Max Count Feature13
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR13),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL13, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF13 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    
    // Max Count Feature14
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR14),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL14, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF14 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),

    // Max Count Feature15
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR15),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL15, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF15 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    
    // Max Count Feature16
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR16),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL16, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF16 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),

    // Max Count Feature17
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR17),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL17, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF17 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    
    // Max Count Feature18
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR18),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL18, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF18 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),

    // Max Count Feature19
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR19),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL19, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF19 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    
    // Max Count Feature20
    // Report Characteristic Declaration
    ATT_ELMT_DECL_CHAR(HID_IDX_MAX_CNT_FEATURE_REPORT_CHAR20),
    // Report Characteristic Value
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_VAL20, ATT_CHAR_REPORT, PROP_RD | PROP_WR, HID_REPORT_MAX_LEN),
    // Report Characteristic - Report Reference Descriptor
    ATT_ELMT(HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF20 , ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0),
    // #endif 
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
    0x05,    0x01,
    0x09,    0x06,
    0xA1,    0x01,
        0x85,    RPT_ID_KB,
        0x05,    0x07,
        0x19,    0xE0,
        0x29,    0xE7,
        0x15,    0x00,
        0x25,    0x01,
        0x75,    0x01,
        0x95,    0x08,
        0x81,    0x02,
        0x95,    0x01,
        0x75,    0x08,
        0x81,    0x01,
        0x95,    0x05,
        0x75,    0x01,
        0x05,    0x08,
        0x19,    0x01,
        0x29,    0x05,
        0x91,    0x02,
        0x95,    0x01,
        0x75,    0x03,
        0x91,    0x01,
        0x95,    0x06,
        0x75,    0x08,
        0x15,    0x28,
        0x25,    0xFE,
        0x05,    0x07,
        0x19,    0x28,
        0x29,    0xFE,
        0x81,    0x00,
        0x05,    0x0C,
        0x09,    0x01,
        0xA1,    0x01,
            0x85,    0xF1, /*!< RPT_ID_MEDIA */
            0x15,    0x00,
            0x25,    0x01,
            0x75,    0x01,
            0x95,    0x18,
            0x09,    0xB5,
            0x09,    0xB6,
            0x09,    0xB7,
            0x09,    0xCD,
            0x09,    0xE2,
            0x09,    0xE5,
            0x09,    0xE7,
            0x09,    0xE9,
            0x09,    0xEA,
            0x0A,    0x52,    0x01,
            0x0A,    0x53,    0x01,
            0x0A,    0x54,    0x01,
            0x0A,    0x55,    0x01,
            0x0A,    0x83,    0x01,
            0x0A,    0x8A,    0x01,
            0x0A,    0x92,    0x01,
            0x0A,    0x94,    0x01,
            0x0A,    0x21,    0x02,
            0x0A,    0x23,    0x02,
            0x0A,    0x24,    0x02,
            0x0A,    0x25,    0x02,
            0x0A,    0x26,    0x02,
            0x0A,    0x27,    0x02,
            0x0A,    0x2A,    0x02,
            0x81,    0x02,
        0xC0,
    0xC0,
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

        case RPT_IDX_FEATURE1_INPUT:
        {
            att_idx = HID_IDX_MAX_CNT_FEATURE_REPORT_VAL1;
        } break;

        case RPT_IDX_FEATURE2_INPUT:
        {
            att_idx = HID_IDX_MAX_CNT_FEATURE_REPORT_VAL2;
        } break;

        case RPT_IDX_FEATURE3_INPUT:
        {
            att_idx = HID_IDX_MAX_CNT_FEATURE_REPORT_VAL3;
        } break;

        case RPT_IDX_FEATURE4_INPUT:
        {
            att_idx = HID_IDX_MAX_CNT_FEATURE_REPORT_VAL4;
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

        case HID_IDX_MAX_CNT_FEATURE_REPORT_VAL0: /*!< wirte voice control handle */
        {
            // connect
            // 04 00 00 80 10 10 00 31 06 40 
            // voice start
            // 00 00 00 80 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
            // voice stop
            // 00 00 00 80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
            status = PRF_ERR_NO_ERROR;
            // debugHex(ind->value, ind->length);

            uint32_t cmd = read32p(ind->value);//ind->value[0] | (ind->value[1] << 8) | (ind->value[2] << 16) | (ind->value[3] << 24);
            uint8_t audio_op = ind->value[4];
            PT_LOGD("cmd=0x%X audio_op=%d\n", cmd, audio_op);
            if (cmd == 0x80000000)
            {
                voice_start_flag = audio_op;
            }
            else if (cmd == 0x80000004)
            {
                uint32_t cmd1 = ind->value[4] | (ind->value[5] << 8) | (ind->value[6] << 16) | (ind->value[7] << 24);
                uint16_t cmd2 = ind->value[8] | (ind->value[9] << 8);
                // TODO: 
                PT_LOGD("0x80000004: cmd1=0x%X, cmd2=0x%X\n", cmd1, cmd2);
            }
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
        // #endif //(HID_RPT_KB)

        case HID_IDX_KB_IN_REPORT_REP_REF1:
        {
            struct hid_report_ref refer;
            refer.report_id = 3;
            refer.report_type = HID_REPORT_INPUT;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;
        
        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF0:
        {
            struct hid_report_ref refer;
            refer.report_id = 4;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF1:
        {
            struct hid_report_ref refer;
            refer.report_id = 5;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF2:
        {
            struct hid_report_ref refer;
            refer.report_id = 6;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF3:
        {
            struct hid_report_ref refer;
            refer.report_id = 7;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF4:
        {
            struct hid_report_ref refer;
            refer.report_id = 8;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;
        
        case HID_IDX_KB_OUT_REPORT_REP_REF1:
        {
            struct hid_report_ref refer;
            refer.report_id = 67;//REPORT_ID_KB;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;
        
        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF5:
        {
            struct hid_report_ref refer;
            refer.report_id = 83;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;
        
        case HID_IDX_MAX_CNT_FEATURE_REPORT_VAL6:
        {
            const uint8_t hid_report_feature6[] = { 0x27, 0x17, 0x32, 0xb8, 0x36, 0x04, 0x64, 0x19, 0xa2, 0x00, 0x00, 0x00 };
            gatt_read_cfm(conidx, LE_SUCCESS, handle, 12, hid_report_feature6);
        }break;
        
        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF6:
        {
            struct hid_report_ref refer;
            refer.report_id = 224;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_VAL7:
        {
            const uint8_t hid_report_feature7[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x20 };
            gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(hid_report_feature7), hid_report_feature7);
        }break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF7:
        {
            struct hid_report_ref refer;
            refer.report_id = 225;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF8:
        {
            struct hid_report_ref refer;
            refer.report_id = 226;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_VAL9:
        {
            const uint8_t hid_report_feature9[] = { 0x00 };
            gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(hid_report_feature9), hid_report_feature9);
        }break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF9:
        {
            struct hid_report_ref refer;
            refer.report_id = 227;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF10:
        {
            struct hid_report_ref refer;
            refer.report_id = 228;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF11:
        {
            struct hid_report_ref refer;
            refer.report_id = 229;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF12:
        {
            struct hid_report_ref refer;
            refer.report_id = 230;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF13:
        {
            struct hid_report_ref refer;
            refer.report_id = 231;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break;   

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF14:
        {
            struct hid_report_ref refer;
            refer.report_id = 232;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break; 

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF15:
        {
            struct hid_report_ref refer;
            refer.report_id = 233;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break; 

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF16:
        {
            struct hid_report_ref refer;
            refer.report_id = 234;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break; 

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF17:
        {
            struct hid_report_ref refer;
            refer.report_id = 235;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break; 

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF18:
        {
            struct hid_report_ref refer;
            refer.report_id = 236;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break; 

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF19:
        {
            struct hid_report_ref refer;
            refer.report_id = 237;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
        } break; 

        case HID_IDX_MAX_CNT_FEATURE_REPORT_REP_REF20:
        {
            struct hid_report_ref refer;
            refer.report_id = 238;
            refer.report_type = HID_REPORT_FEATURE;
            gatt_read_cfm(conidx, LE_SUCCESS, handle, HID_REPORT_REF_SIZE, (uint8_t *)&refer);
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
            
//            if (!SADC->CTRL.SADC_DMAC_EN)
//            {
//                ble_latency_applied(true);
//            }
            
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

    voice_start_flag = 0;

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

int pt_hid_kb_report_send(uint16_t keycode)
{
    if (hids_env.nb_pkt == 0)
        return PRF_ERR_REQ_DISALLOWED;

    uint8_t report[RPT_LEN_KB] = {0};
    report[2] = (keycode & 0xFF);
    report[3] = ((keycode >> 8) & 0xFF);

    uint16_t handle = hids_env.start_hdl + HID_IDX_KB_IN_RPT_VAL;
    PT_LOGD("kb report hd=%d keycode=0x%04X\n", handle, keycode);
    // debugHex(report, sizeof(report));

    hids_env.nb_pkt--;
    gatt_ntf_send(app_env.curidx, handle, sizeof(report), report);

    return 0;
}

__attribute__((section("ram_func"))) int pt_hid_voice_report_send(uint8_t index, const uint8_t *buffer, uint16_t len)
{
    if (hids_env.nb_pkt == 0)
        return PRF_ERR_REQ_DISALLOWED;

    uint8_t voice_rpt_hdls[3] = {HID_IDX_MAX_CNT_FEATURE_REPORT_VAL2, HID_IDX_MAX_CNT_FEATURE_REPORT_VAL3, HID_IDX_MAX_CNT_FEATURE_REPORT_VAL4};

    hids_env.nb_pkt--;
    gatt_ntf_send(app_env.curidx, hids_env.start_hdl + voice_rpt_hdls[index], len, buffer);
    return 0;
}

int pt_hid_voice_request(uint8_t start)
{
    if (hids_env.nb_pkt == 0)
        return PRF_ERR_REQ_DISALLOWED;

    uint8_t temp[20] = {0};
    temp[0] = start ? 0x01 : 0x00;

    uint16_t handle = hids_env.start_hdl + HID_IDX_MAX_CNT_FEATURE_REPORT_VAL1;
    PT_LOGD("voice request hd=%d %d\n", handle, start);

    hids_env.nb_pkt--;
    gatt_ntf_send(app_env.curidx, handle, sizeof(temp), temp);
    return 0;
}

#endif //(PRF_HIDS)
