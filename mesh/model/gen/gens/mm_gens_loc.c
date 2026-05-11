/**
 ****************************************************************************************
 * @file mm_gens_loc.c
 *
 * @brief Mesh Model Generic Location Server Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_GENS_LOC
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
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Information bit field  - base on Bit[7:4] of mm_info
/// 8              7               6           5            4
/// +--------------+---------------+-----------+------------+
/// | Wait App Loc | Wait App Glob | Publi Loc | Publi Glob |
/// +--------------+---------------+-----------+------------+
enum mm_gens_loc_info
{
    /// Publication for Generic Location state (global part) to be sent after
    /// confirmation received from application
    MM_GENS_LOC_INFO_PUBLI_GLOB_POS     = 4,
    MM_GENS_LOC_INFO_PUBLI_GLOB_BIT     = (1 << MM_GENS_LOC_INFO_PUBLI_GLOB_POS),

    /// Publication for Generic Location state (local part) to be sent after
    /// confirmation received from application
    MM_GENS_LOC_INFO_PUBLI_LOC_POS      = 5,
    MM_GENS_LOC_INFO_PUBLI_LOC_BIT      = (1 << MM_GENS_LOC_INFO_PUBLI_LOC_POS),

    /// Wait for confirmation from application for Generic Location Global Get message
    MM_GENS_LOC_INFO_WAIT_APP_GLOB_POS  = 6,
    MM_GENS_LOC_INFO_WAIT_APP_GLOB_BIT  = (1 << MM_GENS_LOC_INFO_WAIT_APP_GLOB_POS),

    /// Wait for confirmation from application for Generic Location Local Get message
    MM_GENS_LOC_INFO_WAIT_APP_LOC_POS   = 7,
    MM_GENS_LOC_INFO_WAIT_APP_LOC_BIT   = (1 << MM_GENS_LOC_INFO_WAIT_APP_LOC_POS),
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Structure for Generic Location Server model environment
typedef struct mm_gens_loc_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
    /// Timer for sending of publications
    mesh_timer_t tmr_publi;
    /// Publication period
    uint32_t publi_period_ms;

    /// List of prepared Generic Location Global Status messages
    list_t list_status_glob;
    /// List of prepared Generic Location Local Status messages
    list_t list_status_loc;

    /// Information (@see enum mm_gens_loc_info) - combine to env.info
    //uint8_t info;
} mm_gens_loc_env_t;

/// Structure for Generic Location Setup Server model environment
typedef struct mm_gens_locs_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
    /// Pointer to environment of associated Generic Location Server model
    mm_gens_loc_env_t *p_env_loc;
} mm_gens_locs_env_t;

/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
__STATIC mesh_buf_t *mm_gens_loc_find_buf(list_t *p_list, uint16_t src, bool extract)
{
    // Get first buffer
    mesh_buf_t *p_buf = (mesh_buf_t *)list_pick(p_list);

    while (p_buf)
    {
        // Read environment
        mm_route_env_t *p_env = (mm_route_env_t *)&p_buf->env[0];

        if (p_env->u_addr.src == src)
        {
            // Extract the buffer if required
            if (extract)
            {
                list_extract(p_list, &(p_buf->hdr));
            }

            break;
        }

        // Get next buffer
        p_buf = (mesh_buf_t *)p_buf->hdr.next;
    }

    return (p_buf);
}

/**
 ****************************************************************************************
 * @brief Prepare and send a Generic Location Global Status message
 *
 * @param[in] p_env_loc          Pointer to Generic Location Server model environment
 ****************************************************************************************
 */
__STATIC void mm_gens_loc_send_status_global(mm_gens_loc_env_t *p_env_loc, mesh_buf_t *p_buf,
                                             mm_route_env_t *p_route_env,
                                             int32_t latitude, int32_t longitude, int16_t altitude)
{
    // Status
    //uint8_t status;

    if (!p_buf)
    {
        p_buf = mm_route_buf_alloc(MM_GEN_LOC_STATUS_GLOB_LEN);
    }

    if (p_buf)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Get pointer to environment
        mm_route_env_t *p_env = (mm_route_env_t *)&p_buf->env[0];

        if (p_route_env)
        {
            memcpy(p_env, p_route_env, sizeof(mm_route_env_t));
        }

        p_env->opcode = MM_MSG_GEN_LOCG_STATUS;
        p_env->mdl_lid = p_env_loc->env.mdl_lid;
        SETB(p_env->info, MM_ROUTE_INFO_PUBLISH, (p_buf == NULL) && (p_route_env == NULL));
        SETB(p_env->info, MM_ROUTE_INFO_RX, 0);

        // Fill the message
        write32p(p_data + MM_GEN_LOC_STATUS_GLOB_LAT_POS, latitude);
        write32p(p_data + MM_GEN_LOC_STATUS_GLOB_LONG_POS, longitude);
        write16p(p_data + MM_GEN_LOC_STATUS_GLOB_ALT_POS, altitude);

        // Send the message
        mm_route_send(p_buf);
    }
}

/**
 ****************************************************************************************
 * @brief Prepare and send a Generic Location Local Status message
 *
 * @param[in] p_env_loc          Pointer to Generic Location Server model environment
 ****************************************************************************************
 */
__STATIC void mm_gens_loc_send_status_local(mm_gens_loc_env_t *p_env_loc, mesh_buf_t *p_buf,
                                            mm_route_env_t *p_route_env,
                                            int16_t north, int16_t east, int16_t altitude, uint8_t floor,
                                            uint16_t uncertainty)
{
    // Status
    //uint8_t status;

    if (!p_buf)
    {
        p_buf = mm_route_buf_alloc(MM_GEN_LOC_STATUS_LOC_LEN);
    }

    if (p_buf)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Get pointer to environment
        mm_route_env_t *p_env = (mm_route_env_t *)&p_buf->env[0];

        if (p_route_env)
        {
            memcpy(p_env, p_route_env, sizeof(mm_route_env_t));
        }

        p_env->opcode = MM_MSG_GEN_LOCL_STATUS;
        p_env->mdl_lid = p_env_loc->env.mdl_lid;
        SETB(p_env->info, MM_ROUTE_INFO_PUBLISH, (p_buf == NULL) && (p_route_env == NULL));
        SETB(p_env->info, MM_ROUTE_INFO_RX, 0);

        // Fill the message
        write16p(p_data + MM_GEN_LOC_STATUS_LOC_NORTH_POS, north);
        write16p(p_data + MM_GEN_LOC_STATUS_LOC_EAST_POS, east);
        write16p(p_data + MM_GEN_LOC_STATUS_LOC_ALT_POS, altitude);
        *(p_data + MM_GEN_LOC_STATUS_LOC_FLOOR_POS) = floor;
        write16p(p_data + MM_GEN_LOC_STATUS_LOC_UNCERT_POS, uncertainty);

        // Send the message
        mm_route_send(p_buf);
    }
}

/**
 ****************************************************************************************
 * @brief Inform application that current Generic Location State is required (global part)
 *
 * @param[in] p_env_loc          Pointer to Generic Location Server model environment
 ****************************************************************************************
 */
__STATIC void mm_gens_loc_send_global_req_ind(mm_gens_loc_env_t *p_env_loc)
{
    // Check that a confirmation is not already expected from application
    if (!GETB(p_env_loc->env.info, MM_GENS_LOC_INFO_WAIT_APP_GLOB))
    {
        SETB(p_env_loc->env.info, MM_GENS_LOC_INFO_WAIT_APP_GLOB, 1);
        mm_srv_state_req_ind_send(MM_SRV_LOCG_REQ_IND, p_env_loc->env.elmt_idx);
    }
}

/**
 ****************************************************************************************
 * @brief Inform application that current Generic Location State is required (local part)
 *
 * @param[in] p_env_loc          Pointer to Generic Location Server model environment
 ****************************************************************************************
 */
__STATIC void mm_gens_loc_send_local_req_ind(mm_gens_loc_env_t *p_env_loc)
{
    // Check that a confirmation is not already expected from application
    if (!GETB(p_env_loc->env.info, MM_GENS_LOC_INFO_WAIT_APP_LOC))
    {
        SETB(p_env_loc->env.info, MM_GENS_LOC_INFO_WAIT_APP_LOC, 1);
        mm_srv_state_req_ind_send(MM_SRV_LOCL_REQ_IND, p_env_loc->env.elmt_idx);
    }
}

/**
 ****************************************************************************************
 * @brief Handler for Generic Location Global Set/Set Unacknowledged message
 *
 * @param[in] p_env_loc     Pointer to Generic Location Server model environment
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] opcode        Received operation code
 ****************************************************************************************
 */
__STATIC void mm_gens_loc_handler_set_global(mm_gens_loc_env_t *p_env_loc, mesh_buf_t *p_buf,
                                             mm_route_env_t *p_route_env)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Global Latitude
    int32_t latitude = (int32_t)read32p(p_data + MM_GEN_LOC_SET_GLOB_LAT_POS);
    // Global Longitude
    int32_t longitude = (int32_t)read32p(p_data + MM_GEN_LOC_SET_GLOB_LONG_POS);
    // Global Altitude
    int16_t altitude = (int16_t)read16p(p_data + MM_GEN_LOC_SET_GLOB_ALT_POS);

    // Inform application about new state value
    mm_srv_locg_upd_ind_send(p_env_loc->env.elmt_idx, latitude, longitude, altitude);

    if (p_route_env->opcode == MM_MSG_GEN_LOCG_SET)
    {
        // Send a Generic Location Global Status message
        mm_gens_loc_send_status_global(p_env_loc, NULL, p_route_env, latitude, longitude, altitude);
    }
}

/**
 ****************************************************************************************
 * @brief Handler for Generic Location Local Set/Set Unacknowledged message
 *
 * @param[in] p_env_loc     Pointer to Generic Location Server model environment
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] opcode        Received operation code
 ****************************************************************************************
 */
__STATIC void mm_gens_loc_handler_set_local(mm_gens_loc_env_t *p_env_loc, mesh_buf_t *p_buf,
                                            mm_route_env_t *p_route_env)
{
    // Get pointer to data
    uint8_t *p_data = MESH_BUF_DATA(p_buf);
    // Local North
    int16_t north = (int16_t)read16p(p_data + MM_GEN_LOC_SET_LOC_NORTH_POS);
    // Local East
    int16_t east = (int16_t)read16p(p_data + MM_GEN_LOC_SET_LOC_EAST_POS);
    // Global Altitude
    int16_t altitude = (int16_t)read16p(p_data + MM_GEN_LOC_SET_LOC_ALT_POS);
    // Floor
    uint8_t floor = *(p_data + MM_GEN_LOC_SET_LOC_FLOOR_POS);
    // Uncertainty
    uint16_t uncertainty = read16p(p_data + MM_GEN_LOC_SET_LOC_UNCERT_POS);

    // Inform application about new state value
    mm_srv_locl_upd_ind_send(p_env_loc->env.elmt_idx, north, east, altitude,
                                  floor, uncertainty);

    if (p_route_env->opcode == MM_MSG_GEN_LOCL_SET)
    {
        // Send a Generic Location Local Status message
        mm_gens_loc_send_status_local(p_env_loc, NULL, p_route_env, north, east, altitude,
                                      floor, uncertainty);
    }
}

/**
 ****************************************************************************************
 * @brief Prepare a buffer for transmission of Generic Location Global Status or Generic
 * Location Local Status message after reception of a get message
 *
 * @param[in] p_env_loc          Pointer to Generic Location Server model environment
 * @param[in] p_route_env        Information about received Generic Location Global/Local Get message
 * @param[in] global             True if Generic Location Global Get message has been received, else False
 ****************************************************************************************
 */
__STATIC void mm_gens_loc_prepare_status(mm_gens_loc_env_t *p_env_loc,
                                         mm_route_env_t *p_route_env, bool global)
{
    // Pointer to the list of received get messages to parse
    list_t *p_list = (global) ? &p_env_loc->list_status_glob : &p_env_loc->list_status_loc;

    // Check that a Generic Location Global/Local Get message from the same source address has not already
    // been received
    if (!mm_gens_loc_find_buf(p_list, p_route_env->u_addr.src, false))
    {
        // Buffer that will contain the Generic Location Global/Local Status message
        mesh_buf_t *p_buf_status = mm_route_buf_alloc((global) ? MM_GEN_LOC_STATUS_GLOB_LEN
                                                      : MM_GEN_LOC_STATUS_LOC_LEN);

        if (p_buf_status)
        {
            // Copy the received environment
            memcpy(&p_buf_status->env[0], p_route_env, sizeof(mm_route_env_t));

            // Insert the buffer in the list of received get messages
            list_push_back(p_list, &p_buf_status->hdr);

            // Retrieve the Generic Location state from the application
            if (global)
            {
                mm_gens_loc_send_global_req_ind(p_env_loc);
            }
            else
            {
                mm_gens_loc_send_local_req_ind(p_env_loc);
            }
        }
    }
}

/*
 * INTERNAL CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Callback function called when timer monitoring publication duration for
 * Generic Location Server model expires
 *
 * @param[in] p_env     Pointer to model environment for Generic Location Server model
 ****************************************************************************************
 */
__STATIC void mm_gens_loc_cb_tmr_publi(void *p_tmr)
{
    // Get allocated environment
    mm_gens_loc_env_t *p_env_loc = MESH_TMR2ENV(p_tmr, mm_gens_loc_env_t, tmr_publi);

    if (p_env_loc->publi_period_ms)
    {
        // Keep in mind that a publication must be sent
        SETB(p_env_loc->env.info, MM_GENS_LOC_INFO_PUBLI_LOC, 1);
        SETB(p_env_loc->env.info, MM_GENS_LOC_INFO_PUBLI_GLOB, 1);

        // Retrieve current Generic Location state (Global part) from the application
        mm_gens_loc_send_global_req_ind(p_env_loc);
        // Retrieve current Generic Location state (Location part) from the application
        mm_gens_loc_send_local_req_ind(p_env_loc);

        // Restart the timer
        mesh_timer_set(&p_env_loc->tmr_publi, p_env_loc->publi_period_ms);
    }
}

/**
 ****************************************************************************************
 * @brief Inform Generic Location Server model about a received message
 *
 * @param[in] p_env         Pointer to the environment allocated for the Generic Location
 * Server model
 * @param[in] p_buf         Pointer to the buffer containing the received message
 * @param[in] p_route_env   Pointer to structure containing reception parameters provided
 * by the Mesh Profile block
 ****************************************************************************************
 */
__STATIC void mm_gens_loc_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf, mm_route_env_t *p_route_env)
{
    // Environment for Generic Location Server model
    mm_gens_loc_env_t *p_env_loc;

    if ((p_route_env->opcode == MM_MSG_GEN_LOCG_GET)
            || (p_route_env->opcode == MM_MSG_GEN_LOCL_GET))
    {
        p_env_loc = (mm_gens_loc_env_t *)p_env;

        // Prepare environment for transition of status message
        mm_gens_loc_prepare_status(p_env_loc, p_route_env,
                                   (p_route_env->opcode == MM_MSG_GEN_LOCG_GET));
    }
    else
    {
        // Environment for Generic Location Setup Server model
        mm_gens_locs_env_t *p_env_locs = (mm_gens_locs_env_t *)p_env;

        p_env_loc = p_env_locs->p_env_loc;

        if ((p_route_env->opcode == MM_MSG_GEN_LOCG_SET)
                || (p_route_env->opcode == MM_MSG_GEN_LOCG_SET_UNACK))
        {
            // Handle the received message
            mm_gens_loc_handler_set_global(p_env_loc, p_buf, p_route_env);
        }
        else
        {
            // Handle the receive message
            mm_gens_loc_handler_set_local(p_env_loc, p_buf, p_route_env);
        }
    }
}

/**
 ****************************************************************************************
 * @brief Check if a given opcode is handled by the Generic Location Server model
 *
 * @param[in] p_env         Pointer to the environment allocated for the Generic Location
 * Server model
 * @param[in] opcode        Opcode to check
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_gens_loc_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status = MESH_ERR_MDL_INVALID_OPCODE;

    if (p_env->model_id == MM_ID_GENS_LOC)
    {
        if ((opcode == MM_MSG_GEN_LOCG_GET)
                || (opcode == MM_MSG_GEN_LOCL_GET))
        {
            status = MESH_ERR_NO_ERROR;
        }
    }
    else if (p_env->model_id == MM_ID_GENS_LOCS)
    {
        if ((opcode == MM_MSG_GEN_LOCG_SET)
                || (opcode == MM_MSG_GEN_LOCG_SET_UNACK)
                || (opcode == MM_MSG_GEN_LOCL_SET)
                || (opcode == MM_MSG_GEN_LOCL_SET_UNACK))
        {
            status = MESH_ERR_NO_ERROR;
        }
    }

    return (status);
}

/*
 * GLOBAL FUNCTIONS
 ****************************************************************************************
 */

uint8_t mm_gens_loc_register(uint8_t elmt_idx)
{
    // Register Generic Location Server model
    m_lid_t mdl_lid = ms_register_model(MM_ID_GENS_LOC, elmt_idx, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID)
    {
        // Inform the Model State Manager about registered model
        mm_gens_loc_env_t *p_env_loc = (mm_gens_loc_env_t *)mm_state_register(elmt_idx, MM_ID_GENS_LOC, mdl_lid,
                                      MM_ROLE_SRV_PUBLI, sizeof(mm_gens_loc_env_t));

        if (p_env_loc)
        {
            // Set internal callback functions
            p_env_loc->tmr_publi.cb = mm_gens_loc_cb_tmr_publi;
            //p_env_loc->tmr_publi.p_env = (void *)p_env_loc;

            // Set internal callback functions
            p_env_loc->env.mdl_cb.cb_rx = mm_gens_loc_cb_rx;
            p_env_loc->env.mdl_cb.cb_opcode_check = mm_gens_loc_cb_opcode_check;
            //p_env_loc->env.mdl_cb.cb_publish_param = mm_gens_loc_cb_publish_param;

            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENS_LOC, elmt_idx, mdl_lid);
        }

        // Register Generic Location Setup Server model
        m_lid_t locs_lid = ms_register_model(MM_ID_GENS_LOCS, elmt_idx, 0);

        if (locs_lid != MESH_INVALID_LID)
        {
            // Inform the Model State Manager about registered model
            mm_gens_locs_env_t *p_env_locs = (mm_gens_locs_env_t *)mm_state_register(elmt_idx, MM_ID_GENS_LOCS, locs_lid,
                                          MM_ROLE_SRV, sizeof(mm_gens_locs_env_t));

            if (p_env_locs)
            {
                // Set internal callback functions
                p_env_locs->env.mdl_cb.cb_rx = mm_gens_loc_cb_rx;
                p_env_locs->env.mdl_cb.cb_opcode_check = mm_gens_loc_cb_opcode_check;

                // Link environment
                p_env_locs->p_env_loc = p_env_loc;

                // Inform application about registered model
                mm_register_ind_send(MM_ID_GENS_LOCS, elmt_idx, locs_lid);
            }
        }
    }

    return (mdl_lid);
}

void mm_gens_locg_cfm(uint8_t elmt_idx, uint8_t status, int32_t latitude,
                            int32_t longitude, int16_t altitude)
{
    // Look for Generic Location Server model in the list of models
    m_lid_t mdl_lid = mm_state_get_lid(elmt_idx, MM_ID_GENS_LOC);

    if (mdl_lid != MESH_INVALID_LID)
    {
        mm_gens_loc_env_t *p_env_loc = (mm_gens_loc_env_t *)mm_state_get_env(mdl_lid);

        // Check if confirmation from application was expected
        if (GETB(p_env_loc->env.info, MM_GENS_LOC_INFO_WAIT_APP_GLOB))
        {
            SETB(p_env_loc->env.info, MM_GENS_LOC_INFO_WAIT_APP_GLOB, 0);

            if (status != MESH_ERR_NO_ERROR)
            {
                latitude = MM_LOC_GLOBAL_LAT_NOT_CONFIG;
                longitude = MM_LOC_GLOBAL_LONG_NOT_CONFIG;
                altitude = MM_LOC_GLOBAL_ALT_NOT_CONFIG;
            }

            // Send responses for received get requests
            while (!list_is_empty(&p_env_loc->list_status_glob))
            {
                mesh_buf_t *p_buf = (mesh_buf_t *)list_pop_front(&p_env_loc->list_status_glob);

                mm_gens_loc_send_status_global(p_env_loc, p_buf, NULL,
                                               latitude, longitude, altitude);
            }

            // Send a publication if needed
            if (GETB(p_env_loc->env.info, MM_GENS_LOC_INFO_PUBLI_GLOB))
            {
                SETB(p_env_loc->env.info, MM_GENS_LOC_INFO_PUBLI_GLOB, 0);

                mm_gens_loc_send_status_global(p_env_loc, NULL, NULL,
                                               latitude, longitude, altitude);
            }
        }
    }
}

void mm_gens_locl_cfm(uint8_t elmt_idx, uint8_t status, int16_t north,
                           int16_t east, int16_t altitude, uint8_t floor, uint16_t uncertainty)
{
    // Look for Generic Location Server model in the list of models
    m_lid_t mdl_lid = mm_state_get_lid(elmt_idx, MM_ID_GENS_LOC);

    if (mdl_lid != MESH_INVALID_LID)
    {
        mm_gens_loc_env_t *p_env_loc = (mm_gens_loc_env_t *)mm_state_get_env(mdl_lid);

        // Check if confirmation from application was expected
        if (GETB(p_env_loc->env.info, MM_GENS_LOC_INFO_WAIT_APP_LOC))
        {
            SETB(p_env_loc->env.info, MM_GENS_LOC_INFO_WAIT_APP_LOC, 0);

            if (status != MESH_ERR_NO_ERROR)
            {
                north = MM_LOC_LOCAL_NORTH_NOT_CONFIG;
                east = MM_LOC_LOCAL_EAST_NOT_CONFIG;
                altitude = MM_LOC_LOCAL_ALT_NOT_CONFIG;
                floor = MM_LOC_FLOOR_NOT_CONFIG;
                uncertainty = 0;
            }

            // Send responses for received get requests
            while (!list_is_empty(&p_env_loc->list_status_loc))
            {
                mesh_buf_t *p_buf = (mesh_buf_t *)list_pop_front(&p_env_loc->list_status_loc);

                mm_gens_loc_send_status_local(p_env_loc, p_buf, NULL,
                                              north, east, altitude, floor, uncertainty);
            }

            // Send a publication if needed
            if (GETB(p_env_loc->env.info, MM_GENS_LOC_INFO_PUBLI_LOC))
            {
                SETB(p_env_loc->env.info, MM_GENS_LOC_INFO_PUBLI_LOC, 0);

                mm_gens_loc_send_status_local(p_env_loc, NULL, NULL,
                                              north, east, altitude, floor, uncertainty);
            }
        }
    }
}

/// @} end of group
