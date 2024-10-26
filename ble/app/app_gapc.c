/**
 ****************************************************************************************
 *
 * @file app_gapc.c
 *
 * @brief App SubTask of GAP Controller Message - Example
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "bledef.h"
#include "app.h"

#if (DBG_GAPC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat,len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

APP_MSG_HANDLER(gapc_cmp_evt)
{
    // operation @see enum gapc_operation
    // Command complete, may ignore
    DEBUG("CMP_EVT(op:%d,sta:0x%x)", param->operation, param->status);
}

APP_MSG_HANDLER(gapc_connection_req_ind)
{
    #if (DBG_GAPC)
    uint8_t conidx = TASK_IDX(src_id);
    #endif

    DEBUG("gapc_connection_req_ind(cid:%d,chdl:%d,conn[intv:%d,late:%d,to:%d],acc:%d,caddr:%d)", conidx, param->conhdl,
          param->con_interval, param->con_latency, param->sup_to, param->clk_accuracy, param->peer_addr_type);
    debugHex(param->peer_addr.addr, GAP_BD_ADDR_LEN);

    // Indicate Connect_Req_Pkt be send, need wait to sync (try 6 times of conn_event).
    // If synced, goto gapc_connection_ind. Otherwise, goto gapc_disconnect_ind(reason=0x3E)
    //app_conn_fsm(BLE_CONNECTING, conidx, param);
}

APP_MSG_HANDLER(gapc_connection_ind)
{
    uint8_t conidx = TASK_IDX(src_id);

    DEBUG("gapc_connection_ind(cid:%d,chdl:%d,role:%d)", conidx, param->conhdl, param->role);

    // Connection established, update from v1.3
    app_conn_fsm(BLE_CONNECTED, conidx, param);
}

APP_MSG_HANDLER(gapc_disconnect_ind)
{
    uint8_t conidx = TASK_IDX(src_id);

    DEBUG("gapc_disconnect_ind(cid:%d,hdl:%d,reason:0x%X)", conidx, param->conhdl, param->reason);

    app_conn_fsm(BLE_DISCONNECTED, conidx, param);
}

APP_MSG_HANDLER(gapc_param_update_req_ind)
{
    uint8_t conidx = TASK_IDX(src_id);

    DEBUG("param_update(cid:%d,invM:%d,invI:%d,late:%d,timo:%d)", conidx,
          param->intv_max, param->intv_min, param->latency, param->time_out);

    // Connection param accept or reject
    gapc_param_update_rsp(conidx, true, 0x2, 0x4);
}

APP_MSG_HANDLER(gapc_param_updated_ind)
{
    DEBUG("param_updated_ind(cid:%d,intv:%d,late:%d,timo:%d)", TASK_IDX(src_id),
          param->con_interval, param->con_latency, param->sup_to);

    // Current param, may update to slaves

}

APP_MSG_HANDLER(gapc_le_pkt_size_ind)
{
    DEBUG("le_pkt_size_ind(cid:%d,txB:%d,txT:%d,rxB:%d,rxT:%d)", TASK_IDX(src_id),
          param->max_tx_octets, param->max_tx_time, param->max_rx_octets, param->max_rx_time);
}

#if (BLE_EN_SMP)
APP_MSG_HANDLER(gapc_bond_req_ind)
{
    uint8_t conidx = TASK_IDX(src_id);

    DEBUG("bond_req_ind(req:%d)", param->request);

    switch (param->request)
    {
        case (GAPC_PAIRING_REQ):
        {
            struct gapc_pairing feat;

            // Retrieve pairing feature to response
            app_pairing_get(&feat);
            gapc_smp_pairing_rsp(conidx, &feat);

            DEBUG("PAIRING_REQ(auth:0x%X)", param->data.auth_req);
            debugHex((uint8_t *)&feat, sizeof(struct gapc_pairing));
        } break;

        case (GAPC_LTK_EXCH):
        {
            struct gapc_ltk ltk;

            // Generate all rand values to exchange
            app_ltk_gen(conidx, &ltk);
            gapc_smp_pairing_ltk_exch(conidx, &ltk);

            DEBUG("LTK_EXCH(size:%d)", param->data.key_size);
            debugHex((uint8_t *)&ltk, sizeof(struct gapc_ltk));
        } break;

        case (GAPC_IRK_EXCH):
        {
            DEBUG("IRK_EXCH");

            // Load IRK
            //memcpy(&(data.irk.irk), gapm_get_irk(), GAP_KEY_LEN);

            // load device address
            //memcpy(&(data.irk.addr.addr), gapm_get_bdaddr(), GAP_BD_ADDR_LEN);
            //data.irk.addr.addr_type = (data.irk.addr.addr.addr[5] & 0xC0) ? ADDR_RAND : ADDR_PUBLIC;
        } break;

        case (GAPC_TK_EXCH):
        {
            DEBUG("TK_EXCH(type:%d)", param->data.tk_type);

            if (param->data.tk_type == GAP_TK_OOB)
            {
                DEBUG("TK_OOB>>>");
                return;
            }
            else if (param->data.tk_type == GAP_TK_DISPLAY)
            {
                struct gap_sec_key tk;
                // Generate a PIN Code- (Between 100000 and 999999)
                uint32_t pin_code = (100000 + (rand_word() % 900000));
                // Set the TK value
                memset(&tk.key, 0, GAP_KEY_LEN);
                write32p(&tk.key, pin_code);

                DEBUG("TK_DISPLAY:%06d", pin_code);
                gapc_smp_pairing_tk_exch(conidx, true, &tk);
            }
            else //if (param->data.tk_type == GAP_TK_KEY_ENTRY)
            {
                DEBUG("TK_KEY_ENTRY>>>");
                return;
            }
        } break;

        case (GAPC_CSRK_EXCH):
        {
            struct gap_sec_key csrk;
            DEBUG("CSRK_EXCH");
            memset(&csrk.key, 0, GAP_KEY_LEN);
            gapc_smp_pairing_csrk_exch(conidx, &csrk);
        } break;

        #if (SEC_CON_ENB)
        case (GAPC_OOB_EXCH):
        {
            DEBUG("OOB_EXCH(conf,rand)>>>");
            debugHex(param->data.oob_data.conf, GAP_KEY_LEN);
            debugHex(param->data.oob_data.rand, GAP_KEY_LEN);

            return;
        } //break;

        case (GAPC_NC_EXCH):
        {
            DEBUG("NC_EXCH(%06d)", read32p(param->data.nc_data.value));
            gapc_smp_pairing_nc_exch(conidx, true);
        } break;
        #endif //(SEC_CON_ENB)

        default:
        {
            // not happen
        } break;
    }
}

APP_MSG_HANDLER(gapc_bond_ind)
{
    uint8_t conidx = TASK_IDX(src_id);
    DEBUG("bond_ind(info:%d)", param->info);

    switch (param->info)
    {
        case (GAPC_PAIRING_SUCCEED):
        {
            DEBUG("PAIRING_SUCCEED(auth:%X,ltk:%d)", param->data.auth.info, param->data.auth.ltk_present);
            // Update the bonding status
            app_conn_fsm(BLE_BONDED, conidx, param);
        } break;

        case (GAPC_PAIRING_FAILED):
        {
            DEBUG("PAIRING_FAILED(reason:0x%X)", param->data.reason);
            //gapc_smp_security_req(conidx, GAP_AUTH_REQ_NO_MITM_NO_BOND);
        } break;

        case (GAPC_REPEATED_ATTEMPT):
        {
            DEBUG("REPEATED_ATTEMPT");
            gapc_disconnect(conidx);
        } break;

        case (GAPC_IRK_EXCH):
        {
            DEBUG("IRK_EXCH(irk,addr)");
            debugHex(param->data.irk.irk.key, GAP_KEY_LEN);
            debugHex(param->data.irk.addr.addr.addr, GAP_BD_ADDR_LEN);
            // Store peer identity
        } break;

        // In Secure Connections we get BOND_IND with SMPC calculated LTK
        case (GAPC_LTK_EXCH) :
        {
            DEBUG("Bond LTK_EXCH(size:%d)", param->data.ltk.key_size);
            debugHex(param->data.ltk.ltk.key, sizeof(struct gapc_ltk));

            // Store peer LTK
            // if ((gapc_auth_get(conidx) & GAP_AUTH_SEC_CON) || (gapc_get_role(conidx) == ROLE_MASTER))
            // {
            //     app_ltk_save(conidx, &param->data.ltk);
            // }
        } break;

        default:
            break;
    }
}

APP_MSG_HANDLER(gapc_encrypt_req_ind)
{
    uint8_t conidx = TASK_IDX(src_id);

    DEBUG("encrypt_req_ind(ediv:0x%X,rand)", param->ediv);
    debugHex(param->rand_nb.nb, GAP_RAND_NB_LEN);

    // Check if the provided EDIV and Rand Nb values match with the stored values
    struct gap_sec_key *ltk = (struct gap_sec_key *)app_ltk_find(param->ediv, param->rand_nb.nb);
    if (ltk != NULL)
    {
        DEBUG("->Find Same LTK");
        gapc_smp_encrypt_cfm(conidx, GAP_KEY_LEN, ltk);
    }
    else
    {
        // bonded with another device, disconnect the link
        DEBUG("->Find Diff LTK");
        gapc_smp_encrypt_cfm(conidx, 0, NULL);
        //gapc_smp_security_req(conidx, GAP_AUTH_REQ_NO_MITM_BOND);
        gapc_disconnect(conidx);
    }
}

APP_MSG_HANDLER(gapc_encrypt_ind)
{
    uint8_t conidx = TASK_IDX(src_id);
    DEBUG("encryp_ind(auth:%d)", param->auth);

    // encryption/ re-encryption succeeded
    app_conn_fsm(BLE_ENCRYPTED, conidx, param);
}

APP_MSG_HANDLER(gapc_security_ind)
{
    // slave request security
    DEBUG("security_ind(auth:%d)", param->auth);
}
#endif //(BLE_EN_SMP)

/**
 ****************************************************************************************
 * @brief SubTask Handler of GAP controller Message.
 ****************************************************************************************
 */
APP_SUBTASK_HANDLER(gapc_msg)
{
    switch (msgid)
    {
        case GAPC_CMP_EVT:
        {
            APP_MSG_FUNCTION(gapc_cmp_evt);
        } break;

        case GAPC_CONNECTION_REQ_IND:
        {
            APP_MSG_FUNCTION(gapc_connection_req_ind);
        } break;

        case GAPC_CONNECTION_IND:
        {
            // Connection established, update from v1.3
            APP_MSG_FUNCTION(gapc_connection_ind);
        } break;

        case GAPC_DISCONNECT_IND:
        {
            APP_MSG_FUNCTION(gapc_disconnect_ind);
        } break;

        case GAPC_PARAM_UPDATE_REQ_IND:
        {
            APP_MSG_FUNCTION(gapc_param_update_req_ind);
        } break;

        case GAPC_PARAM_UPDATED_IND:
        {
            APP_MSG_FUNCTION(gapc_param_updated_ind);
        } break;

        #if (BLE_EN_SMP)
        case GAPC_BOND_REQ_IND:
        {
            APP_MSG_FUNCTION(gapc_bond_req_ind);
        } break;

        case GAPC_BOND_IND:
        {
            APP_MSG_FUNCTION(gapc_bond_ind);
        } break;

        case GAPC_ENCRYPT_REQ_IND:
        {
            APP_MSG_FUNCTION(gapc_encrypt_req_ind);
        } break;

        case GAPC_ENCRYPT_IND:
        {
            APP_MSG_FUNCTION(gapc_encrypt_ind);
        } break;

        case GAPC_SECURITY_IND:
        {
            APP_MSG_FUNCTION(gapc_security_ind);
        } break;
        #endif //(BLE_EN_SMP)

        case GAPC_LE_PKT_SIZE_IND:
        {
            APP_MSG_FUNCTION(gapc_le_pkt_size_ind);
        } break;

        default:
        {
            uint16_t length = ke_param2msg(param)->param_len;
            DEBUG("Unknow MsgId:0x%X, %d", msgid, length);
            debugHex((uint8_t *)param, length);
        } break;
    }

    return (MSG_STATUS_FREE);
}
