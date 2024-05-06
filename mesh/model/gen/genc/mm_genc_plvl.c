/**
 ****************************************************************************************
 *
 * @file mm_genc_plvl.c
 *
 * @brief Mesh Model Generic Power Level Client Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_GENC_PLVL
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

/// Structure for Generic Power Level Client model environment
typedef struct mm_genc_plvl_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
} mm_genc_plvl_env_t;

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handler for Generic Power Level Status message
 *
 * @param[in] p_buf     Pointer to buffer containing the received message
 * @param[in] src       Source address of the message
 ****************************************************************************************
 */
__STATIC void mm_genc_plvl_handler_status(mesh_buf_t *p_buf, uint16_t src)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Generic Power Actual state value
    uint16_t power_actual = read16p(p_data + MM_GEN_PLVL_STATUS_PRES_POWER_POS);
    // Targeted Generic Power Actual state value
    uint16_t power_tgt;
    // Remaining time
    uint32_t rem_time;

    // Check if optional parameters are provided
    if (p_buf->data_len == MM_GEN_PLVL_STATUS_LEN)
    {
        power_tgt = read16p(p_data + MM_GEN_PLVL_STATUS_TGT_POWER_POS);
        rem_time = *(p_data + MM_GEN_PLVL_STATUS_REM_TIME_POS);
    }
    else
    {
        power_tgt = 0;
        rem_time = 0;
    }

    // Inform the application about the received Generic Power Last state value
    mm_cli_state_ind_send(src, MM_STATE_GEN_POWER_ACTUAL, power_actual, power_tgt,
                          mm_get_trans_time_ms(rem_time));
}

/**
 ****************************************************************************************
 * @brief Handler for Generic Power Last Status message
 *
 * @param[in] p_buf     Pointer to buffer containing the received message
 * @param[in] src       Source address of the message
 ****************************************************************************************
 */
__STATIC void mm_genc_plvl_handler_status_last(mesh_buf_t *p_buf, uint16_t src)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Generic Power Last state value
    uint16_t power_last = read16p(p_data + MM_GEN_PLVL_LAST_STATUS_POWER_POS);

    // Inform the application about the received Generic Power Last state value
    mm_cli_state_ind_send(src, MM_STATE_GEN_POWER_LAST, power_last, 0, 0);
}

/**
 ****************************************************************************************
 * @brief Handler for Generic Power Default Status message
 *
 * @param[in] p_buf     Pointer to buffer containing the received message
 * @param[in] src       Source address of the message
 ****************************************************************************************
 */
__STATIC void mm_genc_plvl_handler_status_dflt(mesh_buf_t *p_buf, uint16_t src)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Generic Power Default state value
    uint16_t power_dflt = read16p(p_data + MM_GEN_PLVL_DFLT_SET_POWER_POS);

    // Inform the application about the received Generic Power Default state value
    mm_cli_state_ind_send(src, MM_STATE_GEN_POWER_DFLT, power_dflt, 0, 0);
}

/**
 ****************************************************************************************
 * @brief Handler for Generic Power Range Status message
 *
 * @param[in] p_buf     Pointer to buffer containing the received message
 * @param[in] src       Source address of the message
 ****************************************************************************************
 */
__STATIC void mm_genc_plvl_handler_status_range(mesh_buf_t *p_buf, uint16_t src)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Generic Power Range state value (minimum)
    uint16_t power_min = read16p(p_data + MM_GEN_PLVL_RANGE_STATUS_MIN_POS);
    // Generic Power Range state value (maximum)
    uint16_t power_max = read16p(p_data + MM_GEN_PLVL_RANGE_STATUS_MAX_POS);

    // Inform the application about the received Generic Power Default state value
    mm_cli_state_ind_send(src, MM_STATE_GEN_POWER_RANGE, power_min, power_max, 0);
}

/*
 * INTERNAL CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Inform Generic Power Level Client model about reception of a message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] p_route_env   Pointer to routing environment containing information about the
 * received message
 ****************************************************************************************
 */
__STATIC void mm_genc_plvl_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                                 mm_route_env_t *p_route_env)
{
    // Call the appropriate handler for the received message
    switch (p_route_env->opcode)
    {
        case (MM_MSG_GEN_PLVL_STATUS):
        {
            mm_genc_plvl_handler_status(p_buf, p_route_env->u_addr.src);
        } break;

        case (MM_MSG_GEN_PLAST_STATUS):
        {
            mm_genc_plvl_handler_status_last(p_buf, p_route_env->u_addr.src);
        } break;

        case (MM_MSG_GEN_PDFLT_STATUS):
        {
            mm_genc_plvl_handler_status_dflt(p_buf, p_route_env->u_addr.src);
        } break;

        case (MM_MSG_GEN_PRANGE_STATUS):
        {
            mm_genc_plvl_handler_status_range(p_buf, p_route_env->u_addr.src);
        } break;

        default:
        {

        } break;
    }
}

/**
 ****************************************************************************************
 * @brief Inform Generic Power Level Client model about a received opcode in order
 * to check if the model is authorized to handle the associated message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] opcode        Opcode value to be checked
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_plvl_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if ((opcode == MM_MSG_GEN_PLVL_STATUS)
            || (opcode == MM_MSG_GEN_PLAST_STATUS)
            || (opcode == MM_MSG_GEN_PDFLT_STATUS)
            || (opcode == MM_MSG_GEN_PRANGE_STATUS))
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
 * @brief Get Generic Power Actual or Generic Power Last or Generic Power Default or
 * Generic Power Range state value
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] dst           Address of node's element from which state value must be retrieved
 * @param[in] get_option    Get option
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_plvl_cb_get(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                      uint16_t get_info)
{
    // Status
    uint8_t status = MESH_ERR_NO_ERROR;
    // Get type
    uint8_t get_type = get_info;

    // Check option value
    if (get_type <= MM_GET_TYPE_PLVL_MAX)
    {
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

            if (get_type == MM_GET_TYPE_PLVL_ACTUAL)
            {
                p_buf_env->opcode = MM_MSG_GEN_PLVL_GET;
            }
            else if (get_type == MM_GET_TYPE_PLVL_LAST)
            {
                p_buf_env->opcode = MM_MSG_GEN_PLAST_GET;
            }
            else if (get_type == MM_GET_TYPE_PLVL_DFLT)
            {
                p_buf_env->opcode = MM_MSG_GEN_PDFLT_GET;
            }
            else // (option == MM_GET_OPTION_PLVL_RANGE)
            {
                p_buf_env->opcode = MM_MSG_GEN_PRANGE_GET;
            }

            // Send the message
            mm_route_send(p_buf_get);
        }
        else
        {
            status = MESH_ERR_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        status = MESH_ERR_INVALID_PARAM;
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Set Generic Power Actual state value
 *
 * @param[in] p_env             Pointer to the environment allocated for the model
 * @param[in] dst               Address of node's element on which state value must be set
 * @param[in] state_1           Generic Power Actual state value
 * @param[in] state_2           N/A
 * @param[in] trans_time_ms     Transition time in milliseconds
 * @param[in] delay_ms          Delay in milliseconds
 * @param[in] trans_info        Transition information
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_plvl_cb_trans(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                             uint32_t state_1, uint32_t state_2,
                                             uint32_t trans_time_ms, uint16_t delay_ms,
                                             uint16_t trans_info)
{
    uint8_t status = MESH_ERR_NO_ERROR;
    // Long message or not
    bool long_set = (GETB(trans_info, MM_TRANS_INFO_LONG) || trans_time_ms || delay_ms);
    // Get message length
    uint8_t length = (long_set) ? MM_GEN_PLVL_SET_LEN : MM_GEN_PLVL_SET_MIN_LEN;
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_set = mm_route_buf_alloc(length);

    if (p_buf_set)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf_set);
        // Get pointer to environment
        mm_route_env_t *p_buf_env = (mm_route_env_t *)&p_buf_set->env;
        // Generic Power Level Actual state value
        int16_t power = state_1;

        // Prepare environment
        p_buf_env->app_key_lid = app_key_lid;
        p_buf_env->u_addr.dst = dst;
        p_buf_env->info = 0;
        p_buf_env->mdl_lid = p_env->mdl_lid;
        p_buf_env->opcode = (GETB(trans_info, MM_TRANS_INFO_ACK))
                                    ? MM_MSG_GEN_PLVL_SET : MM_MSG_GEN_PLVL_SET_UNACK;

        // Fill the message
        write16p(p_data + MM_GEN_PLVL_SET_POWER_POS, power);
        *(p_data + MM_GEN_PLVL_SET_TID_POS) = GETF(trans_info, MM_TRANS_INFO_TID);

        if (long_set)
        {
            // Set transition time and delay values
            *(p_data + MM_GEN_PLVL_SET_TRANS_TIME_POS) = mm_get_trans_time(trans_time_ms);
            *(p_data + MM_GEN_PLVL_SET_DELAY_POS) = delay_ms / 5;
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

/**
 ****************************************************************************************
 * @brief Set Generic Power Last or Generic Power Default or Generic Power Range state
 * value
 *
 * @param[in] p_env             Pointer to the environment allocated for the model
 * @param[in] dst               Address of node's element on which state value must be set
 * @param[in] state_1           State value 1
 * @param[in] state_2           State value 2
 * @param[in] set_info          Set information
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_plvl_cb_set(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                      uint32_t state_1, uint32_t state_2,
                                      uint16_t set_info)
{
    // Set type
    uint8_t set_type = GETF(set_info, MM_SET_INFO_TYPE);
    // Status
    uint8_t status = MESH_ERR_NO_ERROR;

    // Check provided set type
    if (set_type <= MM_SET_TYPE_PLVL_MAX)
    {
        // Pointer to the buffer that will contain the message
        mesh_buf_t *p_buf_set;
        // Opcode
        uint32_t opcode;
        // Length
        uint8_t length;

        if (set_type == MM_SET_TYPE_PLVL_DFLT)
        {
            length = MM_GEN_PLVL_DFLT_SET_LEN;
            opcode = GETB(set_info, MM_SET_INFO_ACK)
                            ? MM_MSG_GEN_PDFLT_SET : MM_MSG_GEN_PDFLT_SET_UNACK;
        }
        else // (set_type == MM_SET_TYPE_PLVL_RANGE)
        {
            length = MM_GEN_PLVL_RANGE_SET_LEN;
            opcode = GETB(set_info, MM_SET_INFO_ACK)
                            ? MM_MSG_GEN_PRANGE_SET : MM_MSG_GEN_PRANGE_SET_UNACK;
        }

        // Allocate a new buffer for the publication
        p_buf_set = mm_route_buf_alloc(length);

        if (p_buf_set)
        {
            // Get pointer to data
            uint8_t *p_data = MESH_BUF_DATA(p_buf_set);
            // Get pointer to environment
            mm_route_env_t *p_buf_env = (mm_route_env_t *)&p_buf_set->env;

            // Prepare environment
            p_buf_env->app_key_lid = app_key_lid;
            p_buf_env->u_addr.dst = dst;
            p_buf_env->info = 0;
            p_buf_env->mdl_lid = p_env->mdl_lid;
            p_buf_env->opcode = opcode;

            // Fill the message
            if (set_type == MM_SET_TYPE_PLVL_DFLT)
            {
                write16p(p_data + MM_GEN_PLVL_DFLT_SET_POWER_POS, state_1);
            }
            else // (set_type == MM_SET_TYPE_PLVL_RANGE)
            {
                write16p(p_data + MM_GEN_PLVL_RANGE_SET_MIN_POS, state_1);
                write16p(p_data + MM_GEN_PLVL_RANGE_SET_MAX_POS, state_2);
            }

            // Send the message
            mm_route_send(p_buf_set);
        }
        else
        {
            status = MESH_ERR_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        status = MESH_ERR_INVALID_PARAM;
    }

    return (status);
}

/*
 * GLOBAL FUNCTIONS
 ****************************************************************************************
 */

__STATIC mm_cli_cb_t mm_genc_plvl_cb = 
{
    .cb_get = mm_genc_plvl_cb_get,
    .cb_set = mm_genc_plvl_cb_set,
    .cb_trans = mm_genc_plvl_cb_trans,
};

uint8_t mm_genc_plvl_register(void)
{
    // Register the model
    m_lid_t mdl_lid = ms_register_model(MM_ID_GENC_PLVL, 0, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID)
    {
        // Inform the Model State Manager about registered model
        mm_genc_plvl_env_t *p_env_plvl = (mm_genc_plvl_env_t *)mm_state_register(0, MM_ID_GENC_PLVL, mdl_lid,
                                        MM_ROLE_CLI, sizeof(mm_genc_plvl_env_t));

        if (p_env_plvl)
        {
            // Get client-specific callback functions
            //mm_cli_cb_t *p_cb_cli = p_env_plvl->env.cb.u.p_cb_cli;

            // Set internal callback functions
            p_env_plvl->env.mdl_cb.cb_rx = mm_genc_plvl_cb_rx;
            p_env_plvl->env.mdl_cb.cb_opcode_check = mm_genc_plvl_cb_opcode_check;
            //p_cb_cli->cb_get = mm_genc_plvl_cb_get;
            //p_cb_cli->cb_set = mm_genc_plvl_cb_set;
            //p_cb_cli->cb_trans = mm_genc_plvl_cb_trans;
            p_env_plvl->env.mdl_cb.p_cb_cli = &mm_genc_plvl_cb;

            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENC_PLVL, 0, mdl_lid);
        }
    }

    return (mdl_lid);
}

/// @} end of group
