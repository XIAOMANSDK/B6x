/**
 ****************************************************************************************
 * @file mm_gens_bat.c
 *
 * @brief Mesh Model Generic Battery Server Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_GENS_BAT
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

/// Information bit field - base on Bit[7:4] of mm_info
/// 8     7          6     5       4
/// +-----+----------+-----+-------+
/// | RFU | Wait App | RFU | Publi |
/// +-----+----------+-----+-------+
enum mm_gens_bat_info
{
    /// Publication to be sent after confirmation received from application
    MM_GENS_BAT_INFO_PUBLI_POS      = 4,
    MM_GENS_BAT_INFO_PUBLI_BIT      = (1 << MM_GENS_BAT_INFO_PUBLI_POS),

    /// Wait for confirmation from application
    MM_GENS_BAT_INFO_WAIT_APP_POS   = 6,
    MM_GENS_BAT_INFO_WAIT_APP_BIT   = (1 << MM_GENS_BAT_INFO_WAIT_APP_POS),
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Structure for Generic Battery Server model environment
typedef struct mm_gens_bat_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
    /// Timer for sending of publications
    mesh_timer_t tmr_publi;
    /// Publication period
    uint32_t publi_period_ms;

    /// List of prepared Generic Battery Status messages
    list_t list_status;

    /// Information (@see enum mm_gens_bat_info) - combine to env.info
    //uint8_t info;
} mm_gens_bat_env_t;

/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Prepare and send a Generic Battery Status message
 *
 * @param[in] p_env_bat          Pointer to Generic Battery Server model environment
 ****************************************************************************************
 */
__STATIC void mm_gens_bat_send_status(mm_gens_bat_env_t *p_env_bat, mesh_buf_t *p_buf,
                                      uint8_t bat_lvl, uint32_t time_discharge, uint32_t time_charge,
                                      uint8_t flags)
{
    // Status
    //uint8_t status;

    if (!p_buf)
    {
        p_buf = mm_route_buf_alloc(MM_GEN_BAT_STATUS_LEN);
    }

    if (p_buf)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Get pointer to environment
        mm_route_env_t *p_env = (mm_route_env_t *)&p_buf->env[0];

        p_env->opcode = MM_MSG_GEN_BAT_STATUS;
        SETB(p_env->info, MM_ROUTE_INFO_PUBLISH, (p_buf == NULL));
        SETB(p_env->info, MM_ROUTE_INFO_RX, 0);

        // Fill the message
        *(p_data + MM_GEN_BAT_STATUS_LEVEL_POS) = bat_lvl;
        write24p(p_data + MM_GEN_BAT_STATUS_TIME_DISCHRG_POS, time_discharge);
        write24p(p_data + MM_GEN_BAT_STATUS_TIME_CHRG_POS, time_charge);
        *(p_data + MM_GEN_BAT_STATUS_FLAGS_POS) = flags;

        // Send the message
        mm_route_send(p_buf);
    }
}

/**
 ****************************************************************************************
 * @brief Inform application that current Generic Battery State is required
 *
 * @param[in] p_env_bat          Pointer to Generic Battery Server model environment
 ****************************************************************************************
 */
__STATIC void mm_gens_bat_send_req_ind(mm_gens_bat_env_t *p_env_bat)
{
    // Check that a confirmation is not already expected from application
    if (!GETB(p_env_bat->env.info, MM_GENS_BAT_INFO_WAIT_APP))
    {
        SETB(p_env_bat->env.info, MM_GENS_BAT_INFO_WAIT_APP, 1);
        mm_srv_state_req_ind_send(MM_SRV_BAT_REQ_IND, p_env_bat->env.elmt_idx);
    }
}

/**
 ****************************************************************************************
 * @brief Look for buffer prepared for transmission of Generic Battery Status message to a given
 * node's element
 *
 * @param[in] p_env_bat     Pointer to environment allocated for the Generic Battery Server model
 * @param[in] src           Address of node's element
 ****************************************************************************************
 */
__STATIC bool mm_gens_bat_find_get(mm_gens_bat_env_t *p_env_bat, uint16_t src)
{
    // Buffer with same source address has been found
    bool found = false;
    // Get first buffer
    mesh_buf_t *p_buf = (mesh_buf_t *)list_pick(&p_env_bat->list_status);

    while (p_buf)
    {
        // Read environment
        mm_route_env_t *p_env = (mm_route_env_t *)&p_buf->env[0];

        if (p_env->u_addr.src == src)
        {
            found = true;
            break;
        }

        // Get next buffer
        p_buf = (mesh_buf_t *)p_buf->hdr.next;
    }

    return (found);
}

/**
 ****************************************************************************************
 * @brief Prepare a buffer for transmission of Generic Battery Status message
 *
 * @param[in] p_env_bat          Pointer to Generic Battery Server model environment
 * @param[in] p_route_env        Information about received Generic Battery Get message
 ****************************************************************************************
 */
__STATIC void mm_gens_bat_prepare_status(mm_gens_bat_env_t *p_env_bat,
                                         mm_route_env_t *p_route_env)
{
    // Check that a Generic Battery Get message from the same source address has not already
    // been received
    if (!mm_gens_bat_find_get(p_env_bat, p_route_env->u_addr.src))
    {
        // Buffer that will contain the Generic Battery Status message
        mesh_buf_t *p_buf_status = mm_route_buf_alloc(MM_GEN_BAT_STATUS_LEN);

        if (p_buf_status)
        {
            // Copy the received environment
            memcpy(&p_buf_status->env[0], p_route_env, sizeof(mm_route_env_t));

            // Insert the buffer in the list of received get messages
            list_push_back(&p_env_bat->list_status, &p_buf_status->hdr);
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
 * Generic Battery Server model expires
 *
 * @param[in] p_env     Pointer to model environment for Generic Battery Server model
 ****************************************************************************************
 */
__STATIC void mm_gens_bat_cb_tmr_publi(void *p_tmr)
{
    // Get allocated environment
    mm_gens_bat_env_t *p_env_bat = MESH_TMR2ENV(p_tmr, mm_gens_bat_env_t, tmr_publi);

    if (p_env_bat->publi_period_ms)
    {
        // Keep in mind that a publication must be sent
        SETB(p_env_bat->env.info, MM_GENS_BAT_INFO_PUBLI, 1);

        // Retrieve current Generic Battery state from the application
        mm_gens_bat_send_req_ind(p_env_bat);

        // Restart the timer
        mesh_timer_set(&p_env_bat->tmr_publi, p_env_bat->publi_period_ms);
    }
}

/**
 ****************************************************************************************
 * @brief Inform Generic Battery Server model about a received message
 *
 * @param[in] p_env         Pointer to the environment allocated for the Generic Battery
 * Server model
 * @param[in] p_buf         Pointer to the buffer containing the received message
 * @param[in] p_route_env   Pointer to structure containing reception parameters provided
 * by the Mesh Profile block
 ****************************************************************************************
 */
__STATIC void mm_gens_bat_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                                mm_route_env_t *p_route_env)
{
    do
    {
        // Get environment for Generic Battery Server model
        mm_gens_bat_env_t *p_env_bat = (mm_gens_bat_env_t *)p_env;

        // Prepare buffer for transmission of Generic Battery Status message
        mm_gens_bat_prepare_status(p_env_bat, p_route_env);

        // Retrieve current Generic Battery state from the application
        mm_gens_bat_send_req_ind(p_env_bat);
    } while (0);
}

/**
 ****************************************************************************************
 * @brief Check if a given opcode is handled by the Generic Battery Server model
 *
 * @param[in] p_env         Pointer to the environment allocated for the Generic Battery
 * Server model
 * @param[in] opcode        Opcode to check
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_gens_bat_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if (opcode == MM_MSG_GEN_BAT_GET)
    {
        status = MESH_ERR_NO_ERROR;
    }
    else
    {
        status = MESH_ERR_MDL_INVALID_OPCODE;
    }

    return (status);
}

/*
 * GLOBAL FUNCTIONS
 ****************************************************************************************
 */

uint8_t mm_gens_bat_register(uint8_t elmt_idx)
{
    //uint8_t status = MESH_ERR_COMMAND_DISALLOWED;
    // Register the model
    m_lid_t mdl_lid = ms_register_model(MM_ID_GENS_BAT, elmt_idx, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID) //if (status == MESH_ERR_NO_ERROR)
    {
        // Inform the Model State Manager about registered model
        mm_gens_bat_env_t *p_env_bat = (mm_gens_bat_env_t *)mm_state_register(elmt_idx, MM_ID_GENS_BAT, mdl_lid,
                                      MM_ROLE_SRV_PUBLI, sizeof(mm_gens_bat_env_t));

        if (p_env_bat)
        {
            // Prepare timer for publications
            p_env_bat->tmr_publi.cb = mm_gens_bat_cb_tmr_publi;
            //p_env_bat->tmr_publi.p_env = (void *)p_env_bat;

            // Set internal callback functions
            p_env_bat->env.mdl_cb.cb_rx = mm_gens_bat_cb_rx;
            p_env_bat->env.mdl_cb.cb_opcode_check = mm_gens_bat_cb_opcode_check;
            //p_env_bat->env.mdl_cb.cb_publish_param = mm_gens_bat_cb_publish_param;

            //status = MESH_ERR_NO_ERROR;
            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENS_BAT, elmt_idx, mdl_lid);
        }
    }

    return (mdl_lid);
}

void mm_gens_bat_cfm(uint8_t elmt_idx, uint8_t status, uint8_t bat_lvl,
                     uint32_t time_discharge, uint32_t time_charge, uint8_t flags)
{
    // Look for Generic Battery Server model in the list of models
    m_lid_t mdl_lid = mm_state_get_lid(elmt_idx, MM_ID_GENS_BAT);

    if (mdl_lid != MESH_INVALID_LID)
    {
        mm_gens_bat_env_t *p_env_bat = (mm_gens_bat_env_t *)mm_state_get_env(mdl_lid);

        // Check if confirmation from application was expected
        if (GETB(p_env_bat->env.info, MM_GENS_BAT_INFO_WAIT_APP))
        {
            SETB(p_env_bat->env.info, MM_GENS_BAT_INFO_WAIT_APP, 0);

            if (status != MESH_ERR_NO_ERROR)
            {
                bat_lvl = MM_BAT_LVL_UNKNOWN;
                time_discharge = MM_BAT_TIME_DISCHRG_UNKNOWN;
                time_charge = MM_BAT_TIME_CHRG_UNKNOWN;
                flags = MM_BAT_FLAGS_UNKNOWN;
            }
            else
            {
                // Check the provided values
                if (bat_lvl > MM_BAT_LVL_MAX)
                {
                    bat_lvl = MM_BAT_LVL_UNKNOWN;
                }

                if (time_discharge > MM_BAT_TIME_DISCHRG_UNKNOWN)
                {
                    time_discharge = MM_BAT_TIME_DISCHRG_UNKNOWN;
                }

                if (time_charge > MM_BAT_TIME_CHRG_UNKNOWN)
                {
                    time_charge = MM_BAT_TIME_CHRG_UNKNOWN;
                }
            }

            // Send responses for received get requests
            while (!list_is_empty(&p_env_bat->list_status))
            {
                mesh_buf_t *p_buf = (mesh_buf_t *)list_pop_front(&p_env_bat->list_status);

                mm_gens_bat_send_status(p_env_bat, p_buf,
                                        bat_lvl, time_discharge, time_charge, flags);
            }

            // Send a publication if needed
            if (GETB(p_env_bat->env.info, MM_GENS_BAT_INFO_PUBLI))
            {
                SETB(p_env_bat->env.info, MM_GENS_BAT_INFO_PUBLI, 0);

                mm_gens_bat_send_status(p_env_bat, NULL,
                                        bat_lvl, time_discharge, time_charge, flags);
            }
        }
    }
}

/// @} end of group
