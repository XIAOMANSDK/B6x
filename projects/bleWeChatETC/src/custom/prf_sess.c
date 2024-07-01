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
#include "weChat.h"
#include "proto.h"

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
#define SES_NTF_CFG_GET(ssidx,conidx)  \
    ((sess_env[ssidx].ntf_bits >> (conidx*2)) & (PRF_CLI_START_NTF | PRF_CLI_START_IND))

#define SES_NTF_CFG_CLR(ssidx,conidx)  \
    sess_env[ssidx].ntf_bits &= ~((PRF_CLI_START_NTF | PRF_CLI_START_IND) << (conidx*2))

#define SES_NTF_CFG_SET(ssidx,conidx, conf)  \
    sess_env[ssidx].ntf_bits = (sess_env[ssidx].ntf_bits & ~((PRF_CLI_START_NTF | PRF_CLI_START_IND) << (conidx*2))) | ((conf) << (conidx*2))

#define SESS_ENV_MAX          (4)
#define SES_ATT_IDX_MAX       (SESS_ENV_MAX*16)

uint8_t SES_ATT_IDX[SES_ATT_IDX_MAX] = 
{
    SES_IDX_SVC,
    
    SES_IDX_TXD_CHAR,
    SES_IDX_TXD_VAL,
    SES_IDX_TXD_NTF_CFG,
    
    SES_IDX_RXD_CHAR,
    SES_IDX_RXD_VAL,
    
    SES_IDX_READ_CHAR,
    SES_IDX_READ_VAL,  // 8
    
    
    SES_IDX_SVC,

    SES_IDX_READ_CHAR,
    SES_IDX_READ_VAL,
    SES_IDX_USER_DESC1,
    
    SES_IDX_TXD_CHAR,
    SES_IDX_TXD_VAL,
    SES_IDX_TXD_NTF_CFG,
    SES_IDX_USER_DESC1,
    
    SES_IDX_RXD_CHAR,
    SES_IDX_RXD_VAL,
    SES_IDX_USER_DESC1,
    
    SES_IDX_TXD_CHAR,
    SES_IDX_TXD_VAL,
    SES_IDX_TXD_NTF_CFG,
    SES_IDX_USER_DESC1,
};

  
READ_INFO_T ses_read_info[SESS_ENV_MAX*2] = 
{
    {
        .length = SES_VERS_STR_LEN,
        .data = SES_VERS_STR,
    },
    
    {
        .length = SES_VERS_STR_LEN,
        .data = SES_VERS_STR,
    },
};
/**
 ****************************************************************************************
 * @section ENVIRONMENT DEFINITION
 ****************************************************************************************
 */

/// Global Variable Declarations
sess_env_t sess_env[SESS_ENV_MAX+1];


/**
 ****************************************************************************************
 * @section ATTRIBUTES DEFINITION
 ****************************************************************************************
 */
#if (SES_UUID_128)

#define ATT_UUID_TYPE  ATT_UUID(128)
#define SVC_UUID_TYPE  SVC_UUID(128)

/// Characteristic Base UUID128 (User Customize)
#define SES_ATT_UUID128(uuid)     { 0x16, 0x0A, 0x10, 0x40, 0xD1, 0x9F, 0x4C, 0x6C, \
                                    0xB4, 0x55, 0xE3, 0xF7, (uuid) & 0xFF, (uuid >> 8) & 0xFF, 0x00, 0x00 }
                                  
UUID_INFO_T sess_uuid[SESS_ENV_MAX*4] =
{
    SES_ATT_UUID128(0xFEE7),// Serial Service UUID128
    SES_ATT_UUID128(0xFEC8),// Serial Notify UUID128
    SES_ATT_UUID128(0xFEC7),// Serial Write Command UUID128
    SES_ATT_UUID128(0xFEC9),// Serial Read Command UUID128
    
    SES_ATT_UUID128(0x0000),
    SES_ATT_UUID128(0x0000),
    SES_ATT_UUID128(0x0000),
    SES_ATT_UUID128(0x0000),
    SES_ATT_UUID128(0x0000),
    SES_ATT_UUID128(0x0000),
    SES_ATT_UUID128(0x0000),
    SES_ATT_UUID128(0x0000),
    SES_ATT_UUID128(0x0000),
    SES_ATT_UUID128(0x0000),
    SES_ATT_UUID128(0x0000),
    SES_ATT_UUID128(0x0000),
};

/// Attributes Description
att_decl_t ses_atts[SES_ATT_IDX_MAX] = 
{
};

uint16_t ses_atts_perm[SESS_ENV_MAX*4] = 
{
    (PROP_NTF | PROP_IND),
    (PROP_WC | PROP_WR),
    (PROP_RD),

};

/// Service Description
struct svc_decl ses_svc_db[SESS_ENV_MAX] = 
{
    {
        .uuid128 = sess_uuid[0].uuid, 
        .info    = SVC_UUID(128),
        .atts    = ses_atts,
        .nb_att  = 0,
    }
    
};

#else //(SESS_UUID16)

#define ATT_UUID_TYPE  ATT_UUID(16)
#define SVC_UUID_TYPE  SVC_UUID(16)
/// Characteristic Base UUID128 (User Customize)
#define SES_ATT_UUID16(uuid)     {(uuid) & 0xFF, (uuid >> 8) & 0xFF}

UUID_INFO_T sess_uuid[SESS_ENV_MAX*4] =
{
    SES_ATT_UUID16(0xFEE7),// Serial Service UUID128
    SES_ATT_UUID16(0xFEC8),// Serial Notify UUID128
    SES_ATT_UUID16(0xFEC7),// Serial Write Command UUID128
    SES_ATT_UUID16(0xFEC9),// Serial Read Command UUID128
    
    SES_ATT_UUID16(0xFF17),
    SES_ATT_UUID16(0xFFE4),
    SES_ATT_UUID16(0xFF02),
    SES_ATT_UUID16(0xFF01),
    SES_ATT_UUID16(0xFF03),
    SES_ATT_UUID16(0x0000),
    SES_ATT_UUID16(0x0000),
    SES_ATT_UUID16(0x0000),
    SES_ATT_UUID16(0x0000),
    SES_ATT_UUID16(0x0000),
    SES_ATT_UUID16(0x0000),
    SES_ATT_UUID16(0x0000),
};

/// Attributes Description
att_decl_t ses_atts[SES_ATT_IDX_MAX] = 
{
};

uint16_t ses_atts_perm[SESS_ENV_MAX*4] = 
{
    (PROP_IND),
    (PROP_WC),
    (PROP_RD),  // 3

    (PROP_RD),
    (PROP_IND),
    (PROP_WC),
    (PROP_NTF),
};

/// Service Description
struct svc_decl ses_svc_db[SESS_ENV_MAX] = 
{
    {
        .info    = SVC_UUID_TYPE,
        .atts    = ses_atts,
        .nb_att  = 0,
    }
    
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
uint16_t sess_get_att_handle(uint8_t att_idx)
{
    ASSERT_ERR(att_idx < sess_inx_nmb);
    
    uint8_t nb_att = 0;
    
    for (uint8_t ssidx = 0; ssidx < SESS_ENV_MAX; ssidx++)
    {
        nb_att += ses_svc_db[ssidx].nb_att + 1;
        
        if (att_idx < nb_att)    
        return att_idx + sess_env[0].start_hdl;      
    }
    
    return 0;
}

/// Retrieve attribute index form handle
static uint16_t sess_get_att_idx(uint16_t handle)
{
    ASSERT_ERR((handle >= sess_env[0].start_hdl) && (handle < sess_env[SESS_ENV_MAX+1].start_hdl));
    
    for (uint8_t ssidx = 0; ssidx < SESS_ENV_MAX; ssidx++)
    {
        if (handle < (sess_env[ssidx].start_hdl + ses_svc_db[ssidx].nb_att + 1))      
        return ((handle - sess_env[0].start_hdl) | (ssidx << 8));      
    } 

    return 0;    
}

extern void sess_cb_rxd1(uint16_t handle, uint16_t len, const uint8_t *data);

/// Handles reception of the atts request from peer device
static void sess_svc_func(uint8_t conidx, uint8_t opcode, uint16_t handle, const void *param)
{
    uint16_t val_idx = sess_get_att_idx(handle);
    uint8_t  att_idx = val_idx & 0xFF;
    uint8_t  ssidx   = (val_idx >> 8);
    
    DEBUG("svc_func(cid:%d,op:0x%x,hdl:0x%x,att:%d,ssidx:%d)", conidx, opcode, handle, att_idx, ssidx);

    switch (opcode)
    {
        case ATTS_READ_REQ:
        {
            if (SES_ATT_IDX[att_idx] == SES_IDX_TXD_NTF_CFG)
            {
                // retrieve notification config
                uint16_t cli_cfg = SES_NTF_CFG_GET(ssidx, conidx);
                
                DEBUG("  read_cfm(txd_ntf:%d)", cli_cfg);
                gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&cli_cfg);
                break;
            }

            #if (SES_READ_SUP)
            if (SES_ATT_IDX[att_idx] == SES_IDX_READ_VAL)
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

            if (SES_ATT_IDX[att_idx] == SES_IDX_RXD_VAL)
            {
                // Send write conform first!
                if (!ind->more) gatt_write_cfm(conidx, LE_SUCCESS, handle);

                // Next to process data received
                sess_cb_rxd1(handle, ind->length, ind->value);
                break;
            }
            
            if (SES_ATT_IDX[att_idx] == SES_IDX_TXD_NTF_CFG)
            {
                if ((!ind->more) && (ind->length == sizeof(uint16_t)))
                {
                    uint16_t cli_cfg = read16p(ind->value);

                    // update configuration if value for stop or NTF/IND start
                    if (cli_cfg <= PRF_CLI_START_IND)
                    {
                        DEBUG("  set txd_ntf(cid:%d,cfg:%d)", conidx, cli_cfg);

                        SES_NTF_CFG_SET(ssidx, conidx, cli_cfg);
                        // Send write conform quickly!
                        gatt_write_cfm(conidx, LE_SUCCESS, handle);

                        #if (SES_CLI_CFG)
                        // Next to process cli_cfg changed
                        sess_cb_ccc(conidx, cli_cfg);
                        #endif //(SES_CLI_CFG)
                        
//                        pt_rsp_ntfind_res(0xFEE7, cli_cfg);
                        
                        if(cli_cfg && (ble_head_nSeq == 1))
                        {
                            //主动发送登录请求 ECI_REQ_AUTH  UUID:0xFEC8
//                            ble_req_auth();
                            fee7_ind_cfg_timer_init();  // delay
                        }
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
            
            if (SES_ATT_IDX[att_idx] == SES_IDX_RXD_VAL)
            {
                length = SES_RXD_MAX_LEN;  // accepted length
            }
            else if (SES_ATT_IDX[att_idx] == SES_IDX_TXD_NTF_CFG)
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
            
            sess_env[ssidx].nb_pkt++; // release

            DEBUG("  cmp_evt(op:0x%x,sta:0x%x,nb:%d)", evt->operation, evt->status, sess_env[ssidx].nb_pkt);
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
#include "proto.h"

static bool upHandle = true;

//uint8_t sess_inx_nmb = 8;
//uint8_t sess_uuid_nmb = 4;
//uint8_t sess_read_nmb = 1;
//uint8_t ses_perm_nmb = 3;

uint8_t sess_inx_nmb = 23;
uint8_t sess_uuid_nmb = 9;
uint8_t sess_read_nmb = 2;
uint8_t ses_perm_nmb = 7;

uint8_t sess_svc_init(void)
{
    uint8_t status = LE_SUCCESS;
    uint8_t ss_cnt = 0;
    uint8_t at_cnt = 0;
    uint8_t id_cnt = 0;
    uint8_t as_cnt = 0;
    uint8_t pm_cnt = 0;
    
    if (upHandle)
    {
        memset(&SES_ATT_IDX[sess_inx_nmb], 0xFF, sizeof(SES_ATT_IDX) - sess_inx_nmb);
    }

    for (uint8_t atidx = 0; atidx < sess_inx_nmb; atidx++)
    {
        if (SES_ATT_IDX[atidx] == SES_IDX_SVC)
        {
            if (ss_cnt)
            as_cnt += ses_svc_db[ss_cnt - 1].nb_att; 
            
            #if (SES_UUID_128)
                ses_svc_db[ss_cnt].uuid128 = sess_uuid[id_cnt++].uuid;
            #else
                ses_svc_db[ss_cnt].uuid = read16p(sess_uuid[id_cnt++].uuid);
            #endif

            ses_svc_db[ss_cnt].info    = SVC_UUID_TYPE,
            ses_svc_db[ss_cnt].atts    = &ses_atts[as_cnt],
            ses_svc_db[ss_cnt].nb_att  = 0;
            
            ss_cnt++;  
        }
        else if (SES_ATT_IDX[atidx] == SES_IDX_TXD_CHAR)
        {
            ses_atts[at_cnt].uuid = ATT_DECL_CHARACTERISTIC;
            ses_atts[at_cnt].perm = PROP_RD;
            ses_atts[at_cnt++].info = 0; 
            
            #if (SES_UUID_128)
                ses_atts[at_cnt].uuid128 = sess_uuid[id_cnt++].uuid;
            #else
                ses_atts[at_cnt].uuid = read16p(sess_uuid[id_cnt++].uuid);
            #endif
            ses_atts[at_cnt].perm = ses_atts_perm[pm_cnt++];
            ses_atts[at_cnt++].info = (ATT_UUID_TYPE | (0));

            ses_atts[at_cnt].uuid = ATT_DESC_CLIENT_CHAR_CFG;
            ses_atts[at_cnt].perm = (PROP_RD | PROP_WR);
            ses_atts[at_cnt++].info = sizeof(uint16_t); 

            ses_svc_db[ss_cnt - 1].nb_att += 3;         
        }
        else if (SES_ATT_IDX[atidx] == SES_IDX_RXD_CHAR)
        {
            ses_atts[at_cnt].uuid = ATT_DECL_CHARACTERISTIC;
            ses_atts[at_cnt].perm = PROP_RD;
            ses_atts[at_cnt++].info = 0; 
            
            #if (SES_UUID_128)
                ses_atts[at_cnt].uuid128 = sess_uuid[id_cnt++].uuid;
            #else
                ses_atts[at_cnt].uuid = read16p(sess_uuid[id_cnt++].uuid);
            #endif
            ses_atts[at_cnt].perm = ses_atts_perm[pm_cnt++];
            ses_atts[at_cnt++].info = (ATT_UUID_TYPE | (SES_RXD_MAX_LEN));

            ses_svc_db[ss_cnt - 1].nb_att += 2;            
        }
        else if (SES_ATT_IDX[atidx] == SES_IDX_READ_CHAR)
        {
            ses_atts[at_cnt].uuid = ATT_DECL_CHARACTERISTIC;
            ses_atts[at_cnt].perm = PROP_RD;
            ses_atts[at_cnt++].info = 0; 
            
            #if (SES_UUID_128)
                ses_atts[at_cnt].uuid128 = sess_uuid[id_cnt++].uuid;
            #else
                ses_atts[at_cnt].uuid = read16p(sess_uuid[id_cnt++].uuid);
            #endif
            ses_atts[at_cnt].perm = ses_atts_perm[pm_cnt++];
            ses_atts[at_cnt++].info = (ATT_UUID_TYPE | (0)); 

            ses_svc_db[ss_cnt - 1].nb_att += 2;            
        }
        else if (SES_ATT_IDX[atidx] == SES_IDX_USER_DESC1)
        {
            ses_atts[at_cnt].uuid = ATT_DESC_CHAR_USER_DESCRIPTION;
            ses_atts[at_cnt].perm = (PROP_RD | PROP_WR);
            ses_atts[at_cnt++].info = sizeof(uint16_t); 

            ses_svc_db[ss_cnt - 1].nb_att += 1;            
        }
    }
    
    // Init Environment
    for (uint8_t ssidx = 0; ssidx < ss_cnt; ssidx++)        
    {
        sess_env[ssidx].start_hdl = SES_START_HDL;
        sess_env[ssidx].nb_pkt    = SES_NB_PKT_MAX;
        sess_env[ssidx].ntf_bits  = 0;  
        
        // Create Service in database
        status = attmdb_svc_create(&sess_env[ssidx].start_hdl, NULL, &ses_svc_db[ssidx], sess_svc_func);
        
        DEBUG("svc_init(sta:0x%X,shdl:%d,nb_pkt:%d,ntf_bits:0x%X,ssidx:%d)", 
            status, sess_env[ssidx].start_hdl, sess_env[ssidx].nb_pkt, sess_env[ssidx].ntf_bits, ssidx);        
    }

    if (upHandle)
    {
        upHandle = false;
        pt_rsp_uuid_handle(sess_get_att_handle(SES_IDX_TXD_VAL));
        pt_rsp_uuid_handle(sess_get_att_handle(SES_IDX_RXD_VAL));    
        
        pt_rsp_uuid_handle(sess_get_att_handle(SES_IDX_TXD_VAL1));
        pt_rsp_uuid_handle(sess_get_att_handle(SES_IDX_RXD_VAL1));    
        pt_rsp_uuid_handle(sess_get_att_handle(SES_IDX_TXD_VAL2));
    }

    
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
//void sess_set_ccc(uint8_t conidx, uint8_t cli_cfg)
//{
//    //if (gapc_get_conhdl(conidx) != GAP_INVALID_CONHDL)
//    {
//        // update configuration
//        SES_NTF_CFG_SET(conidx, cli_cfg);
//    }
//}

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
//uint8_t sess_txd_send(uint8_t conidx, uint16_t len, const uint8_t* data)
//{
//    uint8_t status = PRF_ERR_REQ_DISALLOWED;
//    
//    if ((len > 0) && (sess_env.nb_pkt > 0))
//    {
//        uint8_t ntf_cfg = SES_NTF_CFG_GET(conidx);

//        if (ntf_cfg != PRF_CLI_STOP_NTFIND)
//        {
//            status = LE_SUCCESS;
//            gatt_ntf_send(conidx, sess_get_att_handle(SES_IDX_TXD_VAL), len, data);

//            sess_env.nb_pkt--; // allocate
//            DEBUG("txd_send(len:%d,nb:%d)", len, sess_env.nb_pkt);
//        }
//        else
//        {
//            status = PRF_ERR_NTF_DISABLED;
//        }
//    }
//    
//    return status;
//}

uint8_t sess_txd_send1(uint16_t handle, uint16_t len, const uint8_t* data)
{
    uint8_t status = PRF_ERR_REQ_DISALLOWED;
    uint8_t ssidx;
    
    for (uint8_t idx = 0; idx < SESS_ENV_MAX; idx++)
    {
        if (handle < (sess_env[idx].start_hdl + ses_svc_db[idx].nb_att + 1))
        {
            ssidx = idx; 
            break;
        }
    }
    
    DEBUG("txd_send(len:%d,nb:%d,ssidx:%d)", len, sess_env[ssidx].nb_pkt,ssidx);
    
    if ((len > 0) && (sess_env[ssidx].nb_pkt > 0))
    {
        uint8_t ntf_cfg = SES_NTF_CFG_GET(ssidx, 0 /*conidx*/);        

        if (ntf_cfg != PRF_CLI_STOP_NTFIND)
        {
            status = LE_SUCCESS;
            
            if (ntf_cfg & PRF_CLI_START_NTF)
                gatt_ntf_send(0 /*conidx*/, handle, len, data);
            else
                gatt_ind_send(0 /*conidx*/, handle, len, data);
            
            sess_env[ssidx].nb_pkt--; // allocate            
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
//    uint16_t length = SES_VERS_STR_LEN;
//    const uint8_t *p_data = (const uint8_t *)SES_VERS_STR;
    
    uint8_t rdidx = 0;  
    
    for (uint8_t atidx = 0; atidx < sess_inx_nmb; atidx++)
    {
       if (SES_ATT_IDX[atidx] == SES_IDX_READ_VAL)
       {
            if (atidx == attidx)
            {
                DEBUG("  read_cfm(att:%d, len:%d, rdidx:%d)", attidx, ses_read_info[rdidx].length, rdidx);
                gatt_read_cfm(conidx, LE_SUCCESS, handle, ses_read_info[rdidx].length, ses_read_info[rdidx].data); 
                
                break;
            }
            rdidx++;
       }   
    }  
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
