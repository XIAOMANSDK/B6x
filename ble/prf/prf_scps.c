/**
 ****************************************************************************************
 *
 * @file prf_scps.c
 *
 * @brief Scan Parameters Profile Server Role.
 *
 ****************************************************************************************
 */

#if (PRF_SCPS)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "prf.h"
#include "prf_scps.h"

#if (DBG_SCPS)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<SCPS>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINITIONS
 ****************************************************************************************
 */

/// Scan Interval Window Value length @see struct scp_scan_intv_wd
#define SCP_SCAN_INTV_WD_LEN             (0x04)

/// Macro for Client Config value operation
#define SCP_NTF_CFG_GET(conidx)  \
    ((scps_env.ntf_bits >> (conidx)) & (PRF_CLI_START_NTF))

#define SCP_NTF_CFG_CLR(conidx)  \
    scps_env.ntf_bits &= ~((PRF_CLI_START_NTF) << (conidx))

#define SCP_NTF_CFG_SET(conidx, conf)  \
    scps_env.ntf_bits = (scps_env.ntf_bits & ~((PRF_CLI_START_NTF) << (conidx))) | ((conf) << (conidx))

/// Requires scan refresh
enum spcs_refresh
{
    SCPS_REQUIRES_REFRESH    = 0x00,
};


/**
 ****************************************************************************************
 * @section ENVIRONMENT DEFINITION
 ****************************************************************************************
 */

/// SCP Server Environment Variable
typedef struct scps_env_tag
{
    // Service Start Handle
    uint16_t start_hdl;
    // Database configuration @see scp_features
    uint8_t features;
    // Notification config of peer devices, each 1-bit.
    uint8_t ntf_bits;
} scps_env_t;

/// Global Variable Declarations
scps_env_t scps_env;


/**
 ****************************************************************************************
 * @section ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// Attributes configure @see enum scp_att_index
#define SCP_CFG_ATT_MANDATORY_MASK       0x07
#define SCP_CFG_ATT_SCAN_REFRESH_MASK    0x38

/// Attributes Indexes
enum scp_att_index
{
    // Service Declaration, *MUST* Start at 0
    SCP_IDX_SVC,
    
    // Scan Interval Window Char.
    SCP_IDX_SCAN_INTV_WD_CHAR,
    SCP_IDX_SCAN_INTV_WD_VAL,

    // Scan Refresh Char.
    SCP_IDX_SCAN_REFRESH_CHAR,
    SCP_IDX_SCAN_REFRESH_VAL,
    SCP_IDX_SCAN_REFRESH_NTF_CFG,

    // Max Index, *NOTE* Minus 1(Svc Decl) is .nb_att
    SCP_IDX_NB,
};


/// Full SCP Description
const att_decl_t scp_atts[] =
{
    // Scan Interval Window Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( SCP_IDX_SCAN_INTV_WD_CHAR ),
    // Scan Interval Window Characteristic Value
    ATT_ELMT( SCP_IDX_SCAN_INTV_WD_VAL,  ATT_CHAR_SCAN_INTV_WD,  PROP_WC,   SCP_SCAN_INTV_WD_LEN ),

    // Scan Refresh Char. Declaration and Value and CCC Descriptor
    ATT_ELMT_DECL_CHAR( SCP_IDX_SCAN_REFRESH_CHAR ),
    ATT_ELMT( SCP_IDX_SCAN_REFRESH_VAL,  ATT_CHAR_SCAN_REFRESH,  PROP_NTF,  0 ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( SCP_IDX_SCAN_REFRESH_NTF_CFG ),
};

const struct svc_decl scp_svc_db = 
{
    .uuid   = ATT_SVC_SCAN_PARAMETERS, 
    .info   = SVC_UUID(16),
    .atts   = scp_atts,
    .nb_att = SCP_IDX_NB - 1,
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

/// Retrieve attribute handle from idx @see scp_att_index
#define scps_get_att_handle(idx)         (scps_env.start_hdl + (idx))

/// Retrieve attribute index form handle
#define scps_get_att_idx(handle)         (handle - scps_env.start_hdl)

/// Handles reception of the atts request from peer device
static void scps_svc_func(uint8_t conidx, uint8_t opcode, uint16_t handle, const void *param)
{
    uint8_t att_idx = scps_get_att_idx(handle);

    DEBUG("svc_func(cid:%d,op:0x%x,hdl:0x%x,att:%d)", conidx, opcode, handle, att_idx);

    switch (opcode)
    {
        case ATTS_READ_REQ:
        {
            DEBUG("read_req(cid:%d,hdl:0x%x)", conidx, param->handle);

            // Scan Refresh Notification Configuration
            if (att_idx == SCP_IDX_SCAN_REFRESH_NTF_CFG)
            {
                if((scps_env.features & SCP_SCAN_REFRESH_SUP) != 0)
                {
                    uint16_t cli_cfg = SCP_NTF_CFG_GET(conidx);;

                    DEBUG("  read_cfm(txd_ntf:%d)", cli_cfg);
                    gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&cli_cfg);
                    break;
                }
            }

            // Send error response
            gatt_read_cfm(conidx, PRF_ERR_APP_ERROR, handle, 0, NULL);
        } break;

        case ATTS_WRITE_REQ:
        {
            const struct atts_write_ind *ind = param;
            
            DEBUG("  write_req(hdl:0x%x,att:%d,wr:0x%x,len:%d)", handle, att_idx, ind->wrcode, ind->length);

            // Scan Interval Window Value
            if (att_idx == SCP_IDX_SCAN_INTV_WD_VAL)
            {
                uint16_t scan_intv = read16(&ind->value[0]); // value align=2
                uint16_t scan_window = read16(&ind->value[2]);

                // Check interval and window validity
                if ((scan_window <= scan_intv)
                    && (scan_window <= SCP_SCAN_WINDOW_MAX) && (scan_window >= SCP_SCAN_WINDOW_MIN)
                    && (scan_intv <= SCP_SCAN_INTERVAL_MAX) && (scan_intv >= SCP_SCAN_INTERVAL_MIN))
                {
                    // Send write conform first!
                    gatt_write_cfm(conidx, LE_SUCCESS, handle);
                    
                    scps_cb_scan_param(conidx, scan_intv, scan_window);
                    break;
                }
            }
            // Scan Refresh Notification Configuration
            else if (att_idx == SCP_IDX_SCAN_REFRESH_NTF_CFG)
            {
                // Check if Scan Refresh Characteristic is supported
                if ((scps_env.features & SCP_SCAN_REFRESH_SUP) != 0)
                {
                    uint8_t ntf_cfg = ind->value[0]; // 2bytes, only LSB used

                    // Only update configuration if value for stop or notification enable
                    if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF))
                    {
                        DEBUG("    NTF_CFG(cid:%d,cfg:%d)", conidx, ntf_cfg);

                        SCP_NTF_CFG_SET(conidx, ntf_cfg);
                        // Send write conform quickly!
                        gatt_write_cfm(conidx, LE_SUCCESS, handle);
                        break;
                    }
                }
            }

            // Send write conform with error!
            gatt_write_cfm(conidx, PRF_ERR_APP_ERROR, handle);
        } break;

        case ATTS_INFO_REQ:
        {
            uint8_t  status = LE_SUCCESS;
            uint16_t length = 0;
            
            if (att_idx == SCP_IDX_SCAN_REFRESH_NTF_CFG)
            {
                length = sizeof(uint16_t); // CCC attribute
            }
            else
            {
                status = ATT_ERR_WRITE_NOT_PERMITTED;
            }

            // Send length-info confirm for prepWR judging.
            DEBUG("  info_cfm(hdl:0x%x,att:%d,sta:0x%X,len:%d)", handle, att_idx, status, length);
            gatt_info_cfm(conidx, status, handle, length);
        } break;

        case ATTS_CMP_EVT:
        {
            const struct atts_cmp_evt *evt = param;

            DEBUG("  cmp_evt(op:0x%x,sta:0x%x)", evt->operation, evt->status);
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
 * @brief Add Scan Parameters Profile in the DB
 *        Customize via pre-define @see SCP_START_HDL @see SCP_FEATURES
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t scps_svc_init(void)
{
    uint8_t status = LE_SUCCESS;
    uint8_t cfg_att = SCP_CFG_ATT_MANDATORY_MASK;
    // Init Environment
    scps_env.start_hdl = SCP_START_HDL;
    scps_env.features  = SCP_FEATURES;
    scps_env.ntf_bits  = 0;

    // Compute Attributes supported 
    if (scps_env.features & SCP_SCAN_REFRESH_SUP)
    {
        cfg_att |= SCP_CFG_ATT_SCAN_REFRESH_MASK;
    }

    // Create Service in database
    status = attmdb_svc_create(&scps_env.start_hdl, (uint8_t *)&cfg_att, &scp_svc_db, scps_svc_func);
    
    DEBUG("svc_init(sta:0x%X,shdl:%d,feat:0x%X,cfg:0x%X)", 
            status, scps_env.start_hdl, scps_env.features, cfg_att);

    return status;
}

/**
 ****************************************************************************************
 * @brief Enable SCP Notification Configurations
 *
 * @param[in] conidx     Connection index
 * @param[in] ntf_cfg    Scan Refresh Notification Config @see prf_cli_conf
 *
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
void scps_set_ccc(uint8_t conidx, uint8_t ntf_cfg)
{
    //if (gapc_get_conhdl(conidx) != GAP_INVALID_CONHDL)
    {
        SCP_NTF_CFG_SET(conidx, ntf_cfg);
    }
}

/**
 ****************************************************************************************
 * @brief Send a scan refresh to peer device via NTF
 *
 * @param[in] conidx     Connection index
 *
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
uint8_t scps_scan_refresh(uint8_t conidx)
{
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    // check if Notification supported
    if (scps_env.features & SCP_SCAN_REFRESH_SUP)
    {
        if (SCP_NTF_CFG_GET(conidx) != PRF_CLI_STOP_NTFIND)
        {
            // send notification to peer device
            uint8_t req = SCPS_REQUIRES_REFRESH;

            DEBUG("Refresh Send");
            gatt_ntf_send(conidx, scps_get_att_handle(SCP_IDX_SCAN_REFRESH_VAL), sizeof(uint8_t), &req);
            status = LE_SUCCESS;
        }
        else 
        {
            status = PRF_ERR_NTF_DISABLED;
        }
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Callback on received scan params from peer device via WC (__weak func)
 *
 * @param[in] conidx     Connection index
 * @param[in] scan_intv  Scan interval value
 * @param[in] scan_wd    Scan window value
 *
 ****************************************************************************************
 */
__weak void scps_cb_scan_param(uint8_t conidx, uint16_t scan_intv, uint16_t scan_wd)
{
    DEBUG("SCAN_PARAM(cid:%d,intv:%d,winw:%d)", conidx, scan_intv, scan_wd);
}


#endif //PRF_SCPS
