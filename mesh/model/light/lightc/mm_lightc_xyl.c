/**
 ****************************************************************************************
 *
 * @file mm_lightc_xyl.c
 *
 * @brief Mesh Model Light xyL Client Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_LIGHTC_XYL
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

/// Structure for Light xyL Client model environment
typedef struct mm_lightc_xyL_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
} mm_lightc_xyl_env_t;

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handler for Light xyL Status and Light xyL Target Status messages.
 * Note that both messages have the same content.
 *
 * @param[in] p_buf     Pointer to buffer containing the received message
 * @param[in] src       Source address of the message
 * @param[in] target    True if Light xyL Target Status message, else false
 ****************************************************************************************
 */
__STATIC void mm_lightc_xyl_handler_status(mesh_buf_t *p_buf, uint16_t src, bool target)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Light xyL Lightness state value
    uint16_t lightness = read16p(p_data + MM_LIGHT_XYL_STATUS_LIGHTNESS_POS);
    // Light xyL x state value
    uint16_t x = read16p(p_data + MM_LIGHT_XYL_STATUS_X_POS);
    // Light xyL y state value
    uint16_t y = read16p(p_data + MM_LIGHT_XYL_STATUS_Y_POS);
    // Remaining time in milliseconds
    uint32_t rem_time_ms;

    if (p_buf->data_len == MM_LIGHT_XYL_STATUS_LEN)
    {
        rem_time_ms = mm_get_trans_time_ms(*(p_data + MM_LIGHT_XYL_STATUS_REM_TIME_POS));
    }
    else
    {
        rem_time_ms = 0;
    }

    // Inform the application about the received Light xyL Lightness state value
    mm_cli_state_ind_send(src, (target) ? MM_STATE_LIGHT_XYL_LN_TGT : MM_STATE_LIGHT_XYL_LN,
                              lightness, 0, rem_time_ms);
    // Inform the application about the received Light xyL XY state value
    mm_cli_state_ind_send(src, (target) ? MM_STATE_LIGHT_XYL_XY_TGT : MM_STATE_LIGHT_XYL_XY,
                              x , y, rem_time_ms);
}

/**
 ****************************************************************************************
 * @brief Handler for Light xyL Default Status message
 *
 * @param[in] p_buf     Pointer to buffer containing the received message
 * @param[in] src       Source address of the message
 ****************************************************************************************
 */
__STATIC void mm_lightc_xyl_handler_status_dflt(mesh_buf_t *p_buf, uint16_t src)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Light xyL Lightness Default state value
    uint16_t lightness = read16p(p_data + MM_LIGHT_XYL_TGT_STATUS_LIGHTNESS_POS);
    // Light xyL x Default state value
    uint16_t x = read16p(p_data + MM_LIGHT_XYL_TGT_STATUS_X_POS);
    // Light xyL y Default state value
    uint16_t y = read16p(p_data + MM_LIGHT_XYL_TGT_STATUS_Y_POS);

    // Inform the application about the received Light xyL Lightness Default state value
    mm_cli_state_ind_send(src, MM_STATE_LIGHT_XYL_LN_DFLT, lightness, 0, 0);
    // Inform the application about the received Target Light xyL x and y state value
    mm_cli_state_ind_send(src, MM_STATE_LIGHT_XYL_XY_DFLT, x, y, 0);
}

/**
 ****************************************************************************************
 * @brief Handler for Light xyL Range Status message
 *
 * @param[in] p_buf     Pointer to buffer containing the received message
 * @param[in] src       Source address of the message
 ****************************************************************************************
 */
__STATIC void mm_lightc_xyl_handler_status_range(mesh_buf_t *p_buf, uint16_t src)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Light xyL x Range state value (minimum)
    uint16_t x_min = read16p(p_data + MM_LIGHT_XYL_RANGE_STATUS_X_MIN_POS);
    // Light xyL x Range state value (maximum)
    uint16_t x_max = read16p(p_data + MM_LIGHT_XYL_RANGE_STATUS_X_MAX_POS);
    // Light xyL y Range state value (minimum)
    uint16_t y_min = read16p(p_data + MM_LIGHT_XYL_RANGE_STATUS_Y_MIN_POS);
    // Light xyL y Range state value (maximum)
    uint16_t y_max = read16p(p_data + MM_LIGHT_XYL_RANGE_STATUS_Y_MAX_POS);

    // Inform the application about the received Light xyL x Range state value
    mm_cli_state_ind_send(src, MM_STATE_LIGHT_XYL_XY_RANGE,
                              (uint32_t)x_min | ((uint32_t)x_max << 16),
                              (uint32_t)y_min | ((uint32_t)y_max << 16), 0);
}

/*
 * INTERNAL CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Inform Light xyL Client model about reception of a message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] p_route_env   Pointer to routing environment containing information about the
 * received message
 ****************************************************************************************
 */
__STATIC void mm_lightc_xyl_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                                  mm_route_env_t *p_route_env)
{
    // Call the appropriate handler for the received message
    switch (p_route_env->opcode)
    {
        case (MM_MSG_LIGHT_XYL_STATUS):
        case (MM_MSG_LIGHT_XYL_TGT_STATUS):
        {
            mm_lightc_xyl_handler_status(p_buf, p_route_env->u_addr.src,
                                         (p_route_env->opcode == MM_MSG_LIGHT_XYL_TGT_STATUS));
        } break;

        case (MM_MSG_LIGHT_XYL_DFLT_STATUS):
        {
            mm_lightc_xyl_handler_status_dflt(p_buf, p_route_env->u_addr.src);
        } break;

        case (MM_MSG_LIGHT_XYL_RANGE_STATUS):
        {
            mm_lightc_xyl_handler_status_range(p_buf, p_route_env->u_addr.src);
        } break;

        default:
        {
        } break;
    }
}

/**
 ****************************************************************************************
 * @brief Inform Light xyL Client model about a received opcode in order
 * to check if the model is authorized to handle the associated message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] opcode        Opcode value to be checked
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_lightc_xyl_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if ((opcode == MM_MSG_LIGHT_XYL_STATUS)
            || (opcode == MM_MSG_LIGHT_XYL_TGT_STATUS)
            || (opcode == MM_MSG_LIGHT_XYL_DFLT_STATUS)
            || (opcode == MM_MSG_LIGHT_XYL_RANGE_STATUS))
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
 * @brief Send Light xyL Get or Light xyL Target Get or Light xyL Default Get or Light xyL
 * Range message.
 *
 *      - Light xyL Get: Light xyL Lightness and Light xyL x and Light xyL y state values
 *      - Light xyL Target Get: Light xyL Lightness and Light xyL x and Light xyL y state
 * target values
 *      - Light xyL Default Get: Light Lightness and Light xyL x and Light xyL y state
 * default values
 *      - Light xyL Range Get: Light xyL x and Light xyL y state range values
 *
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] dst           Address of node's element from which state value must be retrieved
 * @param[in] get_option    Get option (@see enum mm_get_type_light_xyl)
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_lightc_xyl_cb_get(mm_mdl_env_t *p_env, m_lid_t app_key_lid,
                                       uint16_t dst, uint16_t get_info)
{
    // Status
    uint8_t status = MESH_ERR_NO_ERROR;
    // Get type
    uint8_t get_type = get_info;

    // Check option value
    if (get_type <= MM_GET_TYPE_LIGHT_XYL_MAX)
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

            if (get_type == MM_GET_TYPE_LIGHT_XYL)
            {
                p_buf_env->opcode = MM_MSG_LIGHT_XYL_GET;
            }
            else if (get_type == MM_GET_TYPE_LIGHT_XYL_TGT)
            {
                p_buf_env->opcode = MM_MSG_LIGHT_XYL_TGT_GET;
            }
            else if (get_type == MM_GET_TYPE_LIGHT_XYL_DFLT)
            {
                p_buf_env->opcode = MM_MSG_LIGHT_XYL_DFLT_GET;
            }
            else // (option == MM_GET_TYPE_LIGHT_XYL_RANGE)
            {
                p_buf_env->opcode = MM_MSG_LIGHT_XYL_RANGE_GET;
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
 * @brief Send Light xyL Set or Light xyL Set Unacknowledged message in order to set:
 *      - Light xyL Lightness state value
 *      - Light xyL x state value
 *      - Light xyL y state value
 *
 * @param[in] p_env             Pointer to the environment allocated for the model
 * @param[in] dst               Address of node's element on which state value must be set
 * @param[in] state_1           State value 1
 *      - Light xyL Lightness state value (uint16_t) (Bit 0-15) | Light xyL x
 *      state value (uint16_t) (Bit 16-31)
 * @param[in] state_2           State value 2
 *      - Light xyL y state value (uint16_t)
 * @param[in] trans_time_ms     Transition time in milliseconds
 * @param[in] delay_ms          Delay in milliseconds
 * @param[in] trans_info        Transition information (@see enum mm_trans_info and
 * @see enum mm_trans_type_light_xyl)
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_lightc_xyl_cb_trans(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                              uint32_t state_1, uint32_t state_2,
                                              uint32_t trans_time_ms, uint16_t delay_ms,
                                              uint16_t trans_info)
{
    // Status
    uint8_t status = MESH_ERR_NO_ERROR;
    // Long message or not
    bool long_set = (GETB(trans_info, MM_TRANS_INFO_LONG) || trans_time_ms || delay_ms);
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_set;
    // Message length
    uint8_t length = (long_set) ? MM_LIGHT_XYL_SET_LEN : MM_LIGHT_XYL_SET_MIN_LEN;

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
        p_buf_env->opcode = (GETB(trans_info, MM_TRANS_INFO_ACK))
                        ? MM_MSG_LIGHT_XYL_SET : MM_MSG_LIGHT_XYL_SET_UNACK;

        write16p(p_data + MM_LIGHT_XYL_SET_LIGHTNESS_POS, state_1);
        write16p(p_data + MM_LIGHT_XYL_SET_X_POS, state_1 >> 16);
        write16p(p_data + MM_LIGHT_XYL_SET_Y_POS, state_2);

        // Fill the message
        *(p_data + MM_LIGHT_XYL_SET_TID_POS) = GETF(trans_info, MM_TRANS_INFO_TID);

        if (long_set)
        {
            // Set transition time and delay values
            *(p_data + MM_LIGHT_XYL_SET_TRANS_TIME_POS) = mm_get_trans_time(trans_time_ms);
            *(p_data + MM_LIGHT_XYL_SET_DELAY_POS) = delay_ms / 5;
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
 * @brief Send Light xyL Default Set or Light xyL Default Set Unacknowledged message in
 * order to set:
 *      - Light Lightness Default state value
 *      - Light xyL x Default state value
 *      - Light xyL y Default state value
 * or Light xyL Range Set or Light xyL Range Set Unacknowledged message in order to set
 * Light xyL x and Light xyL y Range state values
 *
 * @param[in] p_env             Pointer to the environment allocated for the model
 * @param[in] dst               Address of node's element on which state value must be set
 * @param[in] state_1           State value 1
 *      - Light Lightness Default state value (uint16_t) (Bit 0-15) | Light xyL x
 *      Default state value (uint16_t) (Bit 16-31) if Light xyL Default Set message
 * or
 *      - Light xyL x Range Min state value (uint16_t) (Bit 0-15) | Light xyL x Range Max
 *      state value (uint16_t) (Bit 16-31) if Light xyL Range Set message
 * @param[in] state_2           State value 2
 *      - Light xyL y Default state value (uint16_t) if Light xyL Default Set message
 * or
 *      - Light xyL y Range Min state value (uint16_t) (Bit 0-15) | Light xyL y Range Max
 *      state value (uint16_t) (Bit 16-31) if Light xyL Range Set message
 * @param[in] set_info          Set information (@see enum mm_set_info and
 * @see enum mm_set_type_light_xyl for set type value)
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_lightc_xyl_cb_set(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                       uint32_t state_1, uint32_t state_2,
                                       uint16_t set_info)
{
    // Set type
    uint8_t set_type = GETF(set_info, MM_SET_INFO_TYPE);
    // Status
    uint8_t status = MESH_ERR_NO_ERROR;

    // Check provided set type
    if (set_type <= MM_SET_TYPE_LIGHT_XYL_MAX)
    {
        // Pointer to the buffer that will contain the message
        mesh_buf_t *p_buf_set;
        // Opcode
        uint32_t opcode;
        // Length
        uint8_t length;

        if (set_type == MM_SET_TYPE_LIGHT_XYL_RANGE)
        {
            length = MM_LIGHT_XYL_RANGE_SET_LEN;
            opcode = GETB(set_info, MM_SET_INFO_ACK)
                            ? MM_MSG_LIGHT_XYL_RANGE_SET : MM_MSG_LIGHT_XYL_RANGE_SET_UNACK;
        }
        else // (set_type == MM_SET_TYPE_LIGHT_XYL_DFLT)
        {
            length = MM_LIGHT_XYL_DFLT_SET_LEN;
            opcode = GETB(set_info, MM_SET_INFO_ACK)
                            ? MM_MSG_LIGHT_XYL_DFLT_SET : MM_MSG_LIGHT_XYL_DFLT_SET_UNACK;
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
            if (set_type == MM_SET_TYPE_LIGHT_XYL_RANGE)
            {
                write16p(p_data + MM_LIGHT_XYL_RANGE_SET_X_MIN_POS, state_1);
                write16p(p_data + MM_LIGHT_XYL_RANGE_SET_X_MAX_POS, state_1 >> 16);
                write16p(p_data + MM_LIGHT_XYL_RANGE_SET_Y_MIN_POS, state_2);
                write16p(p_data + MM_LIGHT_XYL_RANGE_SET_Y_MAX_POS, state_2 >> 16);
            }
            else // (set_type == MM_SET_TYPE_LIGHT_XYL_DFLT)
            {
                write16p(p_data + MM_LIGHT_XYL_DFLT_SET_LIGHTNESS_POS, state_1);
                write16p(p_data + MM_LIGHT_XYL_DFLT_SET_X_POS, (state_1 >> 16));
                write16p(p_data + MM_LIGHT_XYL_DFLT_SET_Y_POS, state_2);
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

__STATIC const mm_cli_cb_t mm_lightc_xyl_cb = 
{
    .cb_get = mm_lightc_xyl_cb_get,
    .cb_set = mm_lightc_xyl_cb_set,
    .cb_trans = mm_lightc_xyl_cb_trans,
};

uint8_t mm_lightc_xyl_register(void)
{
    // Register the model
    m_lid_t mdl_lid = ms_register_model(MM_ID_LIGHTC_XYL, 0, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID)
    {
        // Inform the Model State Manager about registered model
        mm_lightc_xyl_env_t *p_env_xyl = (mm_lightc_xyl_env_t *)mm_state_register(0, MM_ID_LIGHTC_XYL, mdl_lid,
                                        MM_ROLE_CLI, sizeof(mm_lightc_xyl_env_t));

        if (p_env_xyl)
        {
            // Get client-specific callback functions
            //mm_cli_cb_t *p_cb_cli = p_env_xyl->env.cb.u.p_cb_cli;

            // Set internal callback functions
            p_env_xyl->env.mdl_cb.cb_rx = mm_lightc_xyl_cb_rx;
            p_env_xyl->env.mdl_cb.cb_opcode_check = mm_lightc_xyl_cb_opcode_check;
            //p_cb_cli->cb_get = mm_lightc_xyl_cb_get;
            //p_cb_cli->cb_set = mm_lightc_xyl_cb_set;
            //p_cb_cli->cb_trans = mm_lightc_xyl_cb_trans;
            p_env_xyl->env.mdl_cb.p_cb_cli = &mm_lightc_xyl_cb;

            // Inform application about registered model
            mm_register_ind_send(MM_ID_LIGHTC_XYL, 0, mdl_lid);
        }
    }

    return (mdl_lid);
}

/// @} end of group
