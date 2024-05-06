/**
 ****************************************************************************************
 *
 * @file mm_lightc_hsl.c
 *
 * @brief Mesh Model Light HSL Client Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_LIGHTC_HSL
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "mm_itf.h"
#include "mm_lightc.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Structure for Light HSL Client model environment
typedef struct mm_lightc_hsl_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
} mm_lightc_hsl_env_t;

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handler for Light HSL Status and Light HSL Target Status message
 * Note that both messages have the same content.
 *
 * @param[in] p_buf     Pointer to buffer containing the received message
 * @param[in] src       Source address of the message
 * @param[in] target    True if Light HSL Target Status message, else false
 ****************************************************************************************
 */
__STATIC void mm_lightc_hsl_handler_status(mesh_buf_t *p_buf, uint16_t src, bool target)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Light HSL Lightness state value
    uint16_t lightness = read16p(p_data + MM_LIGHT_HSL_STATUS_LIGHTNESS_POS);
    // Light HSL Hue state value
    uint16_t hue = read16p(p_data + MM_LIGHT_HSL_STATUS_HUE_POS);
    // Light HSL Saturation state value
    uint16_t sat = read16p(p_data + MM_LIGHT_HSL_STATUS_SAT_POS);
    // Remaining time in milliseconds
    uint32_t rem_time_ms;

    if (p_buf->data_len == MM_LIGHT_HSL_STATUS_LEN)
    {
        rem_time_ms = mm_get_trans_time_ms(*(p_data + MM_LIGHT_HSL_STATUS_REM_TIME_POS));
    }
    else
    {
        rem_time_ms = 0;
    }

    if (target)
    {
        // Inform the application about the received Light HSL Lightness state value
        mm_cli_state_ind_send(src, MM_STATE_LIGHT_HSL_TGT,
                                       (uint32_t)lightness | ((uint32_t)hue << 16), sat, rem_time_ms);
    }
    else
    {
        // Inform the application about the received Light HSL Lightness state value
        mm_cli_state_ind_send(src, MM_STATE_LIGHT_HSL_LN, lightness, 0, rem_time_ms);
        // Inform the application about the received Light HSL Hue state value
        mm_cli_state_ind_send(src, MM_STATE_LIGHT_HSL_HUE, hue, 0, rem_time_ms);
        // Inform the application about the received Light HSL Saturation state value
        mm_cli_state_ind_send(src, MM_STATE_LIGHT_HSL_SAT, sat, 0, rem_time_ms);
    }
}

/**
 ****************************************************************************************
 * @brief Handler for Light HSL Hue Status and Light HSL Saturation Status message.
 * Note that both message have same content structure (Hue and Staturation fields have
 * same size and same type).
 *
 * @param[in] p_buf     Pointer to buffer containing the received message
 * @param[in] src       Source address of the message
 * @param[in] hue       True if Light HSL Hue Status message, else false
 ****************************************************************************************
 */
__STATIC void mm_lightc_hsl_handler_status_single(mesh_buf_t *p_buf, uint16_t src, bool hue)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Present Light HSL Hue/Saturation state value
    uint16_t present = (int16_t)read16p(p_data + MM_LIGHT_HSL_HUE_STATUS_HUE_POS);
    // Target Light HSL Hue/Saturation state value
    uint16_t target;
    // Remaining time in milliseconds
    uint32_t rem_time_ms;

    // Check if optional parameters are provided
    if (p_buf->data_len == MM_LIGHT_HSL_HUE_STATUS_LEN)
    {
        target = read16p(p_data + MM_LIGHT_HSL_HUE_STATUS_TGT_HUE_POS);
        rem_time_ms = mm_get_trans_time_ms(*(p_data + MM_LIGHT_HSL_HUE_STATUS_REM_TIME_POS));
    }
    else
    {
        target = 0;
        rem_time_ms = 0;
    }

    // Inform the application about the received Light HSL Hue state value
    mm_cli_state_ind_send(src, (hue) ? MM_STATE_LIGHT_HSL_HUE : MM_STATE_LIGHT_HSL_SAT,
                              present, target, rem_time_ms);
}

/**
 ****************************************************************************************
 * @brief Handler for Light HSL Default Status message
 *
 * @param[in] p_buf     Pointer to buffer containing the received message
 * @param[in] src       Source address of the message
 ****************************************************************************************
 */
__STATIC void mm_lightc_hsl_handler_status_dflt(mesh_buf_t *p_buf, uint16_t src)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Light HSL Lightness Default state value
    uint16_t lightness = read16p(p_data + MM_LIGHT_HSL_DFLT_STATUS_LIGHTNESS_POS);
    // Light HSL Hue Default state value
    uint16_t hue = read16p(p_data + MM_LIGHT_HSL_DFLT_STATUS_HUE_POS);
    // Light HSL Saturation Default state value
    uint16_t sat = read16p(p_data + MM_LIGHT_HSL_DFLT_STATUS_SAT_POS);

    // Inform the application about the received Light HSL Default state value
    mm_cli_state_ind_send(src, MM_STATE_LIGHT_HSL_DFLT,
                              (uint32_t)lightness | ((uint32_t)hue << 16), sat, 0);
}

/**
 ****************************************************************************************
 * @brief Handler for Light HSL Range Status message
 *
 * @param[in] p_buf     Pointer to buffer containing the received message
 * @param[in] src       Source address of the message
 ****************************************************************************************
 */
__STATIC void mm_lightc_hsl_handler_status_range(mesh_buf_t *p_buf, uint16_t src)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Light HSL Hue Range state value (minimum)
    uint16_t hue_min = read16p(p_data + MM_LIGHT_HSL_RANGE_STATUS_HUE_MIN_POS);
    // Light HSL Hue Range state value (maximum)
    uint16_t hue_max = read16p(p_data + MM_LIGHT_HSL_RANGE_STATUS_HUE_MAX_POS);
    // Light HSL Saturation Range state value (minimum)
    uint16_t sat_min = read16p(p_data + MM_LIGHT_HSL_RANGE_STATUS_SAT_MIN_POS);
    // Light HSL Saturation Range state value (maximum)
    uint16_t sat_max = read16p(p_data + MM_LIGHT_HSL_RANGE_STATUS_SAT_MAX_POS);

    // Inform the application about the received Light HSL Hue Range state value
    mm_cli_state_ind_send(src, MM_STATE_LIGHT_HSL_RANGE_HUE, hue_min, hue_max, 0);
    // Inform the application about the received Light HSL Hue Range state value
    mm_cli_state_ind_send(src, MM_STATE_LIGHT_HSL_RANGE_SAT, sat_min, sat_max, 0);
}

/*
 * INTERNAL CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Inform Light HSL Client model about reception of a message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] p_route_env   Pointer to routing environment containing information about the
 * received message
 ****************************************************************************************
 */
__STATIC void mm_lightc_hsl_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                                  mm_route_env_t *p_route_env)
{
    // Call the appropriate handler for the received message
    switch (p_route_env->opcode)
    {
        case (MM_MSG_LIGHT_HSL_HUE_STATUS):
        case (MM_MSG_LIGHT_HSL_SAT_STATUS):
        {
            mm_lightc_hsl_handler_status_single(p_buf, p_route_env->u_addr.src,
                                                (p_route_env->opcode == MM_MSG_LIGHT_HSL_HUE_STATUS));
        } break;

        case (MM_MSG_LIGHT_HSL_STATUS):
        case (MM_MSG_LIGHT_HSL_TGT_STATUS):
        {
            mm_lightc_hsl_handler_status(p_buf, p_route_env->u_addr.src,
                                         (p_route_env->opcode == MM_MSG_LIGHT_HSL_TGT_STATUS));
        } break;

        case (MM_MSG_LIGHT_HSL_DFLT_STATUS):
        {
            mm_lightc_hsl_handler_status_dflt(p_buf, p_route_env->u_addr.src);
        } break;

        case (MM_MSG_LIGHT_HSL_RANGE_STATUS):
        {
            mm_lightc_hsl_handler_status_range(p_buf, p_route_env->u_addr.src);
        } break;

        default:
        {
        } break;
    }
}

/**
 ****************************************************************************************
 * @brief Inform Light HSL Client model about a received opcode in order
 * to check if the model is authorized to handle the associated message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] opcode        Opcode value to be checked
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_lightc_hsl_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if ((opcode == MM_MSG_LIGHT_HSL_HUE_STATUS)
            || (opcode == MM_MSG_LIGHT_HSL_SAT_STATUS)
            || (opcode == MM_MSG_LIGHT_HSL_STATUS)
            || (opcode == MM_MSG_LIGHT_HSL_TGT_STATUS)
            || (opcode == MM_MSG_LIGHT_HSL_DFLT_STATUS)
            || (opcode == MM_MSG_LIGHT_HSL_RANGE_STATUS))
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
 * @brief Send Light HSL Get or Light HSL Hue Get or Light HSL Saturation Get
 * or Light HSL Default Get or Light HSL Target or Light HSL Range Get message
 *
 *      - Light HSL Get: Light HSL Lightness and Light HSL Hue and Light HSL Saturation state
 *      values
 *      - Light HSL Saturation Get: Light HSL Saturation state value
 *      - Light HSL Hue Get: Light HSL Hue state value
 *      - Light HSL Target Get: Light HSL Lightness and Light HSL Hue and Light HSL Saturation
 *      state target values
 *      - Light HSL Default Get: Light Lightness and Light HSL Hue and Light HSL Saturation
 *      state default values
 *      - Light HSL Range Get: Light HSL Hue and Light HSL Saturation state range values
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] dst           Address of node's element from which state value must be retrieved
 * @param[in] get_option    Get option (@see enum mm_get_type_light_hsl)
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_lightc_hsl_cb_get(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                       uint16_t get_info)
{
    // Status
    uint8_t status = MESH_ERR_NO_ERROR;
    // Get type
    uint8_t get_type = get_info;

    // Check option value
    if (get_type <= MM_GET_TYPE_LIGHT_HSL_MAX)
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

            if (get_type == MM_GET_TYPE_LIGHT_HSL)
            {
                p_buf_env->opcode = MM_MSG_LIGHT_HSL_GET;
            }
            else if (get_type == MM_GET_TYPE_LIGHT_HSL_HUE)
            {
                p_buf_env->opcode = MM_MSG_LIGHT_HSL_HUE_GET;
            }
            else if (get_type == MM_GET_TYPE_LIGHT_HSL_SAT)
            {
                p_buf_env->opcode = MM_MSG_LIGHT_HSL_SAT_GET;
            }
            else if (get_type == MM_GET_TYPE_LIGHT_HSL_TGT)
            {
                p_buf_env->opcode = MM_MSG_LIGHT_HSL_TGT_GET;
            }
            else if (get_type == MM_GET_TYPE_LIGHT_HSL_DFLT)
            {
                p_buf_env->opcode = MM_MSG_LIGHT_HSL_DFLT_GET;
            }
            else // (option == MM_GET_TYPE_LIGHT_HSL_RANGE)
            {
                p_buf_env->opcode = MM_MSG_LIGHT_HSL_RANGE_GET;
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
 * @brief Send Light HSL Set or Light HSL Set Unacknowledged message in order to set:
 *      - Light HSL Lightness state value
 *      - Light HSL Temperature state value
 *      - Light HSL Delta UV state value
 * or Light HSL Hue Set or Light CTL Hue Set Unacknowledged message in order to set Light
 * HSL Hue state value
 * or Light HSL Saturation Set or Light HSL Saturation Set Unacknowledged message in order to
 * set Light HSL Saturation state value
 *
 * @param[in] p_env             Pointer to the environment allocated for the model
 * @param[in] dst               Address of node's element on which state value must be set
 * @param[in] state_1           State value 1
 * @param[in] state_2           State value 2
 * @param[in] trans_time_ms     Transition time in milliseconds
 * @param[in] delay_ms          Delay in milliseconds
 * @param[in] trans_info        Transition information (@see enum mm_trans_info and
 * @see enum mm_trans_type_light_hsl)
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_lightc_hsl_cb_trans(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                              uint32_t state_1, uint32_t state_2,
                                              uint32_t trans_time_ms, uint16_t delay_ms,
                                              uint16_t trans_info)
{
    // Status
    uint8_t status = MESH_ERR_NO_ERROR;
    // Transition type
    uint8_t trans_type = GETF(trans_info, MM_TRANS_INFO_TYPE);

    if (trans_type <= MM_TRANS_TYPE_LIGHT_HSL_MAX)
    {
        // Long message or not
        bool long_set = (GETB(trans_info, MM_TRANS_INFO_LONG) || trans_time_ms || delay_ms);
        // Pointer to the buffer that will contain the message
        mesh_buf_t *p_buf_set;
        // Message length
        uint8_t length;

        if (trans_type == MM_TRANS_TYPE_LIGHT_HSL)
        {
            length = (long_set) ? MM_LIGHT_HSL_SET_LEN : MM_LIGHT_HSL_SET_MIN_LEN;
        }
        else if (trans_type == MM_TRANS_TYPE_LIGHT_HSL_HUE)
        {
            length = (long_set) ? MM_LIGHT_HSL_HUE_SET_LEN : MM_LIGHT_HSL_HUE_SET_MIN_LEN;
        }
        else // (trans_type == MM_TRANS_TYPE_LIGHT_HSL_SAT)
        {
            length = (long_set) ? MM_LIGHT_HSL_SAT_SET_LEN : MM_LIGHT_HSL_SAT_SET_MIN_LEN;
        }

        // Allocate a new buffer
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

            // Set opcode and start to fill the message
            if (trans_type == MM_TRANS_TYPE_LIGHT_HSL)
            {
                p_buf_env->opcode = (GETB(trans_info, MM_TRANS_INFO_ACK))
                                ? MM_MSG_LIGHT_HSL_SET : MM_MSG_LIGHT_HSL_SET_UNACK;

                write16p(p_data + MM_LIGHT_HSL_SET_LIGHTNESS_POS, state_1);
                write16p(p_data + MM_LIGHT_HSL_SET_HUE_POS, state_1 >> 16);
                write16p(p_data + MM_LIGHT_HSL_SET_SAT_POS, state_2);

                p_data += MM_LIGHT_HSL_SET_TID_POS;
            }
            else if (trans_type == MM_TRANS_TYPE_LIGHT_HSL_HUE)
            {
                p_buf_env->opcode = (GETB(trans_info, MM_TRANS_INFO_ACK))
                                ? MM_MSG_LIGHT_HSL_HUE_SET : MM_MSG_LIGHT_HSL_HUE_SET_UNACK;

                write16p(p_data + MM_LIGHT_HSL_HUE_SET_HUE_POS, state_1);

                p_data += MM_LIGHT_HSL_HUE_SET_TID_POS;
            }
            else // (trans_type == MM_TRANS_TYPE_LIGHT_HSL_SAT)
            {
                p_buf_env->opcode = (GETB(trans_info, MM_TRANS_INFO_ACK))
                                ? MM_MSG_LIGHT_HSL_SAT_SET : MM_MSG_LIGHT_HSL_SAT_SET_UNACK;

                write16p(p_data + MM_LIGHT_HSL_SAT_SET_SAT_POS, state_1);

                p_data += MM_LIGHT_HSL_SAT_SET_TID_POS;
            }

            // Fill the message
            *(p_data++) = GETF(trans_info, MM_TRANS_INFO_TID);

            if (long_set)
            {
                // Set transition time and delay values
                *(p_data++) = mm_get_trans_time(trans_time_ms);
                *(p_data++) = delay_ms / 5;
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

/**
 ****************************************************************************************
 * @brief Send Light HSL Default Set or Light HSL Default Set Unacknowledged message in
 * order to set:
 *      - Light Lightness Default state value
 *      - Light HSL Hue Default state value
 *      - Light HSL Saturation Default state value
 * or Light HSL Range Set or Light HSL Range Set Unacknowledged message in order to set
 * Light HSL Hue and Light HSL Saturation state range value
 *
 * @param[in] p_env             Pointer to the environment allocated for the model
 * @param[in] dst               Address of node's element on which state value must be set
 * @param[in] state_1           State value 1
 * @param[in] state_2           State value 2
 * @param[in] set_info          Set information (@see enum mm_set_info and
 * @see enum mm_set_type_light_hsl for set type value)
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_lightc_hsl_cb_set(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                       uint32_t state_1, uint32_t state_2,
                                       uint16_t set_info)
{
    // Set type
    uint8_t set_type = GETF(set_info, MM_SET_INFO_TYPE);
    // Status
    uint8_t status = MESH_ERR_NO_ERROR;

    // Check provided set type
    if (set_type <= MM_SET_TYPE_LIGHT_HSL_MAX)
    {
        // Pointer to the buffer that will contain the message
        mesh_buf_t *p_buf_set;
        // Opcode
        uint32_t opcode;
        // Length
        uint8_t length;

        if (set_type == MM_SET_TYPE_LIGHT_HSL_RANGE)
        {
            length = MM_LIGHT_HSL_RANGE_SET_LEN;
            opcode = GETB(set_info, MM_SET_INFO_ACK)
                            ? MM_MSG_LIGHT_HSL_RANGE_SET : MM_MSG_LIGHT_HSL_RANGE_SET_UNACK;
        }
        else // (set_type == MM_SET_TYPE_LIGHT_HSL_DFLT)
        {
            length = MM_LIGHT_HSL_DFLT_SET_LEN;
            opcode = GETB(set_info, MM_SET_INFO_ACK)
                            ? MM_MSG_LIGHT_HSL_DFLT_SET : MM_MSG_LIGHT_HSL_DFLT_SET_UNACK;
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
            if (set_type == MM_SET_TYPE_LIGHT_HSL_RANGE)
            {
                write16p(p_data + MM_LIGHT_HSL_RANGE_SET_HUE_MIN_POS, state_1);
                write16p(p_data + MM_LIGHT_HSL_RANGE_SET_HUE_MAX_POS, state_1 >> 16);
                write16p(p_data + MM_LIGHT_HSL_RANGE_SET_SAT_MIN_POS, state_2);
                write16p(p_data + MM_LIGHT_HSL_RANGE_SET_SAT_MAX_POS, state_2 >> 16);
            }
            else // (set_type == MM_SET_TYPE_LIGHT_HSL_DFLT)
            {
                write16p(p_data + MM_LIGHT_HSL_DFLT_SET_LIGHTNESS_POS, state_1);
                write16p(p_data + MM_LIGHT_HSL_DFLT_SET_HUE_POS, (state_1 >> 16));
                write16p(p_data + MM_LIGHT_HSL_DFLT_SET_SAT_POS, state_2);
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

__STATIC mm_cli_cb_t mm_lightc_hsl_cb = 
{
    .cb_get = mm_lightc_hsl_cb_get,
    .cb_set = mm_lightc_hsl_cb_set,
    .cb_trans = mm_lightc_hsl_cb_trans,
};

uint8_t mm_lightc_hsl_register(void)
{
    // Register the model
    m_lid_t mdl_lid = ms_register_model(MM_ID_LIGHTC_HSL, 0, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID)
    {
        // Inform the Model State Manager about registered model
        mm_lightc_hsl_env_t *p_env_hsl = (mm_lightc_hsl_env_t *)mm_state_register(0, MM_ID_LIGHTC_HSL, mdl_lid,
                                        MM_ROLE_CLI, sizeof(mm_lightc_hsl_env_t));

        if (p_env_hsl)
        {
            // Get client-specific callback functions
            //mm_cli_cb_t *p_cb_cli = p_env_hsl->env.cb.u.p_cb_cli;

            // Set internal callback functions
            p_env_hsl->env.mdl_cb.cb_rx = mm_lightc_hsl_cb_rx;
            p_env_hsl->env.mdl_cb.cb_opcode_check = mm_lightc_hsl_cb_opcode_check;
            //p_cb_cli->cb_get = mm_lightc_hsl_cb_get;
            //p_cb_cli->cb_set = mm_lightc_hsl_cb_set;
            //p_cb_cli->cb_trans = mm_lightc_hsl_cb_trans;
            p_env_hsl->env.mdl_cb.p_cb_cli = &mm_lightc_hsl_cb;

            // Inform application about registered model
            mm_register_ind_send(MM_ID_LIGHTC_HSL, 0, mdl_lid);
        }
    }

    return (mdl_lid);
}

/// @} end of group
