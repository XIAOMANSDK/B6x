/**
 ****************************************************************************************
 *
 * @file prf_ptss.c
 *
 * @brief Profile Testing Service - Server Role Implementation.
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#if (PRF_PTSS)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "prf.h"
#include "prf_ptss.h"

#if (DBG_PTSS)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<PTSS>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINITIONS
 ****************************************************************************************
 */

/// Version String for Read
#define PTS_VERS_STR                ("Ver:1.25")
#define PTS_VERS_STR_LEN            (sizeof(PTS_VERS_STR) - 1)

/// Macro for Client Config value operation
#define PTS_CLI_CFG_GET(conidx)  \
    ((ptss_env.cli_cfg >> (conidx*2)) & (PRF_CLI_START_NTF | PRF_CLI_START_IND))

#define PTS_CLI_CFG_CLR(conidx)  \
    ptss_env.cli_cfg &= ~((PRF_CLI_START_NTF | PRF_CLI_START_IND) << (conidx*2))

#define PTS_CLI_CFG_SET(conidx, conf)  \
    ptss_env.cli_cfg = (ptss_env.cli_cfg & ~((PRF_CLI_START_NTF | PRF_CLI_START_IND) << (conidx*2))) | ((conf) << (conidx*2))


/**
 ****************************************************************************************
 * @section ENVIRONMENT DEFINITION
 ****************************************************************************************
 */

/// Server Environment Variable
typedef struct ptss_env_tag
{
    // Service Start Handle
    uint16_t  start_hdl;
    // CCC of peer devices(bits).
    uint16_t   cli_cfg;
} ptss_env_t;

/// Global Variable Declarations
ptss_env_t ptss_env;

/// Global state of bonded, changed in gapc
uint8_t pts_bond;

/// Buffer of received data
static uint16_t recv_len = PTS_DATA_MAX_LEN;
static uint8_t  recv_buf[PTS_DATA_MAX_LEN];

/// Buffer of read data
static uint16_t read_len = PTS_DATA_MAX_LEN;
static uint8_t  read_buf[PTS_DATA_MAX_LEN];

/// Buffer of desc data
static uint16_t desc_len = PTS_DESC_MAX_LEN;
static uint8_t  desc_buf[PTS_DESC_MAX_LEN];


/**
 ****************************************************************************************
 * @section ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// Service Attributes Indexes
enum pts_att_idx
{
    // Service Declaration, *MUST* Start at 0
    PTS_IDX_SVC,

    // Attribute No.1: NTF/IND/RD
    PTS_IDX_ATT1_CHAR,
    PTS_IDX_ATT1_VAL,
    PTS_IDX_ATT1_DESC,

    // Attribute No.2: WR/WC/WS
    PTS_IDX_ATT2_CHAR,
    PTS_IDX_ATT2_VAL,

    // Attribute No.3: RD
    PTS_IDX_ATT3_CHAR,
    PTS_IDX_ATT3_VAL,
    
    // Max Index, *NOTE* Minus 1(Svc Decl) is .nb_att
    PTS_IDX_NB,
};

/////////////////////////////////////////////////////////////////////////////
/// *** 0: Test for Normal ***
/////////////////////////////////////////////////////////////////////////////

#define PTS0_SVC_UUID              ATT_UUID16(0xFF00)

#define PTS0_CHAR_ATT1             ATT_UUID16(0xFF01)
#define PTS0_CHAR_ATT2             ATT_UUID16(0xFF02)
#define PTS0_CHAR_ATT3             ATT_UUID16(0xFF03)
    
const att_decl_t pts0_atts[] =
{
    // No.1 Characteristic Declaration and Value and CCC Descriptor
    ATT_ELMT_DECL_CHAR( PTS_IDX_ATT1_CHAR ),
    ATT_ELMT( PTS_IDX_ATT1_VAL,  PTS0_CHAR_ATT1,  PROP_NTF|PROP_IND,        0 ),
    ATT_ELMT_DESC_CLI_CHAR_CFG( PTS_IDX_ATT1_DESC ),

    // No.2 Characteristic Declaration and Value
    ATT_ELMT_DECL_CHAR( PTS_IDX_ATT2_CHAR ),
    ATT_ELMT( PTS_IDX_ATT2_VAL,  PTS0_CHAR_ATT2,  PROP_WC|PROP_WR|PROP_WS,  PTS_DATA_MAX_LEN ),
    
    // No.3 Characteristic Declaration and Value
    ATT_ELMT_DECL_CHAR( PTS_IDX_ATT3_CHAR ),
    ATT_ELMT( PTS_IDX_ATT3_VAL,  PTS0_CHAR_ATT3,  PROP_RD,                  0 ),
};

const struct svc_decl pts0_svc_db = 
{
    .uuid   = PTS0_SVC_UUID, 
    .info   = SVC_UUID(16),
    .atts   = pts0_atts,
    .nb_att = PTS_IDX_NB - 1,
};

/////////////////////////////////////////////////////////////////////////////
/// *** 1: Test for ERROR ***
/////////////////////////////////////////////////////////////////////////////

#define PTS1_SVC_UUID              ATT_UUID16(0xFF20)

#define PTS1_CHAR_ATT1             ATT_UUID16(0xFF21)
#define PTS1_CHAR_ATT2             ATT_UUID16(0xFF22)
#define PTS1_CHAR_ATT3             ATT_UUID16(0xFF23)
    
const att_decl_t pts1_atts[] =
{
    // No.1 Characteristic Declaration and Value and User Descriptor
    ATT_ELMT_DECL_CHAR( PTS_IDX_ATT1_CHAR ),
    ATT_ELMT( PTS_IDX_ATT1_VAL,  PTS1_CHAR_ATT1,                  PROP_WC|PROP_WR|PROP_RD,  PTS_DATA_MAX_LEN ),
    ATT_ELMT( PTS_IDX_ATT1_DESC, ATT_DESC_CHAR_USER_DESCRIPTION,  PROP_RD,                  0 ),

    // No.2 Characteristic Declaration and Value
    ATT_ELMT_DECL_CHAR( PTS_IDX_ATT2_CHAR ),
    ATT_ELMT( PTS_IDX_ATT2_VAL,  PTS1_CHAR_ATT2,                  PROP_WC|PROP_WR|PROP_RD,  PTS_DATA_MAX_LEN ),
    
    // No.3 Characteristic Declaration and Value
    ATT_ELMT_DECL_CHAR( PTS_IDX_ATT3_CHAR ),
    ATT_ELMT( PTS_IDX_ATT3_VAL,  PTS1_CHAR_ATT3,                  PROP_WC|PROP_WR|PROP_RD,  PTS_DATA_MAX_LEN ),
};

const struct svc_decl pts1_svc_db = 
{
    .uuid   = PTS1_SVC_UUID, 
    .info   = SVC_UUID(16),
    .atts   = pts1_atts,
    .nb_att = PTS_IDX_NB - 1,
};

/////////////////////////////////////////////////////////////////////////////
/// *** 2: Test for UUID128 ***
/////////////////////////////////////////////////////////////////////////////

#define PTS_ATT_UUID128(uuid)    { 0x16, 0x0A, 0x10, 0x40, 0xD1, 0x9F, 0x4C, 0x6C, \
                                   0xB4, 0x55, 0xE3, 0xF7, (uuid) & 0xFF, (uuid >> 8) & 0xFF, 0x00, 0x00}

const uint8_t PTS2_SVC_UUID[]    = PTS_ATT_UUID128(0xFF00);

const uint8_t PTS2_CHAR_ATT1[]   = PTS_ATT_UUID128(0xFF01);
const uint8_t PTS2_CHAR_ATT2[]   = PTS_ATT_UUID128(0xFF02);
const uint8_t PTS2_CHAR_ATT3[]   = PTS_ATT_UUID128(0xFF03);

const att_decl_t pts2_atts[] =
{
    // No.1 Characteristic Declaration and Value and User Descriptor
    ATT_ELMT_DECL_CHAR( PTS_IDX_ATT1_CHAR ),
    ATT_ELMT128( PTS_IDX_ATT1_VAL, PTS2_CHAR_ATT1,                  PROP_RD,                  0 ),
    ATT_ELMT( PTS_IDX_ATT1_DESC,   ATT_DESC_CHAR_USER_DESCRIPTION,  PROP_RD|PROP_WR,          PTS_DESC_MAX_LEN ),

    // No.2 Characteristic Declaration and Value
    ATT_ELMT_DECL_CHAR( PTS_IDX_ATT2_CHAR ),
    ATT_ELMT128( PTS_IDX_ATT2_VAL, PTS2_CHAR_ATT2,                  PROP_WC|PROP_WR|PROP_RD,  PTS_DATA_MAX_LEN ),
    
    // No.3 Characteristic Declaration and Value
    ATT_ELMT_DECL_CHAR( PTS_IDX_ATT3_CHAR ),
    ATT_ELMT128( PTS_IDX_ATT3_VAL, PTS2_CHAR_ATT3,                  PROP_RD|PROP_WR,          PTS_DATA_MAX_LEN ),
};

const struct svc_decl pts_svc_db2 = 
{
    .uuid128 = PTS2_SVC_UUID, 
    .info    = SVC_UUID(128),
    .atts    = pts2_atts,
    .nb_att  = PTS_IDX_NB - 1,
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

/// Retrieve attribute handle from index (@see pts_att_idx)
static uint16_t ptss_get_att_handle(uint8_t att_idx)
{
    ASSERT_ERR(att_idx < PTS_ATT_NB); //svr_idx=0
    
    return (att_idx + ptss_env.start_hdl);
}

/// Retrieve attribute index form handle or ATT_INVALID_IDX if nothing found
static uint8_t ptss_get_att_idx(uint8_t *svr_idx, uint16_t handle)
{
    uint8_t att_idx = ATT_INVALID_IDX;

    if ((handle >= ptss_env.start_hdl) && (handle < ptss_env.start_hdl + PTS_IDX_NB))
    {
        att_idx  = handle - ptss_env.start_hdl;
        *svr_idx = 0;
    }
    else if ((handle >= ptss_env.start_hdl + PTS_IDX_NB) && (handle < ptss_env.start_hdl + PTS_IDX_NB*2))
    {
        att_idx = handle - (ptss_env.start_hdl + PTS_IDX_NB);
        *svr_idx = 1;
    }
    else if ((handle >= ptss_env.start_hdl + PTS_IDX_NB*2) && (handle < ptss_env.start_hdl + PTS_IDX_NB*3))
    {
        att_idx = handle - (ptss_env.start_hdl + PTS_IDX_NB*2);
        *svr_idx = 2;
    }
    
    return (att_idx);
}

/// Handles reception of the attribute info request message.
static void ptss_att_info_cfm(uint8_t conidx, uint8_t svr_idx, uint8_t att_idx, uint16_t handle)
{
    uint16_t length = 0;
    uint8_t status = LE_SUCCESS;

    if (att_idx != ATT_INVALID_IDX)
    {
        if (svr_idx == 1) // PTS1 Error
        {
            // Long Write Error
            if (att_idx == PTS_IDX_ATT1_VAL)
            {
                if ((pts_bond & GAP_AUTH_BOND) == 0)
                    status = ATT_ERR_INSUFF_AUTHOR;
                else
                    length = PTS_DATA_MAX_LEN;
            }
            else if (att_idx == PTS_IDX_ATT2_VAL)
            {
                if ((pts_bond & GAP_AUTH_MITM) == 0)
                    status = ATT_ERR_INSUFF_AUTHEN;
                else
                    length = PTS_DATA_MAX_LEN;
            }
            else if (att_idx == PTS_IDX_ATT3_VAL)
            {
                //if (!pts_bond)
                    status = ATT_ERR_INSUFF_ENC_KEY_SIZE;
                //else
                //    cfm->length = PTS_DATA_MAX_LEN;
            }
            else
            {
                status = ATT_ERR_WRITE_NOT_PERMITTED;
            }
        }
        else if (svr_idx == 2) // PTS2 Normal
        {
            if ((att_idx == PTS_IDX_ATT2_VAL) || (att_idx == PTS_IDX_ATT3_VAL))
            {
                length = PTS_DATA_MAX_LEN;
            }
            else if (att_idx == PTS_IDX_ATT1_DESC)
            {
                length = PTS_DESC_MAX_LEN;
            }
            else
            {
                status = ATT_ERR_WRITE_NOT_PERMITTED;
            }
        }
        else // PTS0 Normal
        {
            if (att_idx == PTS_IDX_ATT2_VAL)
            {
                length = PTS_DATA_MAX_LEN;
            }
            else if (att_idx == PTS_IDX_ATT1_DESC)
            {
                length = sizeof(uint16_t); // CCC attribute
            }
            else
            {
                status = ATT_ERR_WRITE_NOT_PERMITTED;
            }
        }
    }
    else
    {
        status = PRF_ERR_APP_ERROR;
    }
    
    DEBUG("  info_cfm(svr:%d,hdl:0x%x,sta:0x%x,len:%d)", svr_idx, param->handle, status, length);
    
    // Send info response
    gatt_info_cfm(conidx, status, handle, length);
}

/// Confirm ATTS_WRITE_REQ
static void ptss_att_write_cfm(uint8_t conidx, uint8_t svr_idx, uint8_t att_idx, uint16_t handle, const struct atts_write_ind *ind)
{
    uint8_t status = LE_SUCCESS;
    
    if (att_idx != ATT_INVALID_IDX)
    {
        if (svr_idx == 1) // PTS1 Error
        {
            // Write Error
            if (att_idx == PTS_IDX_ATT1_VAL)
            {
                if ((pts_bond & GAP_AUTH_BOND) == 0)
                    status = ATT_ERR_INSUFF_AUTHOR;
            }
            else if (att_idx == PTS_IDX_ATT2_VAL)
            {
                if ((pts_bond & GAP_AUTH_MITM) == 0)
                    status = ATT_ERR_INSUFF_AUTHEN;
            }
            else if (att_idx == PTS_IDX_ATT3_VAL)
            {
                //if (!pts_bond)
                    status = ATT_ERR_INSUFF_ENC_KEY_SIZE;
            }
            else
            {
                status = ATT_ERR_WRITE_NOT_PERMITTED;
            }
        }
        else if (svr_idx == 2) // PTS2 Normal
        {
            if ((att_idx == PTS_IDX_ATT2_VAL) || (att_idx == PTS_IDX_ATT3_VAL))
            {
                // received data to callback
                ptss_cb_recv(conidx, ind->length, ind->value);
                
                // Save data to read-back
                if (att_idx == PTS_IDX_ATT2_VAL)
                {
                    recv_len = ind->length;
                    memcpy(recv_buf, ind->value, ind->length);
                }
                else
                {
                    read_len = ind->length;
                    memcpy(read_buf, ind->value, ind->length);
                }
            }
            else if (att_idx == PTS_IDX_ATT1_DESC)
            {
                // Save desc to read-back
                desc_len = ind->length;
                memcpy(desc_buf, ind->value, desc_len);
            }
            else
            {
                status = ATT_ERR_WRITE_NOT_PERMITTED;
            }
        }
        else // PTS0 Normal
        {
            if (att_idx == PTS_IDX_ATT2_VAL)
            {
                // received data to callback
                ptss_cb_recv(conidx, ind->length, ind->value);
            }
            else if (att_idx == PTS_IDX_ATT1_DESC)
            {
                // update configuration if value for stop or notification enable
                uint8_t cli_cfg = ind->value[0] & (PRF_CLI_START_NTF | PRF_CLI_START_IND);
                
                PTS_CLI_CFG_SET(conidx, cli_cfg);
                
                // client conf to callback
                ptss_cb_ccc(conidx, cli_cfg);
            }
            else
            {
                status = PRF_ERR_APP_ERROR;
            }
        }
    }
    else
    {
        status = PRF_ERR_APP_ERROR;
    }
    
    DEBUG("  --write_cfm(svr:%d,hdl:0x%x,sta:0x%x)", svr_idx, handle, status);
        
    // Send write conform
    gatt_write_cfm(conidx, status, handle);
}

/// Confirm ATTS_READ_REQ
static void ptss_att_read_cfm(uint8_t conidx, uint8_t svr_idx, uint8_t att_idx, uint16_t handle)
{
    uint16_t length = 0;
    uint8_t status  = LE_SUCCESS;

    if (att_idx != ATT_INVALID_IDX)
    {    
        if (svr_idx == 1) // PTS1 Error
        {
            // Read Error
            if (att_idx == PTS_IDX_ATT1_VAL)
            {
                if ((pts_bond & GAP_AUTH_BOND) == 0)
                    status = ATT_ERR_INSUFF_AUTHOR;
                else
                    length = PTS_DATA_MAX_LEN;
            }
            else if (att_idx == PTS_IDX_ATT2_VAL)
            {
                if ((pts_bond & GAP_AUTH_MITM) == 0)
                    status = ATT_ERR_INSUFF_AUTHEN;
                else
                    length = PTS_DATA_MAX_LEN;
            }
            else if (att_idx == PTS_IDX_ATT3_VAL)
            {
                //if (!pts_bond)
                    status = ATT_ERR_INSUFF_ENC_KEY_SIZE;
                //else
                //    length = PTS_DATA_MAX_LEN;
            }
            else if (att_idx == PTS_IDX_ATT1_DESC)
            {
                length = 2 * gatt_get_mtu(conidx) - 2;
            }
            else
            {
                status = ATT_ERR_READ_NOT_PERMITTED;
            }
        }
        else if (svr_idx == 2) // PTS2 Normal
        {
            if (att_idx == PTS_IDX_ATT1_VAL)
            {
                length = gatt_get_mtu(conidx);
                
                if (length < 512) length--;
            }
            else if (att_idx == PTS_IDX_ATT1_DESC)
            {
                length = desc_len;
            }
            else if (att_idx == PTS_IDX_ATT3_VAL)
            {
                length = read_len;
            }
            else if (att_idx == PTS_IDX_ATT2_VAL)
            {
                length = recv_len;
            }
            else
            {
                status = ATT_ERR_READ_NOT_PERMITTED;
            }
        }
        else // PTS0 Normal
        {
            if (att_idx == PTS_IDX_ATT1_DESC)
            {
                length = sizeof(uint16_t);
            }
            else if (att_idx == PTS_IDX_ATT3_VAL)
            {
                //if (ptss_env.rdv_cb)
                //    length = ptss_env.rdv_cb(conidx, NULL);
                length = gatt_get_mtu(conidx);
                
                if (length < 512) length--;
            }
            else
            {
                status = ATT_ERR_READ_NOT_PERMITTED;
            }
        }
    }
    else
    {
        status = PRF_ERR_APP_ERROR;
    }
    
    DEBUG("  read_cfm(svr:%d,hdl:0x%x,sta:0x%x,len:%d)", svr_idx, param->handle, status, length);

    // Send read response
    if (status == LE_SUCCESS)
    {
        if (svr_idx == 2)
        {
            // PTS2 read-back
            if (att_idx == PTS_IDX_ATT2_VAL)
            {
                gatt_read_cfm(conidx, status, handle, recv_len, recv_buf);
                return;
            }
            else if (att_idx == PTS_IDX_ATT3_VAL)
            {
                gatt_read_cfm(conidx, status, handle, read_len, read_buf);
                return;
            }
            else if (att_idx == PTS_IDX_ATT1_DESC)
            {
                gatt_read_cfm(conidx, status, handle, desc_len, desc_buf);
                return;
            }
        }
        else
        {
            // PTS0 fill data
            if (att_idx == PTS_IDX_ATT1_DESC)
            {
                uint16_t cli_cfg = PTS_CLI_CFG_GET(conidx);
                
                gatt_read_cfm(conidx, status, handle, length, (uint8_t *)&cli_cfg);
                return;
            }
            else if (att_idx == PTS_IDX_ATT3_VAL)
            {
                ptss_cb_read(conidx, att_idx, handle);
                return;
            }     
        }
    }

    gatt_read_cfm(conidx, status, handle, length, NULL);
}

/// Handles reception of the atts request from peer device
static void ptss_svc_func(uint8_t conidx, uint8_t opcode, uint16_t handle, const void *param)
{
    uint8_t svr_idx, att_idx;

    att_idx = ptss_get_att_idx(&svr_idx, handle); 
    DEBUG("svc_func(cid:%d,op:0x%x,hdl:0x%x,svr:%d,att:%d)", conidx, opcode, handle, svr_idx, att_idx);

    switch (opcode)
    {
        case ATTS_READ_REQ:
        {
            ptss_att_read_cfm(conidx, svr_idx, att_idx, handle);
        } break;

        case ATTS_WRITE_REQ:
        {
            const struct atts_write_ind *ind = param;

            DEBUG("  write_req(hdl:0x%x,att:%d,wr:0x%x,len:%d)", handle, att_idx, ind->wrcode, ind->length);
            ptss_att_write_cfm(conidx, svr_idx, att_idx, handle, ind);
        } break;

        case ATTS_INFO_REQ:
        {
            ptss_att_info_cfm(conidx, svr_idx, att_idx, handle);
        } break;

        case ATTS_CMP_EVT:
        {
            const struct atts_cmp_evt *evt = param;
            
            DEBUG("  cmp_evt(op:0x%x,sta:0x%x,seq:%d)", evt->operation, evt->status, evt->seq_num);
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
 * @brief Add Service Profile in the DB
 *        Customize via pre-define @see PTSS_START_HDL
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t ptss_svc_init(void)
{
    uint8_t status = LE_SUCCESS;
    uint16_t next_hdl;

    // Init Environment
    ptss_env.start_hdl = PTS_START_HDL;
    ptss_env.cli_cfg   = 0;

    #if (PTSS_START_HDL)
    *start_hdl = PTSS_START_HDL; // at special handle
    #endif
    
    // Create Service in database
    status = attmdb_svc_create(&ptss_env.start_hdl, NULL, &pts0_svc_db, ptss_svc_func);

    DEBUG("svc_init0(sta:0x%X,shdl:%d)", status, ptss_env.start_hdl);
    if (status != LE_SUCCESS)
    {
        return status;
    }

    next_hdl = ptss_env.start_hdl + PTS_IDX_NB;
    status = attmdb_svc_create(&next_hdl, NULL, &pts1_svc_db, ptss_svc_func);
    
    DEBUG("svc_init1(sta:0x%X,shdl:%d)", status, next_hdl);
    if (status != LE_SUCCESS)
    {
        return status;
    }
    
    next_hdl += PTS_IDX_NB;
    status = attmdb_svc_create(&next_hdl, NULL, &pts_svc_db2, ptss_svc_func);
    if (status != LE_SUCCESS)
    {
        return status;
    }
    
    return status;
}

/**
 ****************************************************************************************
 * @brief Enable setting client configuration characteristics
 *
 * @param[in] conidx   Connection index
 * @param[in] cli_cfg  Client configuration @see prf_cli_conf
 *
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
void ptss_set_ccc(uint8_t conidx, uint8_t cli_cfg)
{
    //if (gapc_get_conhdl(conidx) != GAP_INVALID_CONHDL)
    {
        // update configuration
        PTS_CLI_CFG_SET(conidx, cli_cfg);
    }
}

/**
 ****************************************************************************************
 * @brief Transmit data to peer device via NTF or IND
 *
 * @param[in] conidx   peer destination connection index
 * @param[in] handle   Handle of NTF/IND
 * @param[in] len      Length of data
 * @param[in] data     pointer of buffer
 *
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
uint8_t ptss_evt_send(uint8_t conidx, uint16_t handle, uint16_t len, const uint8_t* data)
{
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if (len > 0)
    {
        uint8_t cli_cfg = PTS_CLI_CFG_GET(conidx);
        
        if (cli_cfg != PRF_CLI_STOP_NTFIND)
        {
            uint8_t operation = (cli_cfg & PRF_CLI_START_NTF) ? GATT_NOTIFY : GATT_INDICATE;
        
            if (handle == 0) ptss_get_att_handle(PTS_IDX_ATT1_VAL);
            
            gatt_ntf_send(conidx, handle, len, data);
            
            DEBUG("Send(hdl:0x%x,op:0x%x,len:%d)", handle, operation, len);
            status = LE_SUCCESS;
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
__weak void ptss_cb_recv(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    DEBUG("Recv(cid:%d,len:%d)", conidx, len);
    debugHex(data, len);
}

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
__weak void ptss_cb_read(uint8_t conidx, uint8_t attidx, uint16_t handle)
{
    uint16_t length = PTS_VERS_STR_LEN;
    const uint8_t *p_data = (const uint8_t *)PTS_VERS_STR;
    
    DEBUG("  read_cfm(att:%d, len:%d)", attidx, length);
    gatt_read_cfm(conidx, LE_SUCCESS, handle, length, p_data);
}

/**
 ****************************************************************************************
 * @brief Callback on enabled client config from peer device via WQ (__weak func)
 *
 * @param[in] conidx   Connection index
 * @param[in] cli_cfg  Client configuration @see prf_cli_conf
 ****************************************************************************************
 */
__weak void ptss_cb_ccc(uint8_t conidx, uint8_t cli_cfg)
{
    DEBUG("Enable(cid:%d,cfg:%d)", conidx, cli_cfg);
}


#endif //PRF_PTSS
