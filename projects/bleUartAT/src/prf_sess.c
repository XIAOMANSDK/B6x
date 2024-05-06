/**
 ****************************************************************************************
 *
 * @file prf_sess.c
 *
 * @brief Serial Service - Server Role Implementation.
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#if (PRF_SESS)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "prf.h"
#include "prf_sess.h"
#include "atcmd.h"

#if (DBG_SESS)
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

/// Max length of received once
#if !defined(SES_RXD_MAX_LEN)
    #define SES_RXD_MAX_LEN         (0x200)
#endif

/// Max number of notify/indicate pkt
#if !defined(SES_NB_PKT_MAX)
    #define SES_NB_PKT_MAX          (5)
#endif

/// Version String for SES_IDX_READ_VAL Read
#define SES_VERS_STR                ("Ver:1.25")
#define SES_VERS_STR_LEN            (sizeof(SES_VERS_STR) - 1)

/// Macro for Client Config value operation
#define SES_NTF_CFG_GET(conidx)  \
    ((sess_env.ntf_bits >> (conidx*2)) & (PRF_CLI_START_NTF | PRF_CLI_START_IND))

#define SES_NTF_CFG_CLR(conidx)  \
    sess_env.ntf_bits &= ~((PRF_CLI_START_NTF | PRF_CLI_START_IND) << (conidx*2))

#define SES_NTF_CFG_SET(conidx, conf)  \
    sess_env.ntf_bits = (sess_env.ntf_bits & ~((PRF_CLI_START_NTF | PRF_CLI_START_IND) << (conidx*2))) | ((conf) << (conidx*2))


/**
 ****************************************************************************************
 * @section ENVIRONMENT DEFINITION
 ****************************************************************************************
 */

/// Server Environment Variable
typedef struct sess_env_tag
{
    // Service Start Handle
    uint16_t  start_hdl;
    // Client Config of peer devices - each 2Bits(NTF & IND), so max_peer=8.
    uint16_t  ntf_bits;
    // Number of notify pkt
    uint8_t   nb_pkt;
} sess_env_t;

/// Global Variable Declarations
sess_env_t sess_env;


/**
 ****************************************************************************************
 * @section ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// Attributes Index
enum ses_att_idx
{
    // Service Declaration, *MUST* Start at 0
    SES_IDX_SVC,
    
    // Serial TXD Char.
    SES_IDX_TXD_CHAR,
    SES_IDX_TXD_VAL,
    SES_IDX_TXD_NTF_CFG,

    // Serial RXD Char.
    SES_IDX_RXD_CHAR,
    SES_IDX_RXD_VAL, //5

    #if (SES_READ_SUP)
    // Serial READ Char.
    SES_IDX_READ_CHAR,
    SES_IDX_READ_VAL,
    #endif

    // Max Index, *NOTE* Minus 1(Svc Decl) is .nb_att
    SES_IDX_NB,
};

#if (SES_UUID_128)
/// Characteristic Base UUID128 (User Customize)
#define SES_ATT_UUID128(uuid)     { 0x16, 0x0A, 0x10, 0x40, 0xD1, 0x9F, 0x4C, 0x6C, \
                                    0xB4, 0x55, 0xE3, 0xF7, (uuid) & 0xFF, (uuid >> 8) & 0xFF, 0x00, 0x00 }

/// Serial Service UUID128
const uint8_t ses_svc_uuid[]        = SES_ATT_UUID128(0xFF00);
/// Serial Notify UUID128
const uint8_t ses_char_txd_notify[] = SES_ATT_UUID128(0xFF01);
/// Serial Write Command UUID128
const uint8_t ses_char_rxd_write[]  = SES_ATT_UUID128(0xFF02);
/// Serial Read Command UUID128
const uint8_t ses_char_val_read[]   = SES_ATT_UUID128(0xFF03);

/// Attributes Description
const att_decl_t ses_atts[] = 
{
    // Serial Notify Char. Declaration and Value and Client Char. Configuration Descriptor
    ATT_ELMT_DECL_CHAR( SES_IDX_TXD_CHAR ),
    ATT_ELMT128( SES_IDX_TXD_VAL,  ses_char_txd_notify, PROP_NTF | PROP_IND, 0 ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( SES_IDX_TXD_NTF_CFG ),

    // Serial Write Command Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( SES_IDX_RXD_CHAR ),
    ATT_ELMT128( SES_IDX_RXD_VAL,  ses_char_rxd_write,  PROP_WC | PROP_WR,   SES_RXD_MAX_LEN ),
    
    #if (SES_READ_SUP)
    // Serial Read Command Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( SES_IDX_READ_CHAR ),
    ATT_ELMT128( SES_IDX_READ_VAL, ses_char_val_read,   PROP_RD,             0 ),
    #endif //(SES_READ_SUP)
};

/// Service Description
const struct svc_decl ses_svc_db = 
{
    .uuid128 = ses_svc_uuid, 
    .info    = SVC_UUID(128),
    .atts    = ses_atts,
    .nb_att  = SES_IDX_NB - 1,
};

#else //(SESS_UUID16)

/// Serial Service UUID
#define SES_SVC_UUID                ATT_UUID16(0xFF00)
/// Serial Notify UUID
#define SES_CHAR_TXD_NOTIFY         ATT_UUID16(0xFF01)
/// Serial Write Command UUID
#define SES_CHAR_RXD_WRITE          ATT_UUID16(0xFF02)
/// Serial Read Command UUID
#define SES_CHAR_VAL_READ           ATT_UUID16(0xFF03)

/// Attributes Description
att_decl_t ses_atts[] = 
{
    // Serial Notify Char. Declaration and Value and CCC Descriptor
    ATT_ELMT_DECL_CHAR( SES_IDX_TXD_CHAR ),
    ATT_ELMT( SES_IDX_TXD_VAL,  SES_CHAR_TXD_NOTIFY, PROP_NTF | PROP_IND, 0 ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( SES_IDX_TXD_NTF_CFG ),

    // Serial Write Command Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( SES_IDX_RXD_CHAR ),
    ATT_ELMT( SES_IDX_RXD_VAL,  SES_CHAR_RXD_WRITE,  PROP_WC | PROP_WR,   SES_RXD_MAX_LEN ),
    
    #if (SES_READ_SUP)
    // Serial Read Command Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( SES_IDX_READ_CHAR ),
    ATT_ELMT( SES_IDX_READ_VAL, SES_CHAR_VAL_READ,   PROP_RD,             0 ),
    #endif //(SES_READ_SUP)
};

/// Service Description
struct svc_decl ses_svc_db = 
{
    .uuid   = SES_SVC_UUID, 
    .info   = SVC_UUID(16),
    .atts   = ses_atts,
    .nb_att = SES_IDX_NB - 1,
};
#endif


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @section SVC FUNCTIONS
 ****************************************************************************************
 */

/// Retrieve attribute handle from index (@see ses_att_idx)
static uint16_t sess_get_att_handle(uint8_t att_idx)
{
    ASSERT_ERR(att_idx < SES_IDX_NB);

    return att_idx + sess_env.start_hdl;
}

/// Retrieve attribute index form handle
static uint8_t sess_get_att_idx(uint16_t handle)
{
    ASSERT_ERR((handle >= sess_env.start_hdl) && (handle < sess_env.start_hdl + SES_IDX_NB));

    return handle - sess_env.start_hdl;
}

/// Handles reception of the atts request from peer device
static void sess_svc_func(uint8_t conidx, uint8_t opcode, uint16_t handle, const void *param)
{
    uint8_t att_idx = sess_get_att_idx(handle);

    DEBUG("svc_func(cid:%d,op:0x%x,hdl:0x%x,att:%d)", conidx, opcode, handle, att_idx);

    switch (opcode)
    {
        case ATTS_READ_REQ:
        {
            if (att_idx == SES_IDX_TXD_NTF_CFG)
            {
                // retrieve notification config
                uint16_t cli_cfg = SES_NTF_CFG_GET(conidx);
                
                DEBUG("  read_cfm(txd_ntf:%d)", cli_cfg);
                gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&cli_cfg);
                break;
            }

            #if (SES_READ_SUP)
            if (att_idx == SES_IDX_READ_VAL)
            {
                sess_cb_rdv(conidx, att_idx, handle);
                break;
            }
            #endif //(SES_READ_SUP)

            // Send error response
            gatt_read_cfm(conidx, PRF_ERR_APP_ERROR, handle, 0, NULL);
        } break;
        
        case ATTS_WRITE_REQ:
        {
            const struct atts_write_ind *ind = param;
            
            DEBUG("  write_req(hdl:0x%x,att:%d,wr:0x%x,len:%d)", handle, att_idx, ind->wrcode, ind->length);

            if (att_idx == SES_IDX_RXD_VAL)
            {
                // Send write conform first!
                if (!ind->more) gatt_write_cfm(conidx, LE_SUCCESS, handle);

                // Next to process data received
                sess_cb_rxd(conidx, ind->length, ind->value);
                break;
            }
            
            if (att_idx == SES_IDX_TXD_NTF_CFG)
            {
                if ((!ind->more) && (ind->length == sizeof(uint16_t)))
                {
                    uint16_t cli_cfg = read16p(ind->value);

                    // update configuration if value for stop or NTF/IND start
                    if (cli_cfg <= PRF_CLI_START_IND)
                    {
                        DEBUG("  set txd_ntf(cid:%d,cfg:%d)", conidx, cli_cfg);

                        SES_NTF_CFG_SET(conidx, cli_cfg);
                        // Send write conform quickly!
                        gatt_write_cfm(conidx, LE_SUCCESS, handle);

                        #if (SES_CLI_CFG)
                        // Next to process cli_cfg changed
                        sess_cb_ccc(conidx, cli_cfg);
                        #endif //(SES_CLI_CFG)
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
            
            if (att_idx == SES_IDX_RXD_VAL)
            {
                length = SES_RXD_MAX_LEN;  // accepted length
            }
            else if (att_idx == SES_IDX_TXD_NTF_CFG)
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
            
            sess_env.nb_pkt++; // release

            DEBUG("  cmp_evt(op:0x%x,sta:0x%x,nb:%d)", evt->operation, evt->status, sess_env.nb_pkt);
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
 * @brief Add Serial Service Profile in the DB.
 *        Customize via pre-define @see SES_START_HDL
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t sess_svc_init(void)
{
    uint8_t status = LE_SUCCESS;

    // Init Environment
    sess_env.start_hdl = SES_START_HDL;
    sess_env.nb_pkt    = SES_NB_PKT_MAX;
    sess_env.ntf_bits  = 0;
    
    if (sys_config.uuid_len == ATT_UUID128_LEN)
    {
        ses_svc_db.info = SVC_UUID(128);
        
        ses_svc_db.uuid128 = (uint8_t *)&sys_config.uuids;
        
        ses_atts[SES_IDX_TXD_VAL-1].uuid128 = (uint8_t *)&sys_config.uuidn;
        ses_atts[SES_IDX_RXD_VAL-1].uuid128 = (uint8_t *)&sys_config.uuidw;

        ses_atts[SES_IDX_TXD_VAL-1].info |= ATT_UUID(128);
        ses_atts[SES_IDX_RXD_VAL-1].info |= ATT_UUID(128);
    }
    else
    {
        memcpy((uint8_t *)&ses_svc_db.uuid, (uint8_t *)&sys_config.uuids, sys_config.uuid_len);
        memcpy((uint8_t *)&ses_atts[SES_IDX_TXD_VAL-1].uuid, (uint8_t *)&sys_config.uuidn, sys_config.uuid_len);
        memcpy((uint8_t *)&ses_atts[SES_IDX_RXD_VAL-1].uuid, (uint8_t *)&sys_config.uuidw, sys_config.uuid_len);
    }
    
    // Create Service in database
    status = attmdb_svc_create(&sess_env.start_hdl, NULL, &ses_svc_db, sess_svc_func);
    
    DEBUG("svc_init(sta:0x%X,shdl:%d,nb_pkt:%d,ntf_bits:0x%X)", 
            status, sess_env.start_hdl, sess_env.nb_pkt, sess_env.ntf_bits);

    return status;
}

/**
 ****************************************************************************************
 * @brief Enable setting client configuration characteristics
 *
 * @param[in] conidx   Connection index
 * @param[in] cli_cfg  Client configuration @see prf_cli_conf
 ****************************************************************************************
 */
void sess_set_ccc(uint8_t conidx, uint8_t cli_cfg)
{
    //if (gapc_get_conhdl(conidx) != GAP_INVALID_CONHDL)
    {
        // update configuration
        SES_NTF_CFG_SET(conidx, cli_cfg);
    }
}

/**
 ****************************************************************************************
 * @brief Transmit data to peer device via NTF or IND
 *
 * @param[in] conidx   peer device connection index
 * @param[in] len      Length of data
 * @param[in] data     pointer of buffer
 *
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
uint8_t sess_txd_send(uint8_t conidx, uint16_t len, const uint8_t* data)
{
    uint8_t status = PRF_ERR_REQ_DISALLOWED;
    
    if ((len > 0) && (sess_env.nb_pkt > 0))
    {
        uint8_t ntf_cfg = SES_NTF_CFG_GET(conidx);

        if (ntf_cfg != PRF_CLI_STOP_NTFIND)
        {
            status = LE_SUCCESS;
            gatt_ntf_send(conidx, sess_get_att_handle(SES_IDX_TXD_VAL), len, data);

            sess_env.nb_pkt--; // allocate
            DEBUG("txd_send(len:%d,nb:%d)", len, sess_env.nb_pkt);
        }
        else
        {
            status = PRF_ERR_NTF_DISABLED;
        }
    }
    
    return status;
}

/**
 ****************************************************************************************
 * @brief Callback on received data from peer device via WC or WQ (__weak func)
 *
 * @param[in] conidx   peer device connection index
 * @param[in] len      Length of data
 * @param[in] data     pointer of buffer
 ****************************************************************************************
 */
__weak void sess_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    debugHex(data, len);
    
    // Loopback to txd, just test.
    //sess_txd_send(conidx, len, data);
}

#if (SES_READ_SUP)
/**
 ****************************************************************************************
 * @brief Callback to response 'READ' from peer device (__weak func)
 *
 * @param[in] conidx  peer device connection index
 * @param[in] attidx  SESS attribute index, converted with 'handle'
 * @param[in] handle  SESS attribute handle to send read cfm
 *
 * @return Length of value been READ
 ****************************************************************************************
 */
__weak void sess_cb_rdv(uint8_t conidx, uint8_t attidx, uint16_t handle)
{
    uint16_t length = SES_VERS_STR_LEN;
    const uint8_t *p_data = (const uint8_t *)SES_VERS_STR;
    
    DEBUG("  read_cfm(att:%d, len:%d)", attidx, length);
    gatt_read_cfm(conidx, LE_SUCCESS, handle, length, p_data);
}
#endif //(SES_READ_SUP)

#if (SES_CLI_CFG)
/**
 ****************************************************************************************
 * @brief Callback on enabled client config from peer device via WQ (__weak func)
 *
 * @param[in] conidx   Connection index
 * @param[in] cli_cfg  Client configuration @see prf_cli_conf
 ****************************************************************************************
 */
__weak void sess_cb_ccc(uint8_t conidx, uint8_t cli_cfg)
{
    // user override
}
#endif //(SES_CLI_CFG)


#endif //PRF_SESS
