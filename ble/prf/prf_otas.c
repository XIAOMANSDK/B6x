/**
 ****************************************************************************************
 *
 * @file prf_otas.c
 *
 * @brief OTA Service - Server Role Implementation.
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#if (PRF_OTAS)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "prf.h"
#include "prf_otas.h"

#if (DBG_OTAS)
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
#if !defined(OTA_RXD_MAX_LEN)
    #define OTA_RXD_MAX_LEN         (20)
#endif

/// Max number of notify/indicate pkt
#if !defined(OTA_NB_PKT_MAX)
    #define OTA_NB_PKT_MAX          (5)
#endif

/// Version String for OTA_IDX_READ_VAL Read
#define OTA_VERS_STR                ("bleOTA v1.0")
#define OTA_VERS_STR_LEN            (sizeof(OTA_VERS_STR) - 1)

/// Macro for Client Config value operation
#define OTA_NTF_CFG_GET(conidx)  \
    ((otas_env.ntf_bits >> (conidx*2)) & (PRF_CLI_START_NTF | PRF_CLI_START_IND))

#define OTA_NTF_CFG_CLR(conidx)  \
    otas_env.ntf_bits &= ~((PRF_CLI_START_NTF | PRF_CLI_START_IND) << (conidx*2))

#define OTA_NTF_CFG_SET(conidx, conf)  \
    otas_env.ntf_bits = (otas_env.ntf_bits & ~((PRF_CLI_START_NTF | PRF_CLI_START_IND) << (conidx*2))) | ((conf) << (conidx*2))


/**
 ****************************************************************************************
 * @section ENVIRONMENT DEFINITION
 ****************************************************************************************
 */

/// Server Environment Variable
typedef struct otas_env_tag
{
    // Service Start Handle
    uint16_t  start_hdl;    
    // Client Config of peer devices - each 2Bits(NTF & IND), so max_peer=8.
    uint16_t  ntf_bits;
    // Number of notify pkt
    uint8_t   nb_pkt;
} otas_env_t;

/// Global Variable Declarations
otas_env_t otas_env;


/**
 ****************************************************************************************
 * @section ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// Attributes Index
enum ota_att_idx
{
    // Service Declaration, *MUST* Start at 0
    OTA_IDX_SVC,
    
    // Serial TXD Char.
    OTA_IDX_TXD_CHAR,
    OTA_IDX_TXD_VAL,
    OTA_IDX_TXD_NTF_CFG,

    // Serial RXD Char.
    OTA_IDX_RXD_CHAR,
    OTA_IDX_RXD_VAL, //5

    #if (OTA_READ_SUP)
    // Serial READ Char.
    OTA_IDX_READ_CHAR,
    OTA_IDX_READ_VAL,
    #endif

    // Max Index, *NOTE* Minus 1(Svc Decl) is .nb_att
    OTA_IDX_NB,
};

#if (OTA_UUID_128)
/// Characteristic Base UUID128 (User Customize)
#define OTA_ATT_UUID128(uuid)     { 0xFC, 0x12, 0x41, 0x2A, 0xD2, 0xDE, 0x7E, 0x1D, \
                                    0x4D, 0x47, 0xA2, 0x09, (uuid) & 0xFF, (uuid >> 8) & 0xFF, 0x00, 0x00 }

/// Serial Service UUID128
const uint8_t ota_svc_uuid[]        = OTA_ATT_UUID128(0xFF50);
/// Serial Notify UUID128
const uint8_t ota_char_txd_notify[] = OTA_ATT_UUID128(0xFF51);
/// Serial Write Command UUID128
const uint8_t ota_char_rxd_write[]  = OTA_ATT_UUID128(0xFF52);
/// Serial Read Command UUID128
const uint8_t ota_char_val_read[]   = OTA_ATT_UUID128(0xFF53);

/// Attributes Description
const att_decl_t ota_atts[] = 
{
    // Serial Notify Char. Declaration and Value and Client Char. Configuration Descriptor
    ATT_ELMT_DECL_CHAR( OTA_IDX_TXD_CHAR ),
    ATT_ELMT128( OTA_IDX_TXD_VAL,  ota_char_txd_notify, PROP_NTF | PROP_IND, 0 ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( OTA_IDX_TXD_NTF_CFG ),

    // Serial Write Command Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( OTA_IDX_RXD_CHAR ),
    ATT_ELMT128( OTA_IDX_RXD_VAL,  ota_char_rxd_write,  PROP_WC | PROP_WR,   OTA_RXD_MAX_LEN ),
    
    #if (OTA_READ_SUP)
    // Serial Read Command Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( OTA_IDX_READ_CHAR ),
    ATT_ELMT128( OTA_IDX_READ_VAL, ota_char_val_read,   PROP_RD,             0 ),
    #endif //(OTA_READ_SUP)
};

/// Service Description
const struct svc_decl ota_svc_db = 
{
    .uuid128 = ota_svc_uuid, 
    .info    = SVC_UUID(128),
    .atts    = ota_atts,
    .nb_att  = OTA_IDX_NB - 1,
};

#else //(OTAS_UUID16)

/// Serial Service UUID
#define OTA_SVC_UUID                ATT_UUID16(0xFF50)
/// Serial Notify UUID
#define OTA_CHAR_TXD_NOTIFY         ATT_UUID16(0xFF51)
/// Serial Write Command UUID
#define OTA_CHAR_RXD_WRITE          ATT_UUID16(0xFF52)
/// Serial Read Command UUID
#define OTA_CHAR_VAL_READ           ATT_UUID16(0xFF53)

/// Attributes Description
const att_decl_t ota_atts[] = 
{
    // Serial Notify Char. Declaration and Value and CCC Descriptor
    ATT_ELMT_DECL_CHAR( OTA_IDX_TXD_CHAR ),
    ATT_ELMT( OTA_IDX_TXD_VAL,  OTA_CHAR_TXD_NOTIFY, PROP_NTF | PROP_IND, 0 ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( OTA_IDX_TXD_NTF_CFG ),

    // Serial Write Command Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( OTA_IDX_RXD_CHAR ),
    ATT_ELMT( OTA_IDX_RXD_VAL,  OTA_CHAR_RXD_WRITE,  PROP_WC | PROP_WR,   OTA_RXD_MAX_LEN ),
    
    #if (OTA_READ_SUP)
    // Serial Read Command Char. Declaration and Value
    ATT_ELMT_DECL_CHAR( OTA_IDX_READ_CHAR ),
    ATT_ELMT( OTA_IDX_READ_VAL, OTA_CHAR_VAL_READ,   PROP_RD,             0 ),
    #endif //(OTA_READ_SUP)
};

/// Service Description
const struct svc_decl ota_svc_db = 
{
    .uuid   = OTA_SVC_UUID, 
    .info   = SVC_UUID(16),
    .atts   = ota_atts,
    .nb_att = OTA_IDX_NB - 1,
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

/// Retrieve attribute handle from index (@see ota_att_idx)
static uint16_t otas_get_att_handle(uint8_t att_idx)
{
    ASSERT_ERR(att_idx < OTA_IDX_NB);

    return att_idx + otas_env.start_hdl;
}

/// Retrieve attribute index form handle
static uint8_t otas_get_att_idx(uint16_t handle)
{
    ASSERT_ERR((handle >= otas_env.start_hdl) && (handle < otas_env.start_hdl + OTA_IDX_NB));

    return handle - otas_env.start_hdl;
}

/// Handles reception of the atts request from peer device
static void otas_svc_func(uint8_t conidx, uint8_t opcode, uint16_t handle, const void *param)
{
    uint8_t att_idx = otas_get_att_idx(handle);

    DEBUG("svc_func(cid:%d,op:0x%x,hdl:0x%x,att:%d)", conidx, opcode, handle, att_idx);

    switch (opcode)
    {
        case ATTS_READ_REQ:
        {
            if (att_idx == OTA_IDX_TXD_NTF_CFG)
            {
                // retrieve notification config
                uint16_t cli_cfg = OTA_NTF_CFG_GET(conidx);
                
                DEBUG("  read_cfm(txd_ntf:%d)", cli_cfg);
                gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&cli_cfg);
                break;
            }

            #if (OTA_READ_SUP)
            if (att_idx == OTA_IDX_READ_VAL)
            {
                otas_cb_rdv(conidx, att_idx, handle);
                break;
            }
            #endif //(OTA_READ_SUP)

            // Send error response
            gatt_read_cfm(conidx, PRF_ERR_APP_ERROR, handle, 0, NULL);
        } break;
        
        case ATTS_WRITE_REQ:
        {
            const struct atts_write_ind *ind = param;
            
            DEBUG("  write_req(hdl:0x%x,att:%d,wr:0x%x,len:%d)", handle, att_idx, ind->wrcode, ind->length);

            if (att_idx == OTA_IDX_RXD_VAL)
            {
                // Send write conform first!
                if (!ind->more) gatt_write_cfm(conidx, LE_SUCCESS, handle);

                // Next to process data received
                otas_cb_rxd(conidx, ind->length, ind->value);
                break;
            }
            
            if (att_idx == OTA_IDX_TXD_NTF_CFG)
            {
                if ((!ind->more) && (ind->length == sizeof(uint16_t)))
                {
                    uint16_t cli_cfg = read16p(ind->value);

                    // update configuration if value for stop or NTF/IND start
                    if (cli_cfg <= PRF_CLI_START_IND)
                    {
                        DEBUG("  set txd_ntf(cid:%d,cfg:%d)", conidx, cli_cfg);

                        OTA_NTF_CFG_SET(conidx, cli_cfg);
                        // Send write conform quickly!
                        gatt_write_cfm(conidx, LE_SUCCESS, handle);

                        #if (OTA_CLI_CFG)
                        // Next to process cli_cfg changed
                        otas_cb_ccc(conidx, cli_cfg);
                        #endif //(OTA_CLI_CFG)
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
            
            if (att_idx == OTA_IDX_RXD_VAL)
            {
                length = OTA_RXD_MAX_LEN;  // accepted length
            }
            else if (att_idx == OTA_IDX_TXD_NTF_CFG)
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
            
            otas_env.nb_pkt++; // release

            DEBUG("  cmp_evt(op:0x%x,sta:0x%x,nb:%d)", evt->operation, evt->status, otas_env.nb_pkt);
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
 *        Customize via pre-define @see OTA_START_HDL
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t otas_svc_init(void)
{
    uint8_t status = LE_SUCCESS;

    // Init Environment
    otas_env.start_hdl = OTA_START_HDL;
    otas_env.nb_pkt    = OTA_NB_PKT_MAX;
    otas_env.ntf_bits  = 0;

    // Create Service in database
    status = attmdb_svc_create(&otas_env.start_hdl, NULL, &ota_svc_db, otas_svc_func);
    
    DEBUG("svc_init(sta:0x%X,shdl:%d,nb_pkt:%d,ntf_bits:0x%X)", 
            status, otas_env.start_hdl, otas_env.nb_pkt, otas_env.ntf_bits);

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
void otas_set_ccc(uint8_t conidx, uint8_t cli_cfg)
{
    //if (gapc_get_conhdl(conidx) != GAP_INVALID_CONHDL)
    {
        // update configuration
        OTA_NTF_CFG_SET(conidx, cli_cfg);
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
uint8_t otas_txd_send(uint8_t conidx, uint16_t len, const uint8_t* data)
{
    uint8_t status = PRF_ERR_REQ_DISALLOWED;
    
    if ((len > 0) && (otas_env.nb_pkt > 0))
    {
        uint8_t ntf_cfg = OTA_NTF_CFG_GET(conidx);

        if (ntf_cfg != PRF_CLI_STOP_NTFIND)
        {
            status = LE_SUCCESS;
            gatt_ntf_send(conidx, otas_get_att_handle(OTA_IDX_TXD_VAL), len, data);

            otas_env.nb_pkt--; // allocate
            DEBUG("txd_send(len:%d,nb:%d)", len, otas_env.nb_pkt);
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
__weak void otas_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    debugHex(data, len);
    
    // Loopback to txd, just test.
    //otas_txd_send(conidx, len, data);
}

#if (OTA_READ_SUP)
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
__weak void otas_cb_rdv(uint8_t conidx, uint8_t attidx, uint16_t handle)
{
    uint16_t length = OTA_VERS_STR_LEN;
    const uint8_t *p_data = (const uint8_t *)OTA_VERS_STR;
    
    DEBUG("  read_cfm(att:%d, len:%d)", attidx, length);
    gatt_read_cfm(conidx, LE_SUCCESS, handle, length, p_data);
}
#endif //(OTA_READ_SUP)

#if (OTA_CLI_CFG)
/**
 ****************************************************************************************
 * @brief Callback on enabled client config from peer device via WQ (__weak func)
 *
 * @param[in] conidx   Connection index
 * @param[in] cli_cfg  Client configuration @see prf_cli_conf
 ****************************************************************************************
 */
__weak void otas_cb_ccc(uint8_t conidx, uint8_t cli_cfg)
{
    // user override
}
#endif //(OTA_CLI_CFG)


#endif //PRF_OTAS
