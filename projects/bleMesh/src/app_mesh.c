/**
 ****************************************************************************************
 *
 * @file app_mesh.c
 *
 * @brief App SubTask of GATT Message - Example
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#if (PRF_MESH)

#include "app.h"
#include "drvs.h"
#include "mesh.h"
#include "sig_model.h"
#include "genie_mesh.h"

#if (DBG_MESH)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */


/*
* GLOBAL VARIABLES DECLARATIONS
****************************************************************************************
*/

const tri_tuple_t genie_triple = 
{
    /* pid - 4B */
    .pid = 0x006653de,
    /* key - 16B */
    .key = {0x4e, 0x14, 0x96, 0x96, 0xfc, 0x16, 0x41, 0xca, 0x3b, 0x8d, 0x0b, 0x5a, 0xaf, 0x5f, 0xee, 0xba},
    /* mac - 6B MSB */
    .mac = {0xfc, 0x42, 0x65, 0x49, 0x1e, 0x2e}, // *MUST Changed for multi device
    /* crc - 2B */
    .crc = 0x0000,
};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void genie_triple_init(void)
{
    // Read value from storage, here use const data for demo.

}

static void genie_prov_param_conf(ms_prov_param_t *param)
{
    genie_gen_uuid(param->dev_uuid, &genie_triple);

    DEBUG("Dev UUID:");
    debugHex(param->dev_uuid, 16);

    param->static_oob     = 0x01;
    param->uri_hash       = 0x0000;
    param->oob_info       = 0x00;
    param->pub_key_oob    = 0x00;
    param->out_oob_size   = 0x00;
    param->in_oob_size    = 0x00;
    param->out_oob_action = 0x00;
    param->in_oob_action  = 0x00;
    param->info           = 0x00;
}

static void genie_oob_auth_conf(uint8_t *p_oob)
{
    genie_calc_auth(p_oob, &genie_triple);

    DEBUG("OOB Data:");
    debugHex(p_oob, 16);
}

void app_mesh_create(void)
{
    m_lid_t mdl_lid;

    genie_triple_init();

    // Config SSID
    ms_set_ssid(GENIE_MESH_CID, 0/*pid*/, 0/*vid*/, 0/*loc*/);

    // Register Models
    mdl_lid = mm_gens_oo_register(1, true);
    DEBUG("mm_gens_oo, mdl_lid=%d", mdl_lid);

    mdl_lid = mm_genc_oo_register();
    DEBUG("mm_genc_oo, mdl_lid=%d", mdl_lid);

    // Start Mesh Bearer
    mesh_enable();
}


/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief  MESH Service Message Handler
 ****************************************************************************************
 */

APP_MSG_HANDLER(ms_state_ind)
{
    DEBUG("ms_state_ind(op:%d,v1:%d,v2:%d)", param->ind_code, param->state, param->status);

    switch (param->ind_code)
    {
        case MS_PROV_STATE_IND: 
        {
            DEBUG("    PROV_STATE:%d", param->state);
        } break;

        case MS_PROXY_ADV_UPD_IND:
        {
            DEBUG("    PROXY_ADV(sta:%d,rsn:%d)", param->state, param->status);
        } break;

        default:
        {
            // todo more...
        } break;
    }
}

APP_MSG_HANDLER(ms_fndh_fault_ind)
{
    DEBUG("ms_fault_ind(op:%d,tst:%d,cid:0x%X)", param->ind_code, param->test_id, param->comp_id);

    if ( (param->ind_code == MS_FAULT_GET_REQ_IND)
        || (param->ind_code == MS_FAULT_TEST_REQ_IND) )
    {
        //ms_fndh_fault_rsp(true, param->comp_id, param->test_id, , );
    }
}

APP_MSG_HANDLER(ms_fndh_period_ind)
{
    DEBUG("ms_period_ind(ms:%d,fault_ms:%d)", param->period_ms, param->period_fault_ms);

}

APP_MSG_HANDLER(ms_prov_data_req_ind)
{
    DEBUG("ms_prov_data_req(op:%d,val1:0x%02x,val2:0x%04X)", param->req_code, param->value1, param->value2);

    switch (param->req_code)
    {
        case MS_PROV_PARAM_DATA_REQ_IND:
        {
            ms_prov_param_t param;

            genie_prov_param_conf(&param);
            // Provide provisioning parameters
            ms_prov_param_rsp(&param);
        } break;

        case MS_PROV_COMPO_DATA_REQ_IND:
        {
            // only occur when nb_compo_data_page > 1
            //ms_compo_data_rsp(param->value1/*page*/, /*length*/, /*p_data*/);
        } break;

        default: //MS_PROV_AUTH_DATA_REQ_IND
        {
            uint8_t oob_auth[16];

            genie_oob_auth_conf(oob_auth);
            // Provide authentication data
            ms_prov_auth_rsp(true, 16, oob_auth);
        } break;
    }
}

#if (BLE_MESH_LPN)
APP_MSG_HANDLER(ms_lpn_offer_ind)
{
    DEBUG("ms_lpn_offer_ind(addr:0x%X,cnt:%d,offer:0x%X)", param->frd_addr, param->frd_cnt, param->wd_offer);

}
#endif //(BLE_MESH_LPN)

APP_MSG_HANDLER(ms_conf_load_ind)
{
    DEBUG("ms_conf_load_ind(typ:0x%X,idx:%d,sta:0x%X)", param->cfg_type, param->index, param->status);
    
}

APP_MSG_HANDLER(ms_conf_update_ind)
{
    DEBUG("ms_conf_update_ind(typ:0x%X,val1:%d,val2:0x%X)", param->upd_type, param->value1, param->value2);

}

/**
 ****************************************************************************************
 * @brief  MESH Model Message Handler
 ****************************************************************************************
 */

APP_MSG_HANDLER(mm_ind)
{
    DEBUG("mm_ind(op:%d)", param->ind_code);

    switch (param->ind_code)
    {
        case MM_SRV_STATE_UPD_IND:
        {
            struct mm_srv_state_upd_ind *p_ind = (struct mm_srv_state_upd_ind *)param;

            DEBUG("    SRV_STATE_UPD(elt:%d,sid:%d,state:0x%X,ttms:%d)", 
                    p_ind->elmt_idx, p_ind->state_id, p_ind->state, p_ind->trans_time_ms);

            if (p_ind->state_id == MM_STATE_GEN_ONOFF)
            {
                if (p_ind->state)
                {
                    GPIO_DAT_CLR(GPIO08); // LED0 on
                }
                else
                {
                    GPIO_DAT_SET(GPIO08); // LED0 off
                }
            }
        } break;

        case MM_CLI_STATE_IND:
        {
            struct mm_cli_state_ind *p_ind = (struct mm_cli_state_ind *)param;

            DEBUG("    CLI_STATE(src:%04X,sid:%d,sv1:%d,sv2:%d,rms:%d)", 
                    p_ind->src, p_ind->state_id, p_ind->state_1, p_ind->state_2, p_ind->rem_time_ms);

            if (p_ind->state_id == MM_STATE_GEN_ONOFF)
            {
                if (p_ind->state_1)
                {
                    GPIO_DAT_CLR(GPIO09); // LED1 on
                }
                else
                {
                    GPIO_DAT_SET(GPIO09); // LED1 off
                }
            }
        } break;

        default:
        {
            // todo more...
        } break;
    }
}

APP_MSG_HANDLER(mm_req_ind)
{
    DEBUG("mm_req_ind(op:%d)", param->req_code);
}


/**
 ****************************************************************************************
 * @brief SubTask Handler of MESH Message.
 ****************************************************************************************
 */
APP_SUBTASK_HANDLER(mesh_msg)
{
    switch (msgid)
    {
        /**** Mesh Service Message ****/
        case MESH_STATE_IND:
        {
            APP_MSG_FUNCTION(ms_state_ind);
        } break;

        case MESH_FNDH_FAULT_IND:
        {
            APP_MSG_FUNCTION(ms_fndh_fault_ind);
        } break;

        case MESH_FNDH_PERIOD_IND:
        {
            APP_MSG_FUNCTION(ms_fndh_period_ind);
        } break;

        case MESH_PROV_DATA_REQ_IND:
        {
            APP_MSG_FUNCTION(ms_prov_data_req_ind);
        } break;

        #if (BLE_MESH_LPN)
        case MESH_LPN_OFFER_IND:
        {
            APP_MSG_FUNCTION(ms_lpn_offer_ind);
        } break;
        #endif //(BLE_MESH_LPN)

        case MESH_CONF_LOAD_IND:
        {
            APP_MSG_FUNCTION(ms_conf_load_ind);
        } break;

        case MESH_CONF_UPDATE_IND:
        {
            APP_MSG_FUNCTION(ms_conf_update_ind);
        } break;

        /**** Mesh Model Message   ****/
        case MESH_MDL_IND:
        {
            APP_MSG_FUNCTION(mm_ind);
        } break;

        case MESH_MDL_REQ_IND:
        {
            APP_MSG_FUNCTION(mm_req_ind);
        } break;

        default:
        {
            uint16_t length = ke_param2msg(param)->param_len;

            DEBUG("Unknow MsgId:0x%X, MsgLen:%d", msgid, length);
            debugHex((uint8_t *)param, length);
        } break;
    }

    return (MSG_STATUS_FREE);
}

#endif //(PRF_MESH)
