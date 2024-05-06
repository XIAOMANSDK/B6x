/**
 ****************************************************************************************
 *
 * @file mm_genc_poo.c
 *
 * @brief Mesh Model Generic Power OnOff Client Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_GENC_POO
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

/// Structure for Generic Power OnOff Client model environment
typedef struct mm_genc_poo_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
} mm_genc_poo_env_t;

/*
 * INTERNAL CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Inform Generic Power OnOff Client model about reception of a message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] p_route_env   Pointer to routing environment containing information about the
 * received message
 ****************************************************************************************
 */
__STATIC void mm_genc_poo_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                                mm_route_env_t *p_route_env)
{
    if (p_route_env->opcode == MM_MSG_GEN_ONPUP_STATUS)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Generic OnPowerUp state value
        uint8_t on_power_up = *(p_data + MM_GEN_POO_SET_OPU_POS);

        // Inform the application about the received Generic OnPowerUp state value
        mm_cli_state_ind_send(p_route_env->u_addr.src, MM_STATE_GEN_ONPOWERUP, on_power_up, 0, 0);
    }
}

/**
 ****************************************************************************************
 * @brief Inform Generic Power OnOff Client model about a received opcode in order
 * to check if the model is authorized to handle the associated message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] opcode        Opcode value to be checked
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_poo_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if (opcode == MM_MSG_GEN_ONPUP_STATUS)
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
 * @brief Send a Generic OnPowerUp Get message to a given node's element
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] dst           Address of node's element to which message will be sent
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_poo_cb_get(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst, \
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
        p_buf_env->opcode = MM_MSG_GEN_ONPUP_GET;

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
 * @brief Set Generic OnPowerUp state value on a given node's element by
 * sending either a Generic OnPowerUp Set or a Generic OnPowerUp Set Unacknowledged message
 *
 * @param[in] p_env             Pointer to the environment allocated for the model
 * @param[in] dst               Address of node's element to which message will be sent
 * @param[in] state_1           New Generic OnPowerUp state value
 * @param[in] state_2           N/A
 * @param[in] set_info          Transition information (@see enum mm_set_info)
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_poo_cb_set(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                     uint32_t state_1, uint32_t state_2,
                                     uint16_t set_info)
{
    uint8_t status = MESH_ERR_NO_ERROR;
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_set = mm_route_buf_alloc(MM_GEN_POO_SET_LEN);

    if (p_buf_set)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf_set);
        // Get pointer to environment
        mm_route_env_t *p_buf_env = (mm_route_env_t *)&p_buf_set->env;
        // Generic OnPowerUp state value
        uint8_t on_power_up = state_1;

        // Prepare environment
        p_buf_env->app_key_lid = app_key_lid;
        p_buf_env->u_addr.dst = dst;
        p_buf_env->info = 0;
        p_buf_env->mdl_lid = p_env->mdl_lid;
        p_buf_env->opcode = GETB(set_info, MM_SET_INFO_ACK)
                                        ? MM_MSG_GEN_ONPUP_SET : MM_MSG_GEN_ONPUP_SET_UNACK;

        // Fill the message
        *(p_data + MM_GEN_POO_SET_OPU_POS) = on_power_up;

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

__STATIC mm_cli_cb_t mm_genc_poo_cb = 
{
    .cb_get = mm_genc_poo_cb_get,
    .cb_set = mm_genc_poo_cb_set,
    .cb_trans = NULL,
};

uint8_t mm_genc_poo_register(void)
{
    // Register the model
    m_lid_t mdl_lid = ms_register_model(MM_ID_GENC_POO, 0, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID) //if (status == MESH_ERR_NO_ERROR)
    {
        // Inform the Model State Manager about registered model
        mm_genc_poo_env_t *p_env_poo = (mm_genc_poo_env_t *)mm_state_register(0, MM_ID_GENC_POO, mdl_lid,
                                        MM_ROLE_CLI, sizeof(mm_genc_poo_env_t));

        if (p_env_poo)
        {
            // Get client-specific callback functions
            //mm_cli_cb_t *p_cb_cli = p_env_poo->env.cb.u.p_cb_cli;

            // Set internal callback functions
            p_env_poo->env.mdl_cb.cb_rx = mm_genc_poo_cb_rx;
            p_env_poo->env.mdl_cb.cb_opcode_check = mm_genc_poo_cb_opcode_check;
            //p_cb_cli->cb_get = mm_genc_poo_cb_get;
            //p_cb_cli->cb_set = mm_genc_poo_cb_set;
            p_env_poo->env.mdl_cb.p_cb_cli = &mm_genc_poo_cb;

            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENC_POO, 0, mdl_lid);
        }
    }

    return (mdl_lid);
}

/// @} end of group
