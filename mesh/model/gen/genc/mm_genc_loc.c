/**
 ****************************************************************************************
 *
 * @file mm_genc_loc.c
 *
 * @brief Mesh Model Generic Location Client Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_GENC_LOC
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

/// Structure for Generic Location Client model environment
typedef struct mm_genc_loc_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
} mm_genc_loc_env_t;

/*
 * INTERNAL CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Inform Generic Location Client model about reception of a message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] p_route_env   Pointer to routing environment containing information about the
 * received message
 ****************************************************************************************
 */
__STATIC void mm_genc_loc_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                                mm_route_env_t *p_route_env)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);

    if (p_route_env->opcode == MM_MSG_GEN_LOCG_STATUS)
    {
        // Global latitude value
        int32_t latitude = (int32_t)read32p(p_data + MM_GEN_LOC_STATUS_GLOB_LAT_POS);
        // Global longitude value
        int32_t longitude = (int32_t)read32p(p_data + MM_GEN_LOC_STATUS_GLOB_LONG_POS);
        // Global altitude value
        int16_t altitude = (int16_t)read16p(p_data + MM_GEN_LOC_STATUS_GLOB_ALT_POS);

        // Inform the application about the received Generic Location state value (global part)
        mm_cli_locg_ind_send(p_route_env->u_addr.src, latitude, longitude, altitude);
    }
    else
    {
        // Local north value
        int16_t north = (int16_t)read16p(p_data + MM_GEN_LOC_STATUS_LOC_NORTH_POS);
        // Local east value
        int16_t east = (int16_t)read16p(p_data + MM_GEN_LOC_STATUS_LOC_EAST_POS);
        // Local altitude value
        int16_t altitude = (int16_t)read16p(p_data + MM_GEN_LOC_STATUS_LOC_ALT_POS);
        // Floor value
        uint8_t floor = *(p_data + MM_GEN_LOC_STATUS_LOC_FLOOR_POS);
        // Uncertainty value
        uint16_t uncertainty = read16p(p_data + MM_GEN_LOC_STATUS_LOC_UNCERT_POS);

        // Inform the application about the received Generic Location state value (local part)
        mm_cli_locl_ind_send(p_route_env->u_addr.src, north, east, altitude,
                                  floor, uncertainty);
    }
}

/**
 ****************************************************************************************
 * @brief Inform Generic Location Client model about a received opcode in order
 * to check if the model is authorized to handle the associated message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] opcode        Opcode value to be checked
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_loc_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if ((opcode == MM_MSG_GEN_LOCG_STATUS)
            || (opcode == MM_MSG_GEN_LOCL_STATUS))
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
 * @brief Send a Generic Location Get message in order to retrieve Generic Location state
 * value (either global or local part) of a given node's element.
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] dst           Address of node's element from which Generic Location state value
 * must be retrieved
 * @param[in] get_info      Get information
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_loc_cb_get(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
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
        p_buf_env->opcode = (get_info == MM_GET_TYPE_LOC_GLOBAL)
                                        ? MM_MSG_GEN_LOCG_GET : MM_MSG_GEN_LOCL_GET;

        // Send the message
        mm_route_send(p_buf_get);
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

__STATIC mm_cli_cb_t mm_genc_loc_cb = 
{
    .cb_get = mm_genc_loc_cb_get,
    .cb_set = NULL,
    .cb_trans = NULL,
};

uint8_t mm_genc_loc_register(void)
{
    // Model local index
    m_lid_t mdl_lid = ms_register_model(MM_ID_GENC_LOC, 0, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID)
    {
        // Inform the Model State Manager about registered model
        mm_genc_loc_env_t *p_env_loc = (mm_genc_loc_env_t *)mm_state_register(0, MM_ID_GENC_LOC, mdl_lid,
                                        MM_ROLE_CLI, sizeof(mm_genc_loc_env_t));

        if (p_env_loc)
        {
            // Get client-specific callback functions
            //mm_cli_cb_t *p_cb_cli = p_env_loc->env.cb.u.p_cb_cli;

            // Set internal callback functions
            p_env_loc->env.mdl_cb.cb_rx = mm_genc_loc_cb_rx;
            p_env_loc->env.mdl_cb.cb_opcode_check = mm_genc_loc_cb_opcode_check;
            //p_cb_cli->cb_get = mm_genc_loc_cb_get;
            p_env_loc->env.mdl_cb.p_cb_cli = &mm_genc_loc_cb;

            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENC_LOC, 0, mdl_lid);
        }
    }

    return (mdl_lid);
}

uint8_t mm_genc_locg_set(m_lid_t mdl_lid, m_lid_t app_key_lid, uint16_t dst,
                                uint8_t set_info, int32_t latitude, int32_t longitude, int16_t altitude)
{
    uint8_t status = MESH_ERR_NO_ERROR;
    mm_mdl_env_t *p_env = mm_state_get_env(mdl_lid);

    if (p_env && p_env->model_id == MM_ID_GENC_LOC)
    {
        // Pointer to the buffer that will contain the message
        mesh_buf_t *p_buf_set = mm_route_buf_alloc(MM_GEN_LOC_SET_GLOB_LEN);

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
            p_buf_env->opcode = GETB(set_info, MM_SET_INFO_ACK)
                                            ? MM_MSG_GEN_LOCG_SET : MM_MSG_GEN_LOCG_SET_UNACK;

            // Fill the message
            write32p(p_data + MM_GEN_LOC_SET_GLOB_LAT_POS, latitude);
            write32p(p_data + MM_GEN_LOC_SET_GLOB_LONG_POS, longitude);
            write16p(p_data + MM_GEN_LOC_SET_GLOB_ALT_POS, altitude);

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

uint8_t mm_genc_locl_set(m_lid_t mdl_lid, m_lid_t app_key_lid, uint16_t dst,
                               uint8_t set_info, int16_t north, int16_t east, int16_t altitude,
                               uint8_t floor, uint16_t uncertainty)
{
    uint8_t status = MESH_ERR_NO_ERROR;
    mm_mdl_env_t *p_env = mm_state_get_env(mdl_lid);

    if (p_env && p_env->model_id == MM_ID_GENC_LOC)
    {
        // Pointer to the buffer that will contain the message
        mesh_buf_t *p_buf_set = mm_route_buf_alloc(MM_GEN_LOC_SET_LOC_LEN);

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
            p_buf_env->opcode = GETB(set_info, MM_SET_INFO_ACK)
                                            ? MM_MSG_GEN_LOCL_SET : MM_MSG_GEN_LOCL_SET_UNACK;

            // Fill the message
            write16p(p_data + MM_GEN_LOC_SET_LOC_NORTH_POS, north);
            write16p(p_data + MM_GEN_LOC_SET_LOC_EAST_POS, east);
            write16p(p_data + MM_GEN_LOC_SET_LOC_ALT_POS, altitude);
            *(p_data + MM_GEN_LOC_SET_LOC_FLOOR_POS) = floor;
            write16p(p_data + MM_GEN_LOC_SET_LOC_UNCERT_POS, uncertainty);

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

/// @} end of group
