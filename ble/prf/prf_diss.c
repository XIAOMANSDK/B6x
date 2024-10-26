/**
 ****************************************************************************************
 *
 * @file prf_diss.c
 *
 * @brief Device Information Service(DIS) - Server Role Implementation.
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#if (PRF_DISS)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "prf.h"
#include "prf_diss.h"

#if (DBG_DISS)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINITIONS
 ****************************************************************************************
 */

/// Manufacturer Name Value
#define INFO_MANUF_NAME         ("BLE5x")
#define INFO_MANUF_NAME_LEN     (sizeof(INFO_MANUF_NAME)-1)

/// Model Number String Value
#define INFO_MODEL_NB_STR       ("BLE5.2")
#define INFO_MODEL_NB_STR_LEN   (sizeof(INFO_MODEL_NB_STR)-1)

/// Serial Number
#define INFO_SERIAL_NB_STR      ("11.0.2-LE")
#define INFO_SERIAL_NB_STR_LEN  (sizeof(INFO_SERIAL_NB_STR)-1)

/// Firmware Revision
#define INFO_FW_REV_STR         ("11.0.2")
#define INFO_FW_REV_STR_LEN     (sizeof(INFO_FW_REV_STR)-1)

/// System ID Value - LSB -> MSB, Length=8(DIS_SYS_ID_LEN)
#define INFO_SYS_ID             ("\x12\x34\x56\xFF\xFE\x9A\xBC\xDE")
#define INFO_SYS_ID_LEN         (sizeof(INFO_SYS_ID)-1)

/// Hardware Revision String
#define INFO_HW_REV_STR         ("1.0.0")
#define INFO_HW_REV_STR_LEN     (sizeof(INFO_HW_REV_STR)-1)

/// Software Revision String
#define INFO_SW_REV_STR         ("1.0.0")
#define INFO_SW_REV_STR_LEN     (sizeof(INFO_SW_REV_STR)-1)

/// IEEE Certif, Length>=6(DIS_IEEE_CERTIF_MIN_LEN)
#define INFO_IEEE               ("\xFF\xEE\xDD\xCC\xBB\xAA")
#define INFO_IEEE_LEN           (sizeof(INFO_IEEE)-1)

/**
 * PNP ID Value - LSB -> MSB, Length=7(DIS_PNP_ID_LEN)
 *      Vendor ID Source : 0x02 (USB Implementer¡¯s Forum assigned Vendor ID value)
 *      Vendor ID : 0x045E      (Microsoft Corp)
 *      Product ID : 0x0040
 *      Product Version : 0x0300
 */
#ifndef INFO_PNP_ID
#define INFO_PNP_ID             ("\x02\x5E\x04\x40\x00\x00\x03")
#endif
#define INFO_PNP_ID_LEN         (sizeof(INFO_PNP_ID)-1)


/**
 ****************************************************************************************
 * @section ENVIRONMENT DEFINITION
 ****************************************************************************************
 */

/// DIS Server Environment Variable
typedef struct diss_env_tag
{
    // Service Start Handle
    uint16_t start_hdl;
    // Supported features @see enum dis_features
    uint16_t features;
} diss_env_t;

/// Global Variable Declarations
__VAR_ENV diss_env_t diss_env;


/**
 ****************************************************************************************
 * @section ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// DIS Attributes Index, keep up with @see enum dis_values
enum dis_att_idx
{
    // Service Declaration, *MUST* Start at 0
    DIS_IDX_SVC,

    // Manufacturer Name String Char.
    DIS_IDX_MANUF_NAME_CHAR,
    DIS_IDX_MANUF_NAME_VAL,
    // Model Number String Char.
    DIS_IDX_MODEL_NB_STR_CHAR,
    DIS_IDX_MODEL_NB_STR_VAL,
    // Serial Number String Char.
    DIS_IDX_SERIAL_NB_STR_CHAR,
    DIS_IDX_SERIAL_NB_STR_VAL,
    // Hardware Revision String Char.
    DIS_IDX_HW_REV_STR_CHAR,
    DIS_IDX_HW_REV_STR_VAL,
    // Firmware Revision String Char.
    DIS_IDX_FW_REV_STR_CHAR,
    DIS_IDX_FW_REV_STR_VAL,
    // Software Revision String Char.
    DIS_IDX_SW_REV_STR_CHAR,
    DIS_IDX_SW_REV_STR_VAL,
    // System Identifier Char.
    DIS_IDX_SYS_ID_CHAR,
    DIS_IDX_SYS_ID_VAL,
    // IEEE Certificate Char.
    DIS_IDX_IEEE_CHAR,
    DIS_IDX_IEEE_VAL,
    // PnP ID Char.
    DIS_IDX_PNP_ID_CHAR,
    DIS_IDX_PNP_ID_VAL,

    // Max Index, *NOTE* Minus 1(Svc Decl) is .nb_att
    DIS_IDX_NB,
};

/// Attributes Description
const att_decl_t dis_atts[] =
{
    // Manufacturer Name Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( DIS_IDX_MANUF_NAME_CHAR ),
    ATT_ELMT( DIS_IDX_MANUF_NAME_VAL,    ATT_CHAR_MANUF_NAME,  PROP_RD, DIS_VAL_MAX_LEN ),

    // Model Number String Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( DIS_IDX_MODEL_NB_STR_CHAR ),
    ATT_ELMT( DIS_IDX_MODEL_NB_STR_VAL,  ATT_CHAR_MODEL_NB,    PROP_RD, DIS_VAL_MAX_LEN ),

    // Serial Number String Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( DIS_IDX_SERIAL_NB_STR_CHAR ),
    ATT_ELMT( DIS_IDX_SERIAL_NB_STR_VAL, ATT_CHAR_SERIAL_NB,   PROP_RD, DIS_VAL_MAX_LEN ),

    // Hardware Revision String Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( DIS_IDX_HW_REV_STR_CHAR ),
    ATT_ELMT( DIS_IDX_HW_REV_STR_VAL,    ATT_CHAR_HW_REV,      PROP_RD, DIS_VAL_MAX_LEN ),

    // Firmware Revision String Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( DIS_IDX_FW_REV_STR_CHAR ),
    ATT_ELMT( DIS_IDX_FW_REV_STR_VAL,    ATT_CHAR_FW_REV,      PROP_RD, DIS_VAL_MAX_LEN ),

    // Software Revision String Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( DIS_IDX_SW_REV_STR_CHAR ),
    ATT_ELMT( DIS_IDX_SW_REV_STR_VAL,    ATT_CHAR_SW_REV,      PROP_RD, DIS_VAL_MAX_LEN ),

    // System ID Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( DIS_IDX_SYS_ID_CHAR ),
    ATT_ELMT( DIS_IDX_SYS_ID_VAL,        ATT_CHAR_SYS_ID,      PROP_RD, DIS_SYS_ID_LEN ),

    // IEEE 11073-20601 Regulatory Certification Data List Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( DIS_IDX_IEEE_CHAR ),
    ATT_ELMT( DIS_IDX_IEEE_VAL,          ATT_CHAR_IEEE_CERTIF, PROP_RD, DIS_SYS_ID_LEN ),

    // PnP ID Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( DIS_IDX_PNP_ID_CHAR ),
    ATT_ELMT( DIS_IDX_PNP_ID_VAL,        ATT_CHAR_PNP_ID,      PROP_RD, DIS_PNP_ID_LEN ),
};

/// Service Description
const struct svc_decl dis_svc_db =
{
    .uuid   = ATT_SVC_DEVICE_INFO,
    .info   = SVC_UUID(16),
    .atts   = dis_atts,
    .nb_att = DIS_IDX_NB - 1,
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

/// Retrieve value index from attribute handle
static uint8_t diss_get_val_idx(uint16_t handle)
{
    // handle start from first characteristic value handle
    uint16_t cur_hdl = diss_env.start_hdl + 2;

    for (uint8_t idx = 0; idx < DIS_CHAR_MAX; idx++)
    {
        if (((diss_env.features >> idx) & 1) == 1)
        {
            // check if value handle correspond to requested handle
            if (cur_hdl == handle)
            {
                return idx;
            }
            cur_hdl += 2;
        }
    }

    return DIS_CHAR_MAX;
}

/// Handles reception of the read request from peer device
static void diss_svc_func(uint8_t conidx, uint8_t opcode, uint16_t handle, const void *param)
{
    uint8_t val_idx = diss_get_val_idx(handle);

    DEBUG("svc_func(cid:%d,op:0x%x,hdl:0x%x,att:%d)", conidx, opcode, handle, val_idx);

    if (opcode == ATTS_READ_REQ)
    {
        // Check Characteristic Code
        if (val_idx < DIS_CHAR_MAX)
        {
            uint16_t length;
            const uint8_t *value = diss_value_get(val_idx, &length);

            if (value != NULL)
            {
                // Send value to peer device.
                gatt_read_cfm(conidx, LE_SUCCESS, handle, length, value);
                return;
            }
        }

        // application error, value cannot be retrieved
        gatt_read_cfm(conidx, PRF_ERR_APP_ERROR, handle, 0, NULL);
    }
}


/**
 ****************************************************************************************
 * @section API FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Add Device Information Profile in the DB.
 *        Customize via pre-define @see DIS_START_HDL @see DIS_FEATURES
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t diss_svc_init(void)
{
    uint8_t status = LE_SUCCESS;
    uint32_t cfg_att = 0;

    // Init Environment
    diss_env.start_hdl = DIS_START_HDL;
    diss_env.features  = DIS_FEATURES;

    // Compute Attributes supported
    for (uint8_t i = 0; i < DIS_CHAR_MAX; i++)
    {
        if (((diss_env.features >> i) & 1) == 1)
        {
            cfg_att |= (3 << (i*2 + 1)); // 3:CHAR + VAL
        }
    }

    // Create Service in database
    status = attmdb_svc_create(&diss_env.start_hdl, (uint8_t *)&cfg_att, &dis_svc_db, diss_svc_func);

    DEBUG("svc_init(sta:0x%X,shdl:%d,feat:0x%X,cfg:0x%X)",
            status, diss_env.start_hdl, diss_env.features, cfg_att);

    return status;
}

/**
 ****************************************************************************************
 * @brief Get value for attribute read (__weak func)
 *
 * @param[in] val_idx  Index of Value to set @see enum dis_values
 * @param[out] p_len   Value Length
 *
 * @return Value data pointer
 ****************************************************************************************
 */
__weak const uint8_t *diss_value_get(uint8_t val_idx, uint16_t *p_len)
{
    uint16_t length = 0;
    const uint8_t *p_data = NULL;

    // Retrieve value information
    switch (val_idx)
    {
        #if DIS_FEAT_SUP(MANUF_NAME)
        case DIS_MANUF_NAME_CHAR:
        {
            length = INFO_MANUF_NAME_LEN;
            p_data = (const uint8_t *)INFO_MANUF_NAME;
        } break;
        #endif

        #if DIS_FEAT_SUP(MODEL_NB_STR)
        case DIS_MODEL_NB_STR_CHAR:
        {
            length = INFO_MODEL_NB_STR_LEN;
            p_data = (const uint8_t *)INFO_MODEL_NB_STR;
        } break;
        #endif

        #if DIS_FEAT_SUP(SERIAL_NB_STR)
        case DIS_SERIAL_NB_STR_CHAR:
        {
            length = INFO_SERIAL_NB_STR_LEN;
            p_data = (const uint8_t *)INFO_SERIAL_NB_STR;
        } break;
        #endif

        #if DIS_FEAT_SUP(HW_REV_STR)
        case DIS_HW_REV_STR_CHAR:
        {
            length = INFO_HW_REV_STR_LEN;
            p_data = (const uint8_t *)INFO_HW_REV_STR;
        } break;
        #endif

        #if DIS_FEAT_SUP(FW_REV_STR)
        case DIS_FW_REV_STR_CHAR:
        {
            length = INFO_FW_REV_STR_LEN;
            p_data = (const uint8_t *)INFO_FW_REV_STR;
        } break;
        #endif

        #if DIS_FEAT_SUP(SW_REV_STR)
        case DIS_SW_REV_STR_CHAR:
        {
            length = INFO_SW_REV_STR_LEN;
            p_data = (const uint8_t *)INFO_SW_REV_STR;
        } break;
        #endif

        #if DIS_FEAT_SUP(SYS_ID)
        case DIS_SYS_ID_CHAR:
        {
            // Check if length matches particular requirements
            if (INFO_SYS_ID_LEN == DIS_SYS_ID_LEN)
            {
                length = INFO_SYS_ID_LEN;
                p_data = (const uint8_t *)INFO_SYS_ID;
            }
        } break;
        #endif

        #if DIS_FEAT_SUP(IEEE)
        case DIS_IEEE_CHAR:
        {
            // Check if length matches particular requirements
            if (INFO_IEEE_LEN >= DIS_IEEE_CERTIF_MIN_LEN)
            {
                length = INFO_IEEE_LEN;
                p_data = (const uint8_t *)INFO_IEEE;
            }
        } break;
        #endif

        #if DIS_FEAT_SUP(PNP_ID)
        case DIS_PNP_ID_CHAR:
        {
            // Check if length matches particular requirements
            if (INFO_PNP_ID_LEN == DIS_PNP_ID_LEN)
            {
                length = INFO_PNP_ID_LEN;
                p_data = (const uint8_t *)INFO_PNP_ID;
            }
        } break;
        #endif

        default:
            break;
    }

    // Check value length not exceed MAX_LEN
    if (length > DIS_VAL_MAX_LEN)
    {
        length = DIS_VAL_MAX_LEN;
    }
    DEBUG("value_get(req:%d,len:%d)", val_idx, length);

    *p_len = length;
    return p_data;
}

#endif //(PRF_DISS)
