/**
 ****************************************************************************************
 * @file mm_gens_oo.c
 *
 * @brief Mesh Model Generic OnOff Server Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_GENS_OO
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "mm_itf.h"
#include "mm_gens.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Validity of information provided to the Replay Manager
#define MM_GENS_OO_REPLAY_MS               (6000)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Structure for Generic OnOff Server model environment
typedef struct mm_gens_oo_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
    /// Timer for sending of publications
    mesh_timer_t tmr_publi;
    /// Publication period in milliseconds
    uint32_t publi_period_ms;

    /// Environment for replay protection mechanism
    mm_replay_env_t replay_env;

    /// Current OnOff state value
    uint8_t onoff;
    /// Target OnOff state value
    uint8_t tgt_onoff;

    /// Address to which a Generic OnOff Status message must be sent (unassigned address if no
    /// status must be sent)
    uint8_t status_dst_addr;
    /// Application key local index to be used for transmission of Generic OnOff Status message
    m_lid_t status_app_key_lid;
    /// Relaying of sent Generic OnOff Status authorized
    bool status_relay;
} mm_gens_oo_env_t;

/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Prepare and send a Generic OnOff Status message
 *
 * @param[in] p_env_oo          Pointer to Generic OnOff Server model environment
 * @param[in] publish           True if status is a publication, false if state is a response
 ****************************************************************************************
 */
__STATIC void mm_gens_oo_send_status(mm_gens_oo_env_t *p_env_oo, mm_route_env_t *p_route_env,
                                     bool publish)
{
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_status;
    // Remaining time
    uint8_t rem_time;
    // Transition type
    uint8_t trans_type;
    // Data length
    uint8_t data_length;

    // Check if a transition has been started
    mm_bind_get_trans_info(p_env_oo->env.grp_lid, &trans_type, &rem_time);

    // Deduce deduce data length
    data_length = (trans_type != MM_TRANS_TYPE_NONE) ? MM_GEN_OO_STATUS_LEN : MM_GEN_OO_STATUS_MIN_LEN;

    p_buf_status = mm_route_buf_alloc(data_length);
    if (p_buf_status)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf_status);
        // Get pointer to environment
        mm_route_env_t *p_buf_env = (mm_route_env_t *)&p_buf_status->env;

        // Prepare environment
        if (p_route_env)
        {
            p_buf_env->app_key_lid = p_route_env->app_key_lid;
            p_buf_env->u_addr.dst = p_route_env->u_addr.src;
            p_buf_env->info = p_route_env->info;
        }
        else if (!publish)
        {
            p_buf_env->app_key_lid = p_env_oo->status_app_key_lid;
            p_buf_env->u_addr.dst = p_env_oo->status_dst_addr;
            SETB(p_buf_env->info, MM_ROUTE_INFO_RELAY, p_env_oo->status_relay);

            // Can accept new transitions
            p_env_oo->status_dst_addr = MESH_UNASSIGNED_ADDR;
        }

        SETB(p_buf_env->info, MM_ROUTE_INFO_RX, 0);
        SETB(p_buf_env->info, MM_ROUTE_INFO_PUBLISH, publish);
        p_buf_env->mdl_lid = p_env_oo->env.mdl_lid;
        p_buf_env->opcode = MM_MSG_GEN_OO_STATUS;

        // Fill the message
        *(p_data + MM_GEN_OO_STATUS_OO_POS) = p_env_oo->onoff;

        if (data_length == MM_GEN_OO_STATUS_LEN)
        {
            *(p_data + MM_GEN_OO_STATUS_TGT_OO_POS) = p_env_oo->tgt_onoff;
            *(p_data + MM_GEN_OO_STATUS_REM_TIME_POS) = rem_time;
        }

        // Send the message
        mm_route_send(p_buf_status);
    }
}

/**
 ****************************************************************************************
 * @brief Publish Generic OnOff state value if sending of publications is enabled
 *
 * @param[in] p_env_oo          Pointer to Generic OnOff Server model environment
 ****************************************************************************************
 */
__STATIC void mm_gens_oo_publish(mm_gens_oo_env_t *p_env_oo)
{
    // Check if sending of publication is enabled
    if (GETB(p_env_oo->env.info, MM_INFO_PUBLI))
    {
        mm_gens_oo_send_status(p_env_oo, NULL, true);
    }
}

/**
 ****************************************************************************************
 * @brief Check if a Generic OnOff Status message must be sent and send it if it
 * is the case
 *
 * @param[in] p_env_oo        Pointer to Generic OnOff Server model environment
 ****************************************************************************************
 */
__STATIC void mm_gens_oo_check_status_rsp(mm_gens_oo_env_t *p_env_oo)
{
    if (p_env_oo->status_dst_addr)
    {
        // Send a response to the node that has required the transition
        mm_gens_oo_send_status(p_env_oo, NULL, false);

        p_env_oo->status_dst_addr = MESH_UNASSIGNED_ADDR;
    }
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handler for Generic OnOff Set/Set Unacknowledged message
 *
 * @param[in] p_env     Pointer to environment of model for which message has been received
 * @param[in] p_data    Pointer to received message
 ****************************************************************************************
 */
__STATIC void mm_gens_oo_handler_set(mm_gens_oo_env_t *p_env_oo, mesh_buf_t *p_buf,
                                     mm_route_env_t *p_route_env)
{
    do
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Extract required onoff state value
        uint8_t onoff = *(p_data + MM_GEN_OO_SET_OO_POS);
        // Extract TID value
        uint8_t tid = *(p_data + MM_GEN_OO_SET_TID_POS);
        // Transition time
        uint8_t trans_time = MM_TRANS_TIME_UNKNOWN;
        // Delay
        uint8_t delay = 0;

        // Check received state value
        if (onoff > 1)
        {
            break;
        }

        // Extract and check optional parameters if present
        if (p_buf->data_len == MM_GEN_OO_SET_LEN)
        {
            trans_time = (uint16_t)(*(p_data + MM_GEN_OO_SET_TRANS_TIME_POS));

            // Check received value
            if (GETF(trans_time, MM_TRANS_TIME_STEP_NB) > MM_TRANS_TIME_STEPS_MAX)
            {
                // Drop the message
                break;
            }

            delay = *(p_data + MM_GEN_OO_SET_DELAY_POS);
        }

        // Check if received message is a retransmitted one, if state is modified and if
        // a new transition can be started now
        if ((p_env_oo->status_dst_addr != MESH_UNASSIGNED_ADDR)
                || mm_replay_is_retx(&p_env_oo->replay_env, p_route_env->u_addr.src, tid)
                || (onoff == p_env_oo->onoff))
        {
            if (p_route_env->opcode == MM_MSG_GEN_OO_SET)
            {
                // Send Generic OnOff status message
                mm_gens_oo_send_status(p_env_oo, p_route_env, false);
            }
            break;
        }

        // Keep information for transmission of status if needed
        if (p_route_env->opcode == MM_MSG_GEN_OO_SET)
        {
            p_env_oo->status_dst_addr = p_route_env->u_addr.src;
            p_env_oo->status_app_key_lid = p_route_env->app_key_lid;
            p_env_oo->status_relay = GETB(p_route_env->info, MM_ROUTE_INFO_RELAY);
        }

        if (GETB(p_env_oo->env.info, MM_INFO_MAIN))
        {
            // Update target state
            p_env_oo->tgt_onoff = onoff;

            // Inform the Binding Manager about new transition
            mm_bind_trans_new(p_env_oo->env.grp_lid, MM_TRANS_TYPE_CLASSIC,
                                 trans_time, delay);
        }
        else
        {
            // Inform the Binding Manager
            mm_bind_trans_req(p_env_oo->env.grp_lid, p_env_oo->env.mdl_lid,
                                 MM_TRANS_TYPE_CLASSIC, onoff, trans_time, delay);
        }
    } while (0);
}

/*
 * INTERNAL CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Callback function called when timer monitoring publication duration for
 * Generic OnOff Server model expires
 *
 * @param[in] p_env     Pointer to model environment for Generic OnOff Server model
 ****************************************************************************************
 */
__STATIC void mm_gens_oo_cb_tmr_publi(void *p_tmr)
{
    // Get allocated environment
    mm_gens_oo_env_t *p_env_oo = MESH_TMR2ENV(p_tmr, mm_gens_oo_env_t, tmr_publi);

    if (p_env_oo->publi_period_ms)
    {
        // Publish a Generic OnOff Status message
        mm_gens_oo_publish(p_env_oo);

        // Restart the timer
        mesh_timer_set(&p_env_oo->tmr_publi, p_env_oo->publi_period_ms);
    }
}

/**
 ****************************************************************************************
 * @brief Inform Generic OnOff Server model about a received message
 *
 * @param[in] p_env         Pointer to the environment allocated for the Generic OnOff
 * Server model
 * @param[in] p_buf         Pointer to the buffer containing the received message
 * @param[in] p_route_env   Pointer to structure containing reception parameters provided
 * by the Mesh Profile block
 ****************************************************************************************
 */
__STATIC void mm_gens_oo_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                               mm_route_env_t *p_route_env)
{
    // Get environment for Generic OnOff Server model
    mm_gens_oo_env_t *p_env_oo = (mm_gens_oo_env_t *)p_env;

    if (p_route_env->opcode != MM_MSG_GEN_OO_GET)
    {
        // Handle Generic OnOff Set/Set Unacknowledged message
        mm_gens_oo_handler_set(p_env_oo, p_buf, p_route_env);
    }
    else
    {
        // Send a Generic OnOff Status message
        mm_gens_oo_send_status(p_env_oo, p_route_env, false);
    }
}

/**
 ****************************************************************************************
 * @brief Check if a given opcode is handled by the Generic OnOff Server model
 *
 * @param[in] p_env         Pointer to the environment allocated for the Generic OnOff
 * Server model
 * @param[in] opcode        Opcode to check
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_gens_oo_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    #if 0
    uint8_t status;

    if ((opcode == MM_MSG_GEN_OO_GET)
            || (opcode == MM_MSG_GEN_OO_SET)
            || (opcode == MM_MSG_GEN_OO_SET_UNACK))
    {
        status = MESH_ERR_NO_ERROR;
    }
    else
    {
        status = MESH_ERR_MDL_INVALID_OPCODE;
    }
    #else
    uint8_t status = MESH_ERR_MDL_INVALID_OPCODE;

    if ((opcode & 0xFF) == 0x82)
    {
        uint8_t opcode_2b = (uint8_t)(opcode >> 8);

        if ((opcode_2b >= 0x01/*MM_MSG_GEN_OO_GET*/) 
            && (opcode_2b <= 0x03/*MM_MSG_GEN_OO_SET_UNACK*/))
        {
            status = MESH_ERR_NO_ERROR;
        }
    }
    #endif
    return (status);
}

/**
 ****************************************************************************************
 * @brief Set Generic OnOff state value
 *
 * @param[in] p_env         Pointer the the environment allocated for the Generic OnOff
 * Server model
 * @param[in] onoff         Generic OnOff state value
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_gens_oo_cb_set(mm_mdl_env_t *p_env, uint16_t state_id, uint32_t state)
{
    // Returned status
    uint8_t status = MESH_ERR_NO_ERROR;

    if (GETB(p_env->info, MM_INFO_MAIN))
    {
        // Generic OnOff state
        uint8_t onoff = state;

        if (onoff < 1)
        {
            // Get environment for the Generic OnOff Server model
            mm_gens_oo_env_t *p_env_oo = (mm_gens_oo_env_t *)p_env;

            // Set state value
            p_env_oo->onoff = onoff;
        }
        else
        {
            status = MESH_ERR_INVALID_PARAM;
        }
    }
    else
    {
        status = MESH_ERR_COMMAND_DISALLOWED;
    }

    return (status);
}

/*
 * CALLBACK FUNCTIONS FOR BINDING MANAGER
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Callback function provided to the Binding Manager so that Generic OnOff Server
 * model can be informed about group event
 *
 * @param[in] mdl_lid       Model Local Index
 * @param[in] event         Event
 * @param[in] info          Information (depends on the event type)
 ****************************************************************************************
 */
__STATIC void mm_gens_oo_cb_grp_event(m_lid_t mdl_lid, uint8_t event, uint8_t info)
{
    // Get environment for Generic OnOff Server model
    mm_gens_oo_env_t *p_env_oo = (mm_gens_oo_env_t *)mm_state_get_env(mdl_lid);

    switch (event)
    {
        case (MM_GRP_EVENT_TRANS_REJECTED):
        {
            // Send a response to the node that has required the transition
            mm_gens_oo_check_status_rsp(p_env_oo);
        } break;

        case (MM_GRP_EVENT_TRANS_DELAY_EXPIRED):
        {
            // Start the transition
            mm_bind_trans_start(p_env_oo->env.grp_lid);
        } break;

        case (MM_GRP_EVENT_TRANS_IMMEDIATE):
        {
            p_env_oo->onoff = p_env_oo->tgt_onoff;
        } // no break;

        case (MM_GRP_EVENT_TRANS_STARTED):
        {
            // Inform application about the update
            if (GETB(p_env_oo->env.info, MM_INFO_MAIN))
            {
                uint8_t trans_time = info;

                // Inform application about state update
                mm_srv_state_upd_ind_send(MM_STATE_GEN_ONOFF, p_env_oo->env.elmt_idx,
                                              p_env_oo->tgt_onoff,
                                              (trans_time) ? mm_get_trans_time_ms(trans_time) : 0);
            }

            // Send a response to the node that has required the transition
            mm_gens_oo_check_status_rsp(p_env_oo);

            // Send a publication
            mm_gens_oo_publish(p_env_oo);

            if (p_env_oo->tgt_onoff == 1)
            {
                p_env_oo->onoff = 1;
            }
        } break;

        case (MM_GRP_EVENT_TRANS_END):
        {
            // New state is the target state
            p_env_oo->onoff = p_env_oo->tgt_onoff;

            // Inform application about the update
            if (GETB(p_env_oo->env.info, MM_INFO_MAIN))
            {
                // Inform application about state update
                mm_srv_state_upd_ind_send(MM_STATE_GEN_ONOFF, p_env_oo->env.elmt_idx,
                                              p_env_oo->onoff, 0);
            }

            // Send a publication
            mm_gens_oo_publish(p_env_oo);
        } break;

        // case (MM_GRP_EVENT_TRANS_ABORTED):
        // case (MM_GRP_EVENT_GROUP_FULL):
        default:
        {
        } break;
    }
}

/**
 ****************************************************************************************
 * @brief Callback function provided to the Binding Manager so that Generic OnOff state
 * value can be set by main model of the group
 *
 * @param[in] mdl_lid       Model Local Index
 * @param[in] type          Type
 * @param[in] state         State value
 ****************************************************************************************
 */
__STATIC void mm_gens_oo_cb_set_state(m_lid_t mdl_lid, uint8_t type, uint32_t state)
{
    // Get environment for Generic Level Server model
    mm_gens_oo_env_t *p_env_oo = (mm_gens_oo_env_t *)mm_state_get_env(mdl_lid);

    // Sanity check
    ASSERT_INFO(!GETB(p_env_oo->env.info, MM_INFO_MAIN), mdl_lid, 0);

    if (type == MM_STATE_TYPE_CURRENT)
    {
        p_env_oo->onoff = (uint8_t)state;
    }
    else // (type == MM_STATE_TYPE_TARGET) || (type == MM_STATE_TYPE_TARGET_MOVE)
    {
        p_env_oo->tgt_onoff = (int8_t)state;
    }
}

/*
 * GLOBAL FUNCTIONS
 ****************************************************************************************
 */

m_lid_t mm_gens_oo_register(uint8_t elmt_idx, bool main)
{
    // Register the model
    m_lid_t mdl_lid = ms_register_model(MM_ID_GENS_OO, elmt_idx, MM_CFG_PUBLI_AUTH_BIT);

    // Check if model has been properly registered
    if (mdl_lid != MESH_INVALID_LID)
    {
        // Inform the Model State Manager about registered model
        mm_gens_oo_env_t *p_env_oo = (mm_gens_oo_env_t *)mm_state_register(elmt_idx, MM_ID_GENS_OO, mdl_lid,
                                      MM_ROLE_SRV_PUBLI, sizeof(mm_gens_oo_env_t));

        if (p_env_oo)
        {
            // Prepare timer for publication
            p_env_oo->tmr_publi.cb = mm_gens_oo_cb_tmr_publi;

            // Prepare environment for Replay Manager
            p_env_oo->replay_env.delay_ms = MM_GENS_OO_REPLAY_MS;

            // Set internal callback functions
            p_env_oo->env.mdl_cb.cb_rx = mm_gens_oo_cb_rx;
            p_env_oo->env.mdl_cb.cb_opcode_check = mm_gens_oo_cb_opcode_check;
            p_env_oo->env.mdl_cb.cb_srv_set = mm_gens_oo_cb_set;

            if (main)
            {
                // Create group
                m_lid_t grp_lid = mm_bind_add_group(0, elmt_idx, mdl_lid,
                                     mm_gens_oo_cb_grp_event, NULL);
            }

            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENS_OO, elmt_idx, mdl_lid);
        }
        else
        {
            // Allocate fail - MESH_ERR_STORAGE_FAILURE
        }
    }

    return (mdl_lid);
}

uint8_t mm_gens_oo_bind_group(m_lid_t grp_lid, m_lid_t oo_lid)
{
    return mm_bind_group_add_mdl(grp_lid, oo_lid, MM_ID_GENS_OO,
                             mm_gens_oo_cb_grp_event,
                             mm_gens_oo_cb_set_state);
}

/// @} end of group
