/**
 ****************************************************************************************
 *
 * @file mm_genc_bat.c
 *
 * @brief Mesh Model Generic Battery Client Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_GENC_BAT
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

/// Structure for Generic Battery Client model environment
typedef struct mm_genc_bat_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
} mm_genc_bat_env_t;


/*
 * INTERNAL CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Inform Generic Battery Client model about reception of a message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] p_route_env   Pointer to routing environment containing information about the
 * received message
 ****************************************************************************************
 */
__STATIC void mm_genc_bat_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                                mm_route_env_t *p_route_env)
{
    if (p_route_env->opcode == MM_MSG_GEN_BAT_STATUS)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Generic Battery State values
        uint8_t bat_lvl = *(p_data + MM_GEN_BAT_STATUS_LEVEL_POS);
        uint32_t time_discharge = read24p(p_data + MM_GEN_BAT_STATUS_TIME_DISCHRG_POS);
        uint32_t time_charge = read24p(p_data + MM_GEN_BAT_STATUS_TIME_CHRG_POS);
        uint8_t flags = *(p_data + MM_GEN_BAT_STATUS_FLAGS_POS);

        // Inform the application about the received Generic Battery state value
        mm_cli_bat_ind_send(p_route_env->u_addr.src, bat_lvl, time_discharge,
                                time_charge, flags);
    }
}

/**
 ****************************************************************************************
 * @brief Inform Generic Battery Client model about a received opcode in order
 * to check if the model is authorized to handle the associated message
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] opcode        Opcode value to be checked
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_bat_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if (opcode == MM_MSG_GEN_BAT_STATUS)
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
 * @brief Send a Generic Battery Get message to a given node's element
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] dst           Address of node's element to which message will be sent
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_genc_bat_cb_get(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
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
        p_buf_env->opcode = MM_MSG_GEN_BAT_GET;

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

__STATIC mm_cli_cb_t mm_genc_bat_cb = 
{
    .cb_get = mm_genc_bat_cb_get,
    .cb_set = NULL,
    .cb_trans = NULL,
};

uint8_t mm_genc_bat_register(void)
{
    // Model local index
    m_lid_t mdl_lid = ms_register_model(MM_ID_GENC_BAT, 0, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID) //if (status == MESH_ERR_NO_ERROR)
    {
        // Inform the Model State Manager about registered model
        mm_genc_bat_env_t *p_env_bat = (mm_genc_bat_env_t *)mm_state_register(0, MM_ID_GENC_BAT, mdl_lid,
                                        MM_ROLE_CLI, sizeof(mm_genc_bat_env_t));

        if (p_env_bat)
        {
            // Get client-specific callback functions
            //mm_cli_cb_t *p_cb_cli = p_env_bat->env.cb.u.p_cb_cli;

            // Set internal callback functions
            p_env_bat->env.mdl_cb.cb_rx = mm_genc_bat_cb_rx;
            p_env_bat->env.mdl_cb.cb_opcode_check = mm_genc_bat_cb_opcode_check;
            //p_cb_cli->cb_get = mm_genc_bat_cb_get;
            p_env_bat->env.mdl_cb.p_cb_cli = &mm_genc_bat_cb;

            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENC_BAT, 0, mdl_lid);
        }
    }

    return (mdl_lid);
}

/// @} end of group
