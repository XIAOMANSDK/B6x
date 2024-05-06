/**
 ****************************************************************************************
 *
 * @file mm_genc_oo.c
 *
 * @brief Mesh Model Generic OnOff Client Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_GENC_OO
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

/// Structure for Generic OnOff Client model environment
typedef struct mm_genc_oo_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
} mm_genc_oo_env_t;

/*
 * INTERNAL CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Inform Generic OnOff Client model about reception of a message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] p_route_env   Pointer to routing environment containing information about the
 * received message
 ****************************************************************************************
 */
__STATIC void mm_genc_oo_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                               mm_route_env_t *p_route_env)
{
    if (p_route_env->opcode == MM_MSG_GEN_OO_STATUS)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Current state
        uint8_t cur_state = *(p_data + MM_GEN_OO_STATUS_OO_POS);
        // Target state
        uint8_t tgt_state = 0;
        // Remaining time
        uint8_t rem_time = 0;

        // Read optional parameters if presents
        if (p_buf->data_len == MM_GEN_OO_STATUS_LEN)
        {
            tgt_state = *(p_data + MM_GEN_OO_STATUS_TGT_OO_POS);
            rem_time = *(p_data + MM_GEN_OO_STATUS_REM_TIME_POS);
        }

        // Inform the application about the received Generic OnOff state value
        mm_cli_state_ind_send(p_route_env->u_addr.src, MM_STATE_GEN_ONOFF, cur_state,
                              tgt_state, mm_get_trans_time_ms(rem_time));
    }
}

/**
 ****************************************************************************************
 * @brief Inform Generic OnOff Client model about a received opcode in order to check if the
 * model is authorized to handle the associated message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] opcode        Opcode value to be checked
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_oo_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if (opcode == MM_MSG_GEN_OO_STATUS)
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
 * @brief Send a Generic OnOff Get message to a given node's element
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] dst           Address of node's element to which message will be sent
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_oo_cb_get(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
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
        p_buf_env->opcode = MM_MSG_GEN_OO_GET;

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
 * @brief Initiate transition of Generic OnOff state value on a given node's element by
 * sending either a Generic OnOff Set or a Generic OnOff Set Unacknowledged message
 *
 * @param[in] p_env             Pointer to the environment allocated for the model
 * @param[in] dst               Address of node's element to which message will be sent
 * @param[in] state_1           Target Generic OnOff state value
 * @param[in] state_2           N\A
 * @param[in] trans_time_ms     Transition time in milliseconds
 * @param[in] delay_ms          Delay in milliseconds
 * @param[in] trans_info        Transition information (@see enum mm_trans_info)
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_oo_cb_trans(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                           uint32_t state_1, uint32_t state_2,
                                           uint32_t trans_time_ms, uint16_t delay_ms,
                                           uint16_t trans_info)
{
    uint8_t status = MESH_ERR_NO_ERROR;
    // Get message length
    uint8_t length = (GETB(trans_info, MM_TRANS_INFO_LONG) || trans_time_ms || delay_ms)
                                                    ? MM_GEN_OO_SET_LEN : MM_GEN_OO_SET_MIN_LEN;
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_set = mm_route_buf_alloc(length);

    if (p_buf_set)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf_set);
        // Get pointer to environment
        mm_route_env_t *p_buf_env = (mm_route_env_t *)&p_buf_set->env;
        // Generic OnOff state value
        uint8_t onoff = state_1;

        // Prepare environment
        p_buf_env->app_key_lid = app_key_lid;
        p_buf_env->u_addr.dst = dst;
        p_buf_env->info = 0;
        p_buf_env->mdl_lid = p_env->mdl_lid;
        p_buf_env->opcode = GETB(trans_info, MM_TRANS_INFO_ACK)
                                        ? MM_MSG_GEN_OO_SET : MM_MSG_GEN_OO_SET_UNACK;

        // Fill the message
        *(p_data + MM_GEN_OO_SET_OO_POS) = onoff;
        *(p_data + MM_GEN_OO_SET_TID_POS) = GETF(trans_info, MM_TRANS_INFO_TID);

        if (length == MM_GEN_OO_SET_LEN)
        {
            *(p_data + MM_GEN_OO_SET_TRANS_TIME_POS) = mm_get_trans_time(trans_time_ms);
            *(p_data + MM_GEN_OO_SET_DELAY_POS) = delay_ms / 5;
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

__STATIC mm_cli_cb_t mm_genc_oo_cb = 
{
    .cb_get = mm_genc_oo_cb_get,
    .cb_set = NULL,
    .cb_trans = mm_genc_oo_cb_trans,
};

uint8_t mm_genc_oo_register(void)
{
    // Register the model
    m_lid_t mdl_lid = ms_register_model(MM_ID_GENC_OO, 0, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID) //if (status == MESH_ERR_NO_ERROR)
    {
        // Inform the Model State Manager about registered model
        mm_genc_oo_env_t *p_env_oo = (mm_genc_oo_env_t *)mm_state_register(0, MM_ID_GENC_OO, mdl_lid,
                                        MM_ROLE_CLI, sizeof(mm_genc_oo_env_t));

        if (p_env_oo)
        {
            // Get client-specific callback functions
            //mm_cli_cb_t *p_cb_cli = p_env_oo->env.cb.u.p_cb_cli;

            // Set internal callback functions
            p_env_oo->env.mdl_cb.cb_rx = mm_genc_oo_cb_rx;
            p_env_oo->env.mdl_cb.cb_opcode_check = mm_genc_oo_cb_opcode_check;
            //p_cb_cli->cb_get = mm_genc_oo_cb_get;
            //p_cb_cli->cb_trans = mm_genc_oo_cb_trans;
            p_env_oo->env.mdl_cb.p_cb_cli = &mm_genc_oo_cb;

            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENC_OO, 0, mdl_lid);
        }
    }

    return (mdl_lid);
}

/// @} end of group
