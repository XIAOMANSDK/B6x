/**
 ****************************************************************************************
 *
 * @file prf_bass.c
 *
 * @brief Battery Service - Server Role Implementation.
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#if (PRF_BASS)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "prf.h"
#include "prf_bass.h"

#if (DBG_BASS)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat,len)
#endif


/*
 * DEFINITIONS
 ****************************************************************************************
 */

/// Value of Battery Level and Power State
#define BAT_LVL_MIN                 (0)
#define BAT_LVL_MAX                 (100)
#define BAT_LVL_DFT                 (88)   // 88%
#define PWR_STA_DFT                 (0xBB)

/// Macro for Client Config value operation
#define BAS_LVL_NTF_GET(conidx)  \
    ((bass_env.lvl_ntfs >> (conidx)) & PRF_CLI_START_NTF)

#define BAS_LVL_NTF_CLR(conidx)  \
    bass_env.lvl_ntfs &= ~(PRF_CLI_START_NTF << (conidx))

#define BAS_LVL_NTF_SET(conidx, conf)  \
    bass_env.lvl_ntfs = (bass_env.lvl_ntfs & ~(PRF_CLI_START_NTF << (conidx))) | ((conf) << (conidx))

#if (BAS_PWR_STA)
#define BAS_PWR_NTF_GET(conidx)  \
    ((bass_env.pwr_ntfs >> (conidx)) & PRF_CLI_START_NTF)

#define BAS_PWR_NTF_CLR(conidx)  \
    bass_env.pwr_ntfs &= ~(PRF_CLI_START_NTF << (conidx))

#define BAS_PWR_NTF_SET(conidx, conf)  \
    bass_env.pwr_ntfs = (bass_env.pwr_ntfs & ~(PRF_CLI_START_NTF << (conidx))) | ((conf) << (conidx))
#endif

/// Bits of Battery Power State
struct bat_pwr_sta_def
{
    uint8_t level       : 2; // bit[0:1] (0-Unknown, 1-Not Supported,  2-No, 3-Yes)
    uint8_t charging    : 2; // bit[2:3] (0-Unknown, 1-Not Chargeable, 2-No, 3-Yes)
    uint8_t discharging : 2; // bit[4:5] (0-Unknown, 1-Not Supported,  2-No, 3-Yes)
    uint8_t present     : 2; // bit[6:7] (0-Unknown, 1-Not Supported,  2-No, 3-Yes)
};


/**
 ****************************************************************************************
 * @section ENVIRONMENT DEFINITION
 ****************************************************************************************
 */

/// BAS Server Environment Variable
typedef struct bass_env_tag
{
    // Service Start Handle
    uint16_t start_hdl;

    // Current Battery Level(0~100), unit in '%'
    uint8_t  bat_lvl;
    // Client Config Bits of Battery Level - each 1Bit(NTF only), so max_peer=8.
    uint8_t  lvl_ntfs;

    #if (BAS_PWR_STA)
    // Current Power State, @see struct bat_pwr_sta_def
    uint8_t  pwr_sta;
    // Client Config Bits of Power State - each 1Bit(NTF only), so max_peer=8.
    uint8_t  pwr_ntfs;
    #endif
} bass_env_t;

/// Global Variable Declarations
__VAR_ENV bass_env_t bass_env;


/**
 ****************************************************************************************
 * @section ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// BAS Attributes Index
enum bas_att_idx
{
    // Service Declaration, *MUST* Start at 0
    BAS_IDX_SVC,

    // Battery Level Char.
    BAS_IDX_BAT_LVL_CHAR,
    BAS_IDX_BAT_LVL_VAL,
    BAS_IDX_BAT_LVL_NTF_CFG,

    #if (BAS_PWR_STA)
    // Battery Power State Char.
    BAS_IDX_PWR_STA_CHAR,
    BAS_IDX_PWR_STA_VAL,
    BAS_IDX_PWR_STA_NTF_CFG,
    #endif //(BAS_PWR_STA)

    // Max Index, *NOTE* Minus 1(Svc Decl) is .nb_att
    BAS_IDX_NB,
};

/// Attributes Description
const att_decl_t bas_atts[] =
{
    // Battery Level Char. Declaration and Value and CCC Descriptor
    ATT_ELMT_DECL_CHAR( BAS_IDX_BAT_LVL_CHAR ),
    ATT_ELMT( BAS_IDX_BAT_LVL_VAL,  ATT_CHAR_BATTERY_LEVEL,       PROP_NTF | PROP_RD, 0),
    ATT_ELMT_DESC_CLI_CHAR_CFG( BAS_IDX_BAT_LVL_NTF_CFG ),

    #if (BAS_PWR_STA)
    // Battery Power State Char. Declaration and Value and CCC Descriptor
    ATT_ELMT_DECL_CHAR( BAS_IDX_PWR_STA_CHAR ),
    ATT_ELMT(BAS_IDX_PWR_STA_VAL,   ATT_CHAR_BATTERY_POWER_STATE, PROP_NTF | PROP_RD, 0),
    ATT_ELMT_DESC_CLI_CHAR_CFG( BAS_IDX_PWR_STA_NTF_CFG ),
    #endif
};

/// Service Description
const struct svc_decl bas_svc_db =
{
    .uuid   = ATT_SVC_BATTERY_SERVICE,
    .info   = SVC_UUID(16),
    .atts   = bas_atts,
    .nb_att = BAS_IDX_NB - 1,
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

/// Retrieve attribute handle from index (@see bass_att_idx)
static uint16_t bass_get_att_handle(uint8_t att_idx)
{
    ASSERT_ERR(att_idx < BAS_IDX_NB);

    return (att_idx + bass_env.start_hdl);
}

/// Retrieve attribute index form handle
static uint8_t bass_get_att_idx(uint16_t handle)
{
    ASSERT_ERR((handle >= bass_env.start_hdl) && (handle < bass_env.start_hdl + BAS_IDX_NB));

    return (handle - bass_env.start_hdl);
}

/// Handles reception of the atts request from peer device
static void bass_svc_func(uint8_t conidx, uint8_t opcode, uint16_t handle, const void *param)
{
    uint8_t att_idx = bass_get_att_idx(handle);

    DEBUG("svc_func(cid:%d,op:0x%x,hdl:0x%x,att:%d)", conidx, opcode, handle, att_idx);

    switch (opcode)
    {
        case ATTS_READ_REQ:
        {
            if (att_idx == BAS_IDX_BAT_LVL_VAL)
            {
                DEBUG("  read_cfm(bat_lvl:%d)", bass_env.bat_lvl);
                gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint8_t), &(bass_env.bat_lvl));
                break;
            }

            if (att_idx == BAS_IDX_BAT_LVL_NTF_CFG)
            {
                // retrieve notification config
                uint16_t cli_cfg = BAS_LVL_NTF_GET(conidx);

                DEBUG("  read_cfm(lvl_ntf:%d)", cli_cfg);
                gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&cli_cfg);
                break;
            }

            #if (BAS_PWR_STA)
            if (att_idx == BAS_IDX_PWR_STA_VAL)
            {
                DEBUG("  read_cfm(pwr_sta:%d)", bass_env.pwr_sta);
                gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint8_t), &(bass_env.pwr_sta));
                break;
            }

            if (att_idx == BAS_IDX_PWR_STA_NTF_CFG)
            {
                // retrieve notification config
                uint16_t cli_cfg = BAS_PWR_NTF_GET(conidx);

                DEBUG("  read_cfm(pwr_ntf:%d)", cli_cfg);
                gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&cli_cfg);
                break;
            }
            #endif //(BAS_PWR_STA)

            // Send error response
            gatt_read_cfm(conidx, PRF_ERR_APP_ERROR, handle, 0, NULL);
        } break;

        case ATTS_WRITE_REQ:
        {
            const struct atts_write_ind *ind = param;

            DEBUG("  write_req(hdl:0x%x,att:%d,wr:0x%x,len:%d)", handle, att_idx, ind->wrcode, ind->length);

            #if (BAS_PWR_STA)
            if ((att_idx == BAS_IDX_BAT_LVL_NTF_CFG) || (att_idx == BAS_IDX_PWR_STA_NTF_CFG))
            #else
            if (att_idx == BAS_IDX_BAT_LVL_NTF_CFG)
            #endif //(BAS_PWR_STA)
            {
                if ((!ind->more) && (ind->length == sizeof(uint16_t)))
                {
                    uint16_t cli_cfg = read16p(ind->value);

                    // update configuration if value for stop or NTF start
                    if (cli_cfg <= PRF_CLI_START_NTF)
                    {
                        // Send write confirm quickly!
                        gatt_write_cfm(conidx, LE_SUCCESS, handle);

                        #if (BAS_PWR_STA)
                        if (att_idx == BAS_IDX_PWR_STA_NTF_CFG)
                        {
                            DEBUG("  set pwr_ntf(cid:%d,cfg:%d)", conidx, cli_cfg);
                            BAS_PWR_NTF_SET(conidx, cli_cfg);

                            // Send Battery Power State Notify
                            if (cli_cfg == PRF_CLI_START_NTF)
                            {
                                gatt_ntf_send(conidx, bass_get_att_handle(BAS_IDX_PWR_STA_VAL), sizeof(uint8_t), &bass_env.pwr_sta);
                            }
                        }
                        else
                        #endif //(BAS_PWR_STA)
                        {
                            DEBUG("  set lvl_ntf(cid:%d,cfg:%d)", conidx, cli_cfg);
                            BAS_LVL_NTF_SET(conidx, cli_cfg);

                            // Send Battery Level Notify
                            if (cli_cfg == PRF_CLI_START_NTF)
                            {
                                gatt_ntf_send(conidx, bass_get_att_handle(BAS_IDX_BAT_LVL_VAL), sizeof(uint8_t), &bass_env.bat_lvl);
                            }
                        }
                        break;
                    }
                }
            }

            // Send write confirm with error!
            gatt_write_cfm(conidx, PRF_ERR_APP_ERROR, handle);
        } break;

        case ATTS_INFO_REQ:
        {
            uint16_t length = ATT_MAX_LEN_GET(att_idx, bas_atts);

            // Send length-info confirm for prepWR judging.
            DEBUG("  info_cfm(hdl:0x%x,att:%d,len:%d)", handle, att_idx, length);
            gatt_info_cfm(conidx, LE_SUCCESS, handle, length);
        } break;

        case ATTS_CMP_EVT:
        {
            const struct atts_cmp_evt *evt = param;

            DEBUG("  cmp_evt(op:0x%x,sta:0x%x)", evt->operation, evt->status);
            // add 'if' to avoid warning #117-D: "evt" never referenced
            if (evt->operation == GATT_NOTIFY)
            {
                // Update operation result
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
 * @brief Add Battery Service Profile in the DB.
 *        Customize via pre-define @see BAS_START_HDL
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t bass_svc_init(void)
{
    uint8_t status = LE_SUCCESS;

    // Init Environment
    bass_env.start_hdl = BAS_START_HDL;
    bass_env.bat_lvl   = BAT_LVL_DFT;
    bass_env.lvl_ntfs  = PRF_CLI_START_NTF;
    #if (BAS_PWR_STA)
    bass_env.pwr_sta   = PWR_STA_DFT;
    bass_env.pwr_ntfs  = PRF_CLI_START_NTF;
    #endif //(BAS_PWR_STA)

    // Create Service in database
    status = attmdb_svc_create(&bass_env.start_hdl, NULL, &bas_svc_db, bass_svc_func);
    DEBUG("svc_init(sta:0x%X,shdl:%d)", status, bass_env.start_hdl);

    return status;
}

/**
 ****************************************************************************************
 * @brief Transmit Battery Level to peer device via NTF
 *
 * @param[in] conidx   peer device connection index
 * @param[in] bat_lvl  Battery Level
 *
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
static uint8_t bass_bat_lvl_send(uint8_t conidx, uint8_t bat_lvl)
{
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if ((bat_lvl <= BAT_LVL_MAX)
        && (BAS_LVL_NTF_GET(conidx) == PRF_CLI_START_NTF))
    {
        status = LE_SUCCESS;
        gatt_ntf_send(conidx, bass_get_att_handle(BAS_IDX_BAT_LVL_VAL), sizeof(uint8_t), &bat_lvl);
    }

    return status;
}

/**
 ****************************************************************************************
 * @brief Update Battery Level value
 *
 * @param[in] bat_lvl    Battery Level
 ****************************************************************************************
 */
void bass_bat_lvl_update(uint8_t bat_lvl)
{
    if (bass_env.bat_lvl != bat_lvl)
    {
        bass_env.bat_lvl = bat_lvl;

        // todo update operation, loop on all connection
        for (uint8_t idx = 0; idx < BLE_CONNECTION_MAX; idx++)
        {
            bass_bat_lvl_send(idx, bass_env.bat_lvl);
        }
    }
}

/**
 ****************************************************************************************
 * @brief Enable Battery level Notification Configurations
 *
 * @param[in] conidx     Connection index
 * @param[in] ntf_cfg    Notification Config @see prf_cli_conf
 ****************************************************************************************
 */
void bass_set_lvl_ntf(uint8_t conidx, uint8_t ntf_cfg)
{
    //if (gapc_get_conhdl(conidx) != GAP_INVALID_CONHDL)
    {
        ntf_cfg &= PRF_CLI_START_NTF; // NTF only(0x0001)

        // update configuration
        BAS_LVL_NTF_SET(conidx, ntf_cfg);
    }
}

#if (BAS_PWR_STA)
/**
 ****************************************************************************************
 * @brief Transmit Battery Power State to peer device via NTF
 *
 * @param[in] conidx   peer device connection index
 * @param[in] pwr_sta  Power State
 *
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
static uint8_t bass_pwr_sta_send(uint8_t conidx, uint8_t pwr_sta)
{
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if (BAS_PWR_NTF_GET(conidx) == PRF_CLI_START_NTF)
    {
        status = LE_SUCCESS;
        gatt_ntf_send(conidx, bass_get_att_handle(BAS_IDX_PWR_STA_VAL), sizeof(uint8_t), &pwr_sta);
    }

    return status;
}

/**
 ****************************************************************************************
 * @brief Update Battery Power State value
 *
 * @param[in] pwr_sta    Power State
 ****************************************************************************************
 */
void bass_pwr_sta_update(uint8_t pwr_sta)
{
    if (bass_env.pwr_sta != pwr_sta)
    {
        bass_env.pwr_sta = pwr_sta;

        // todo update operation, loop on all connection
        for (uint8_t idx = 0; idx < BLE_CONNECTION_MAX; idx++)
        {
            bass_pwr_sta_send(idx, bass_env.pwr_sta);
        }
    }
}

/**
 ****************************************************************************************
 * @brief Enable Battery Power State Notification Configurations
 *
 * @param[in] conidx     Connection index
 * @param[in] ntf_cfg    Notification Config @see prf_cli_conf
 ****************************************************************************************
 */
void bass_set_pwr_ntf(uint8_t conidx, uint8_t ntf_cfg)
{
    //if (gapc_get_conhdl(conidx) != GAP_INVALID_CONHDL)
    {
        ntf_cfg &= PRF_CLI_START_NTF; // NTF only(0x0001)

        // update configuration
        BAS_PWR_NTF_SET(conidx, ntf_cfg);
    }
}
#endif //(BAS_PWR_STA)


#endif //(PRF_BASS)
