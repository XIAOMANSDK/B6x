/**
 ****************************************************************************************
 *
 * @file mm_genc_lvl.c
 *
 * @brief Mesh Model Generic Level Client Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_GENC_LVL
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "mm_itf.h"
#include "mm_genc.h"


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Structure for Generic Level Client model environment
typedef struct mm_genc_lvl_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
} mm_genc_lvl_env_t;

/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Compute length of set message to prepare:
 *          - Generic Level Set/Set Unacknowledged
 *          - Generic Delta Set/Set Unacknowledged
 *          - Generic Move Set/Set Unacknowledged
 *
 * @param[in] trans_type        Transition type (@see enum mm_trans_type)
 * @param[in] set_long          True if optional parameters are present, else false
 *
 * @return Length of the set message
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_lvl_get_length(uint8_t trans_type, bool set_long)
{
    uint8_t length;

    if (trans_type == MM_TRANS_TYPE_CLASSIC)
    {
        // Generic Level Set/Set Unacknowledged
        length = (set_long) ? MM_GEN_LVL_SET_LEN : MM_GEN_LVL_SET_MIN_LEN;
    }
    else if (trans_type == MM_TRANS_TYPE_DELTA)
    {
        // Generic Delta Set/Set Unacknowledged
        length = (set_long) ? MM_GEN_LVL_SET_DELTA_LEN : MM_GEN_LVL_SET_DELTA_MIN_LEN;
    }
    else // (trans_type == MM_TRANS_TYPE_MOVE)
    {
        // Generic Move Set/Set Unacknowledged
        length = (set_long) ? MM_GEN_LVL_SET_MOVE_LEN : MM_GEN_LVL_SET_MOVE_MIN_LEN;
    }

    return (length);
}

/*
 * INTERNAL CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Inform Generic Level Client model about reception of a message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] p_route_env   Pointer to routing environment containing information about the
 * received message
 ****************************************************************************************
 */
__STATIC void mm_genc_lvl_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                                mm_route_env_t *p_route_env)
{
    if (p_route_env->opcode == MM_MSG_GEN_LVL_STATUS)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Current state
        uint16_t cur_state = read16p(p_data + MM_GEN_LVL_STATUS_LVL_POS);
        // Target state
        uint16_t tgt_state;
        // Remaining time
        uint8_t rem_time;

        // Read optional parameters if presents
        if (p_buf->data_len == MM_GEN_LVL_STATUS_LEN)
        {
            tgt_state = read16p(p_data + MM_GEN_LVL_STATUS_TGT_LVL_POS);
            rem_time = *(p_data + MM_GEN_LVL_STATUS_REM_TIME_POS);
        }
        else
        {
            tgt_state = 0;
            rem_time = 0;
        }

        // Inform the application about the received Generic Level state value
        mm_cli_state_ind_send(p_route_env->u_addr.src, MM_STATE_GEN_LVL, cur_state,
                              tgt_state, mm_get_trans_time_ms(rem_time));
    }
}

/**
 ****************************************************************************************
 * @brief Inform Generic Level Client model about a received opcode in order to check if the
 * model is authorized to handle the associated message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] opcode        Opcode value to be checked
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_lvl_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if (opcode == MM_MSG_GEN_LVL_STATUS)
    {
        status = MESH_ERR_NO_ERROR;
    }
    else
    {
        status = MESH_ERR_MDL_INVALID_OPCODE;
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Send a Generic Level Get message to a given node's element
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] dst           Address of node's element to which message will be sent
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_lvl_cb_get(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                     uint16_t get_info)
{
    uint8_t status = MESH_ERR_NO_ERROR;
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_get = mm_route_buf_alloc(0);

    if (p_buf_get)
    {
        // Get pointer to environment
        mm_route_env_t *p_buf_env = (mm_route_env_t *)&p_buf_get->env;

        // Prepare environment
        p_buf_env->app_key_lid = app_key_lid;
        p_buf_env->u_addr.dst = dst;
        p_buf_env->info = 0;
        p_buf_env->mdl_lid = p_env->mdl_lid;
        p_buf_env->opcode = MM_MSG_GEN_LVL_GET;

        // Send the message
        mm_route_send(p_buf_get);
    }
    else
    {
        status = MESH_ERR_INSUFFICIENT_RESOURCES;
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Initiate transition of Generic Level state value on a given node's element by
 * sending either a Generic Level Set or a Generic Level Set Unacknowledged message
 *
 * @param[in] p_env             Pointer to the environment allocated for the model
 * @param[in] dst               Address of node's element to which message will be sent
 * @param[in] state_1           Target Generic Level state value
 * @param[in] state_2           N\A
 * @param[in] trans_time_ms     Transition time in milliseconds
 * @param[in] delay_ms          Delay in milliseconds
 * @param[in] trans_info        Transition information (@see enum mm_trans_info)
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_lvl_cb_trans(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                            uint32_t state_1, uint32_t state_2,
                                            uint32_t trans_time_ms, uint16_t delay_ms,
                                            uint16_t trans_info)
{
    uint8_t status = MESH_ERR_NO_ERROR;
    // Transition type
    uint8_t trans_type = GETF(trans_info, MM_TRANS_INFO_TYPE);
    // Long message or not
    bool long_set = (GETB(trans_info, MM_TRANS_INFO_LONG) || trans_time_ms || delay_ms);
    // Get message length
    uint8_t length = mm_genc_lvl_get_length(trans_type, long_set);
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_set = mm_route_buf_alloc(length);

    if (p_buf_set)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf_set);
        // Get pointer to environment
        mm_route_env_t *p_buf_env = (mm_route_env_t *)&p_buf_set->env;

        // Prepare environment
        p_buf_env->app_key_lid = 6;
        p_buf_env->u_addr.dst = dst;
        p_buf_env->info = 0;
        p_buf_env->mdl_lid = p_env->mdl_lid;

        if (trans_type == MM_TRANS_TYPE_CLASSIC)
        {
            p_buf_env->opcode = GETB(trans_info, MM_TRANS_INFO_ACK)
                                            ? MM_MSG_GEN_LVL_SET : MM_MSG_GEN_LVL_SET_UNACK;

            // Fill the message
            write16p(p_data + MM_GEN_LVL_SET_LVL_POS, state_1);
            *(p_data + MM_GEN_LVL_SET_TID_POS) = GETF(trans_info, MM_TRANS_INFO_TID);
            p_data += MM_GEN_LVL_SET_TRANS_TIME_POS;
        }
        else if (trans_type == MM_TRANS_TYPE_DELTA)
        {
            p_buf_env->opcode = GETB(trans_info, MM_TRANS_INFO_ACK)
                                            ? MM_MSG_GEN_DELTA_SET : MM_MSG_GEN_DELTA_SET_UNACK;

            // Fill the message
            write32p(p_data + MM_GEN_LVL_SET_DELTA_LVL_POS, state_1);
            *(p_data + MM_GEN_LVL_SET_DELTA_TID_POS) = GETF(trans_info, MM_TRANS_INFO_TID);
            p_data += MM_GEN_LVL_SET_DELTA_TRANS_TIME_POS;
        }
        else
        {
            p_buf_env->opcode = GETB(trans_info, MM_TRANS_INFO_ACK)
                                            ? MM_MSG_GEN_MOVE_SET : MM_MSG_GEN_MOVE_SET_UNACK;

            // Fill the message
            write16p(p_data + MM_GEN_LVL_SET_MOVE_DELTA_LVL_POS, state_1);
            *(p_data + MM_GEN_LVL_SET_MOVE_TID_POS) = GETF(trans_info, MM_TRANS_INFO_TID);
            p_data += MM_GEN_LVL_SET_MOVE_TRANS_TIME_POS;
        }

        if (long_set)
        {
            // Set transition time and delay values
            *p_data = mm_get_trans_time(trans_time_ms);
            *(p_data + 1) = delay_ms / 5;
        }

        // Send the message
        mm_route_send(p_buf_set);
    }
    else
    {
        status = MESH_ERR_INSUFFICIENT_RESOURCES;
    }

    return (status);
}


/*
 * GLOBAL FUNCTIONS
 ****************************************************************************************
 */

__STATIC mm_cli_cb_t mm_genc_lvl_cb = 
{
    .cb_get = mm_genc_lvl_cb_get,
    .cb_set = NULL,
    .cb_trans = mm_genc_lvl_cb_trans,
};

uint8_t mm_genc_lvl_register(void)
{
    // Register the model
    m_lid_t mdl_lid = ms_register_model(MM_ID_GENC_LVL, 0, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID) //if (status == MESH_ERR_NO_ERROR)
    {
        // Inform the Model State Manager about registered model
        mm_genc_lvl_env_t *p_env_lvl = (mm_genc_lvl_env_t *)mm_state_register(0, MM_ID_GENC_LVL, mdl_lid,
                                        MM_ROLE_CLI, sizeof(mm_genc_lvl_env_t));

        if (p_env_lvl)
        {
            // Get client-specific callback functions
            //mm_cli_cb_t *p_cb_cli = p_env_lvl->env.cb.u.p_cb_cli;

            // Set internal callback functions
            p_env_lvl->env.mdl_cb.cb_rx = mm_genc_lvl_cb_rx;
            p_env_lvl->env.mdl_cb.cb_opcode_check = mm_genc_lvl_cb_opcode_check;
            //p_cb_cli->cb_get = mm_genc_lvl_cb_get;
            //p_cb_cli->cb_trans = mm_genc_lvl_cb_trans;
            p_env_lvl->env.mdl_cb.p_cb_cli = &mm_genc_lvl_cb;

            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENC_LVL, 0, mdl_lid);
        }
    }

    return (mdl_lid);
}

/// @} end of group
