/**
 ****************************************************************************************
 * @file mm_gens_prop.c
 *
 * @brief Mesh Model Generic User/Admin/Manufacturer/Client Property Server Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_GENS_PROP
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
 * DEFINES
 ****************************************************************************************
 */

/// Length of Property ID field
#define MM_GENS_PROP_ID_LENGTH          (2)
/// Length of User Access field
#define MM_GENS_PROP_ACCESS_LENGTH      (1)
/// Timeout timer duration in milliseconds (5s)
#define MM_GENS_PROP_TMR_TO_DUR         (5000)

/*
 * ENUMERATION
 ****************************************************************************************
 */

/// Access bitfield content
enum mm_gens_prop_access_bf
{
    /// Readable
    MM_GENS_PROP_ACCESS_READ_POS = 0,
    MM_GENS_PROP_ACCESS_READ_BIT = (1 << MM_GENS_PROP_ACCESS_READ_POS),

    /// Writable
    MM_GENS_PROP_ACCESS_WRITE_POS = 1,
    MM_GENS_PROP_ACCESS_WRITE_BIT = (1 << MM_GENS_PROP_ACCESS_WRITE_POS),
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Buffer environment for buffer containing a stored message
typedef struct mm_gens_prop_buf_env
{
    /// Routing environment
    mm_route_env_t route_env;
    /// Type of request that has been received (User, Admin, Manufacturer)
    uint8_t type;
    /// Set (> 0) or get request
    uint8_t set;
    /// Set a status to acknowledge reception of message
    uint8_t ack;
    /// Type of property
    uint8_t prop_type;
    /// Property ID
    uint16_t prop_id;
    /// Element Index
    uint8_t elmt_idx;
    /// Property Index
    uint8_t prop_idx;
} mm_gens_prop_buf_env_t;

/// General environment for Generic Property models
typedef struct mm_gens_prop_env
{
    /// List of received messages waiting to be handled
    list_t list_msg_rx;
    /// Timer for detection of application timeout
    mesh_timer_t tmr_to;
    /// Delayed job structure
    mesh_djob_t djob;

    /// Total number of user properties
    uint16_t nb_prop_user;
    /// Number of stored received messages (not higher than MM_GENS_PROP_STORED_MSG_MAX)
    uint8_t nb_msg_rx;
    /// Local index for Generic User Property Server model
    m_lid_t mdl_lid_user;
    /// Local index for Generic Admin Property Server model
    m_lid_t mdl_lid_admin;
    /// Local index for Generic Manufacturer Property Server model
    m_lid_t mdl_lid_manuf;
    /// Number of message that can be stored in the list
    uint8_t queue_len;
} mm_gens_prop_env_t;

/// Environment common for Generic User/Admin/Manufacturer Property models
typedef struct mm_gens_prop_common_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
    /// Pointer to environment allocated for all Generic Property models
    mm_gens_prop_env_t *p_env_prop;
} mm_gens_prop_common_env_t;

/// Structure for Generic User/Admin/Manufacturer Property Server model environment
typedef struct mm_gens_prop_full_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
    /// Pointer to environment allocated for all Generic Property models - DO NOT MOVE
    mm_gens_prop_env_t *p_env_prop;

    /// Number of Generic Admin Property states
    uint8_t nb_prop;
    /// Generic Admin Property states
    mm_prop_t prop[];
} mm_gens_prop_full_env_t;

/// Structure for Generic Client Property Server model environment
typedef struct mm_gens_prop_cli_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;

    /// Number of Generic Client Property states
    uint8_t nb_prop;
    /// Generic Client Property states
    uint16_t prop[];
} mm_gens_prop_cli_env_t;

/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Look for Generic Property information in a list of Generic Properties.
 * Properties are sorted in ascending order, dichotomy is used.
 *
 * @param[in] prop_id       Property ID to look for
 * @param[in] p_env_prop    Pointer to environment for the model
 * @param[out] p_prop_idx   Pointer at which index of found property must be returned
 * @param[out] p_access     Pointer at which user access of found property must be returned
 *
 * @return True if indicated property has been found, else False
 ****************************************************************************************
 */
__STATIC bool mm_gens_prop_find(uint16_t prop_id, mm_gens_prop_full_env_t *p_env_prop,
                                uint8_t *p_prop_idx, uint8_t *p_access)
{
    // Position
    uint8_t pos = p_env_prop->nb_prop;

    if (pos == 1)
    {
        if (p_env_prop->prop[0].prop_id == prop_id)
        {
            pos = 0;
        }
    }
    else
    {
        // Start index
        uint16_t start = 0;
        // End index
        uint16_t end = p_env_prop->nb_prop - 1;

        while (true)
        {
            // Checked index
            uint16_t idx = start + ((end - start) >> 1);

            if (p_env_prop->prop[idx].prop_id == prop_id)
            {
                pos = idx;
                break;
            }

            if (start == end)
            {
                break;
            }

            if (p_env_prop->prop[idx].prop_id < prop_id)
            {
                start = idx;
            }
            else
            {
                end = idx;
            }

            if (end == (start + 1))
            {
                if (p_env_prop->prop[start].prop_id == prop_id)
                {
                    pos = start;
                }
                else if (p_env_prop->prop[end].prop_id == prop_id)
                {
                    pos = end;
                }
                break;
            }
        }
    }

    if (pos != p_env_prop->nb_prop)
    {
        // Return user access and position
        if (p_access)
        {
            *p_access = p_env_prop->prop[pos].user_access;
        }
        *p_prop_idx = pos;
    }

    // Return if property has been found
    return (pos != p_env_prop->nb_prop);
}

/**
 ****************************************************************************************
 * @brief Send a Generic User/Admin/Manufacturer Properties Status message
 *
 * @param[in] p_route_env       Pointer to the routing environment for the received get
 * request
 * @param[in] type              Status type
 * @param[in] p_env_prop        Pointer to the environment for the model
 ****************************************************************************************
 */
__STATIC void mm_gens_prop_send_status_list(mm_route_env_t *p_route_env, uint8_t type,
                                            mm_gens_prop_full_env_t *p_env_prop)
{
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_status;
    // Environment common for all Generic Property models
    mm_gens_prop_env_t *p_env_common = p_env_prop->p_env_prop;
    // Number of Property IDs
    uint16_t nb_prop;

    // Get number of Property IDs
    if (type == MM_PROP_TYPE_USER)
    {
        nb_prop = p_env_common->nb_prop_user;
    }
    else
    {
        nb_prop = p_env_prop->nb_prop;
    }

    p_buf_status = mm_route_buf_alloc_status(MM_GENS_PROP_ID_LENGTH * nb_prop, p_route_env);
    if (p_buf_status)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf_status);
        // Get pointer to environment
        mm_route_env_t *p_env = (mm_route_env_t *)&p_buf_status->env[0];
        // Counter
        uint16_t cnt;

        if (type == MM_PROP_TYPE_USER)
        {
            p_env->opcode = MM_MSG_GEN_UPROPS_STATUS;
        }
        else if (type == MM_PROP_TYPE_ADMIN)
        {
            p_env->opcode = MM_MSG_GEN_APROPS_STATUS;
        }
        else // (type == MM_PROP_TYPE_MANUF)
        {
            p_env->opcode = MM_MSG_GEN_MPROPS_STATUS;
        }
        SETB(p_env->info, MM_ROUTE_INFO_PUBLISH, 0);
        SETB(p_env->info, MM_ROUTE_INFO_RX, 0);

        // Fill the message
        for (cnt = 0; cnt < p_env_prop->nb_prop; cnt++)
        {
            write16p(p_data, p_env_prop->prop[cnt].prop_id);
            p_data += MM_GENS_PROP_ID_LENGTH;
        }

        if (type == MM_PROP_TYPE_USER)
        {
            if (p_env_common->mdl_lid_manuf != MESH_INVALID_LID)
            {
                // Get environment for Generic Manufacturer Property Server model
                mm_gens_prop_full_env_t *p_env_manuf
                            = (mm_gens_prop_full_env_t *)mm_state_get_env(p_env_common->mdl_lid_manuf);

                // Get Property ID of Manufacturer Properties that are User Properties
                for (cnt = 0; cnt < p_env_manuf->nb_prop; cnt++)
                {
                    if (p_env_manuf->prop[cnt].user_access != 0)
                    {
                        write16p(p_data, p_env_manuf->prop[cnt].prop_id);
                        p_data += MM_GENS_PROP_ID_LENGTH;
                    }
                }
            }

            if (p_env_common->mdl_lid_admin != MESH_INVALID_LID)
            {
                // Get environment for Generic Admin Property Server model
                mm_gens_prop_full_env_t *p_env_admin
                            = (mm_gens_prop_full_env_t *)mm_state_get_env(p_env_common->mdl_lid_admin);

                // Get Property ID of Admin Properties that are User Properties
                for (cnt = 0; cnt < p_env_admin->nb_prop; cnt++)
                {
                    if (p_env_admin->prop[cnt].user_access != 0)
                    {
                        write16p(p_data, p_env_admin->prop[cnt].prop_id);
                        p_data += MM_GENS_PROP_ID_LENGTH;
                    }
                }
            }
        }

        // Send the message
        mm_route_send(p_buf_status);
    }
}

/**
 ****************************************************************************************
 * @brief Send a Generic Client Properties Status message
 *
 * @param[in] p_route_env       Pointer to the routing environment for the received get
 * request
 * @param[in] p_env_prop        Pointer to the environment for the model
 ****************************************************************************************
 */
__STATIC void mm_gens_prop_send_status_list_cli(mm_route_env_t *p_route_env,
                                                 mm_gens_prop_cli_env_t *p_env_prop_cli)
{
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_status;
    // Status length
    uint16_t length_status = MM_GENS_PROP_ID_LENGTH * p_env_prop_cli->nb_prop;
    // Allocate a new buffer for the publication
    p_buf_status = mm_route_buf_alloc_status(length_status, p_route_env);

    if (p_buf_status)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf_status);
        // Get pointer to environment
        mm_route_env_t *p_env = (mm_route_env_t *)&p_buf_status->env[0];
        // Counter
        uint8_t cnt;

        p_env->opcode = MM_MSG_GEN_CPROPS_STATUS;
        SETB(p_env->info, MM_ROUTE_INFO_PUBLISH, 0);
        SETB(p_env->info, MM_ROUTE_INFO_RX, 0);

        // Fill the message
        for (cnt = 0; cnt < p_env_prop_cli->nb_prop; cnt++)
        {
            write16p(p_data, p_env_prop_cli->prop[cnt]);
            p_data += MM_GENS_PROP_ID_LENGTH;
        }

        // Send the message
        mm_route_send(p_buf_status);
    }
}

/**
 ****************************************************************************************
 * @brief Send a Generic User/Admin/Manufacturer Property Status message
 *
 * @param[in] p_route_env       Pointer to the routing environment for the received get
 * request
 * @param[in] type              Status type
 * @param[in] prop_id           Property ID
 * @param[in] found             True if property has been found and is known
 * @param[in] access            User access
 * @param[in] length            Length of property value
 * @param[in] p_val             Pointer to the property value
 ****************************************************************************************
 */
__STATIC void mm_gens_prop_send_status(mm_route_env_t *p_route_env, bool publish, uint8_t type,
                                       uint16_t prop_id, bool found, uint8_t access,
                                       uint8_t length, const uint8_t *p_val)
{
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_status;
    // Status message length
    uint16_t length_status = MM_GEN_PROP_STATUS_MIN_LEN;

    if (found)
    {
        length_status += (length + 1);
    }

    // Allocate a new buffer for the publication
    p_buf_status = mm_route_buf_alloc_status(length_status, p_route_env);

    if (p_buf_status)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf_status);
        // Get pointer to environment
        mm_route_env_t *p_env = (mm_route_env_t *)&p_buf_status->env[0];

        if (type == MM_PROP_TYPE_USER)
        {
            p_env->opcode = MM_MSG_GEN_UPROP_STATUS;
        }
        else if (type == MM_PROP_TYPE_ADMIN)
        {
            p_env->opcode = MM_MSG_GEN_APROP_STATUS;
        }
        else if (type == MM_PROP_TYPE_MANUF)
        {
            p_env->opcode = MM_MSG_GEN_MPROP_STATUS;
        }
        SETB(p_env->info, MM_ROUTE_INFO_PUBLISH, publish);

        // Fill the message
        write16p(p_data + MM_GEN_PROP_STATUS_ID_POS, prop_id);

        if (found)
        {
            *(p_data + MM_GEN_PROP_STATUS_ACCESS_POS) = access;

            if (p_val)
            {
                memcpy(p_data + MM_GEN_PROP_STATUS_VALUE_POS, p_val, length);
            }
        }

        // Send the message
        mm_route_send(p_buf_status);
    }
}

/*
 * MESSAGE HANDLING FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handle received Generic User Properties Get or Generic User Property Get or Generic
 * User Property Set or Generic User Property Set Unacknowledged
 *
 * @param[in] p_env_prop_user   Pointer to the environment allocated for the Generic User
 * Property Server model
 * @param[in] p_buf             Pointer to the buffer containing the received message
 * @param[in] p_route_env       Pointer to structure containing reception parameters provided
 * by the Mesh Profile block
 ****************************************************************************************
 */
__STATIC bool mm_gens_prop_handler_user(mm_gens_prop_full_env_t *p_env_prop_user,
                                        mesh_buf_t *p_buf, mm_route_env_t *p_route_env)
{
    // Free the message
    bool free_msg = true;

    if (p_route_env->opcode == MM_MSG_GEN_UPROPS_GET)
    {
        // Send a Generic User Properties Status message
        mm_gens_prop_send_status_list(p_route_env, MM_PROP_TYPE_USER, p_env_prop_user);
    }
    else // MM_MSG_GEN_UPROP_GET || MM_MSG_GEN_UPROP_SET || MM_MSG_GEN_UPROP_SET_UNACK
    {
        do
        {
            // Pointer to data
            uint8_t *p_data = MESH_BUF_DATA(p_buf);
            // Extract Property ID (same position in set and get messages)
            uint16_t prop_id = read16p(p_data + MM_GEN_PROP_SET_USER_ID_POS);
            // Property type, property index and user access rights
            uint8_t prop_type, prop_idx, access;
            // Indicate if indicated property has been found
            bool found = true;

            if (prop_id == 0x0000)
            {
                break;
            }

            // Look for indicated property
            do
            {
                mm_gens_prop_env_t *p_env_prop;

                // Start by parsing list of Generic User Properties
                if (mm_gens_prop_find(prop_id, p_env_prop_user, &prop_idx, &access))
                {
                    prop_type = MM_PROP_TYPE_USER;
                    break;
                }

                p_env_prop = p_env_prop_user->p_env_prop;

                // Not found, parse list of Generic Admin Properties if not empty
                if (p_env_prop->mdl_lid_admin != MESH_INVALID_LID)
                {
                    // Get environment for the Generic Admin Property Server model
                    mm_gens_prop_full_env_t *p_env_prop_admin
                            = (mm_gens_prop_full_env_t *)mm_state_get_env(p_env_prop->mdl_lid_admin);

                    if (mm_gens_prop_find(prop_id, p_env_prop_admin, &prop_idx, &access))
                    {
                        prop_type = MM_PROP_TYPE_ADMIN;
                        break;
                    }
                }

                // Not found, parse list of Generic Manufacturer Properties if not empty
                if (p_env_prop->mdl_lid_manuf != MESH_INVALID_LID)
                {
                    // Get environment for the Generic Manufacturer Property Server model
                    mm_gens_prop_full_env_t *p_env_prop_manuf
                            = (mm_gens_prop_full_env_t *)mm_state_get_env(p_env_prop->mdl_lid_manuf);

                    if (mm_gens_prop_find(prop_id, p_env_prop_manuf, &prop_idx, &access))
                    {
                        prop_type = MM_PROP_TYPE_MANUF;
                        break;
                    }
                }

                found = false;
            } while (0);

            if (found)
            {
                // Pointer to buffer environment
                mm_gens_prop_buf_env_t *p_buf_env = (mm_gens_prop_buf_env_t *)&p_buf->env[0];

                // Fill buffer environment
                p_buf_env->elmt_idx = p_env_prop_user->env.elmt_idx;
                p_buf_env->prop_id = prop_id;
                p_buf_env->prop_idx = prop_idx;
                p_buf_env->prop_type = prop_type;
                p_buf_env->type = MM_PROP_TYPE_USER;
                p_buf_env->set = (p_route_env->opcode != MM_MSG_GEN_UPROP_GET);
                p_buf_env->ack = (p_route_env->opcode != MM_MSG_GEN_UPROP_SET_UNACK);

                if (!p_buf_env->set)
                {
                    if (GETB(access, MM_GENS_PROP_ACCESS_READ))
                    {
                        // Inform the application about received get request
                        mm_srv_prop_get_send(p_buf_env->elmt_idx, prop_type, prop_id);
                        free_msg = false;
                        break;
                    }
                }
                else
                {
                    if (GETB(access, MM_GENS_PROP_ACCESS_WRITE))
                    {
                        // Get property value length
                        uint16_t length = p_buf->data_len - MM_GEN_PROP_SET_USER_VAL_POS;
                        // Get pointer to property value
                        uint8_t *p_val = p_data + MM_GEN_PROP_SET_USER_VAL_POS;

                        // Inform the application about received set request
                        mm_srv_prop_set_send(p_buf_env->elmt_idx, prop_type, prop_id, length, p_val);
                        free_msg = false;
                        break;
                    }
                }
            }

            // Error detected, send a Generic User Property Status message
            mm_gens_prop_send_status(p_route_env, false, MM_PROP_TYPE_USER, prop_id, false, 0, 0, NULL);
        } while (0);
    }

    return (free_msg);
}

/**
 ****************************************************************************************
 * @brief Handle received Generic Admin Properties Get or Generic Admin Property Get or Generic
 * Admin Property Set or Generic Admin Property Set Unacknowledged
 *
 * @param[in] p_env_prop_admin  Pointer to the environment allocated for the Generic Admin
 * Property Server model
 * @param[in] p_buf             Pointer to the buffer containing the received message
 * @param[in] p_route_env       Pointer to structure containing reception parameters provided
 * by the Mesh Profile block
 ****************************************************************************************
 */
__STATIC bool mm_gens_prop_handler_admin(mm_gens_prop_full_env_t *p_env_prop_admin,
                                         mesh_buf_t *p_buf, mm_route_env_t *p_route_env)
{
    // Free the message
    bool free_msg = true;

    if (p_route_env->opcode == MM_MSG_GEN_APROPS_GET)
    {
        // Send a Generic Admin Properties Status message
        mm_gens_prop_send_status_list(p_route_env, MM_PROP_TYPE_ADMIN, p_env_prop_admin);
    }
    else // MM_MSG_GEN_APROP_GET || MM_MSG_GEN_APROP_SET || MM_MSG_GEN_APROP_SET_UNACK
    {
        // Pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Extract Property ID (same position in set and get messages)
        uint16_t prop_id = read16p(p_data + MM_GEN_PROP_SET_ADMIN_ID_POS);
        // Property index
        uint8_t prop_idx;

        if (prop_id != 0x0000)
        {
            if (mm_gens_prop_find(prop_id, p_env_prop_admin, &prop_idx, NULL))
            {
                // Pointer to buffer environment
                mm_gens_prop_buf_env_t *p_buf_env = (mm_gens_prop_buf_env_t *)&p_buf->env[0];

                // Fill buffer environment
                p_buf_env->elmt_idx = p_env_prop_admin->env.elmt_idx;
                p_buf_env->prop_id = prop_id;
                p_buf_env->prop_idx = prop_idx;
                p_buf_env->prop_type = MM_PROP_TYPE_ADMIN;
                p_buf_env->type = MM_PROP_TYPE_ADMIN;
                p_buf_env->set = (p_route_env->opcode != MM_MSG_GEN_APROP_GET);
                p_buf_env->ack = (p_route_env->opcode != MM_MSG_GEN_APROP_SET_UNACK);

                if (!p_buf_env->set)
                {
                    // Inform the application about received get request
                    mm_srv_prop_get_send(p_buf_env->elmt_idx, MM_PROP_TYPE_ADMIN, prop_id);
                    free_msg = false;
                }
                else
                {
                    // Get User Access
                    uint8_t access = *(p_data + MM_GEN_PROP_SET_ADMIN_ACCESS_POS);

                    // Check that user access value is valid
                    if ((access & ~(MM_GENS_PROP_ACCESS_READ_BIT | MM_GENS_PROP_ACCESS_WRITE_BIT)) == 0)
                    {
                        // Get property value length
                        uint16_t length = p_buf->data_len - MM_GEN_PROP_SET_ADMIN_VAL_POS;
                        // Get pointer to property value
                        uint8_t *p_val = p_data + MM_GEN_PROP_SET_ADMIN_VAL_POS;

                        // Decrease number of User Properties if property is a User Property
                        if (p_env_prop_admin->prop[prop_idx].user_access != 0)
                        {
                            p_env_prop_admin->p_env_prop->nb_prop_user--;
                        }

                        // Update user access
                        p_env_prop_admin->prop[prop_idx].user_access = access;

                        // Increase number of User Properties if property is a User Property
                        if (access != 0)
                        {
                            p_env_prop_admin->p_env_prop->nb_prop_user++;
                        }

                        // Inform the application about received set request
                        mm_srv_prop_set_send(p_buf_env->elmt_idx, MM_PROP_TYPE_ADMIN,
                                                 prop_id, length, p_val);
                        free_msg = false;
                    }
                }
            }
            else
            {
                // Send a Generic Admin Property Status message
                mm_gens_prop_send_status(p_route_env, false, MM_PROP_TYPE_ADMIN, prop_id,
                                         false, 0, 0, NULL);
            }
        }
    }

    return (free_msg);
}

/**
 ****************************************************************************************
 * @brief Handle received Generic Manufacturer Properties Get or Generic Manufacturer Property
 * Get or Generic Manufacturer Property Set or Generic Manufacturer Property Set Unacknowledged
 *
 * @param[in] p_env_prop_manuf  Pointer to the environment allocated for the Generic
 * Manufacturer Property Server model
 * @param[in] p_buf             Pointer to the buffer containing the received message
 * @param[in] p_route_env       Pointer to structure containing reception parameters provided
 * by the Mesh Profile block
 ****************************************************************************************
 */
__STATIC bool mm_gens_prop_handler_manuf(mm_gens_prop_full_env_t *p_env_prop_manuf,
                                         mesh_buf_t *p_buf, mm_route_env_t *p_route_env)
{
    // Free the message
    bool free_msg = true;

    if (p_route_env->opcode == MM_MSG_GEN_MPROPS_GET)
    {
        // Send a Generic Manufacturer Properties Status message
        mm_gens_prop_send_status_list(p_route_env, MM_PROP_TYPE_MANUF, p_env_prop_manuf);
    }
    else // MM_MSG_GEN_MPROP_GET || MM_MSG_GEN_MPROP_SET || MM_MSG_GEN_MPROP_SET_UNACK
    {
        // Pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Extract Property ID (same position in set and get messages)
        uint16_t prop_id = read16p(p_data + MM_GEN_PROP_SET_MANUF_ID_POS);
        // Property index
        uint8_t prop_idx;

        if (prop_id != 0x0000)
        {
            if (mm_gens_prop_find(prop_id, p_env_prop_manuf, &prop_idx, NULL))
            {
                // Pointer to buffer environment
                mm_gens_prop_buf_env_t *p_buf_env = (mm_gens_prop_buf_env_t *)&p_buf->env[0];

                // Fill buffer environment
                p_buf_env->elmt_idx = p_env_prop_manuf->env.elmt_idx;
                p_buf_env->prop_id = prop_id;
                p_buf_env->prop_idx = prop_idx;
                p_buf_env->prop_type = MM_PROP_TYPE_MANUF;
                p_buf_env->type = MM_PROP_TYPE_MANUF;
                p_buf_env->set = (p_route_env->opcode != MM_MSG_GEN_MPROP_GET);
                p_buf_env->ack = (p_route_env->opcode != MM_MSG_GEN_MPROP_SET_UNACK);

                if (!p_buf_env->set)
                {
                    // Inform the application about received get request
                    mm_srv_prop_get_send(p_buf_env->elmt_idx, MM_PROP_TYPE_MANUF, prop_id);
                    free_msg = false;
                }
                else
                {
                    // Get User Access
                    uint8_t access = *(p_data + MM_GEN_PROP_SET_MANUF_ACCESS_POS);

                    // Check that user access value is valid
                    if ((access & ~MM_GENS_PROP_ACCESS_READ_BIT) == 0)
                    {
                        // Decrease number of User Properties if property is a User Property
                        if (p_env_prop_manuf->prop[prop_idx].user_access != 0)
                        {
                            p_env_prop_manuf->p_env_prop->nb_prop_user--;
                        }

                        // Update user access
                        p_env_prop_manuf->prop[prop_idx].user_access = access;

                        // Decrease number of User Properties if property is a User Property
                        if (access != 0)
                        {
                            p_env_prop_manuf->p_env_prop->nb_prop_user++;
                        }

                        // Inform the application about received get request
                        mm_srv_prop_get_send(p_buf_env->elmt_idx, MM_PROP_TYPE_MANUF, prop_id);
                        free_msg = false;
                    }
                }
            }
            else
            {
                // Send a Generic Manufacturer Property Status message
                mm_gens_prop_send_status(p_route_env, false, MM_PROP_TYPE_MANUF, prop_id,
                                         false, 0, 0, NULL);
            }
        }
    }

    return (free_msg);
}

/**
 ****************************************************************************************
 * @brief Extract first buffer from the list of stored received message. Release the buffer.
 * Delay handling of next buffer in the list.
 *
 * @param[in] p_env_prop        Pointer to general environment for the Generic Property models
 ****************************************************************************************
 */
__STATIC void mm_gens_prop_msg_free(mm_gens_prop_env_t *p_env_prop)
{
    // Extract first buffer from the list
    mesh_buf_t *p_buf = (mesh_buf_t *)list_pop_front(&p_env_prop->list_msg_rx);

    p_env_prop->nb_msg_rx--;

    // Release the buffer
    mesh_buf_release(p_buf);

    // Delay processing of next stored buffer if needed
    if (p_env_prop->nb_msg_rx)
    {
        mesh_djob_reg(&p_env_prop->djob);
    }
}

/**
 ****************************************************************************************
 * @brief Get first buffer from the list of stored received message. Process it
 *
 * @param[in] p_env_prop        Pointer to general environment for the Generic Property models
 ****************************************************************************************
 */
__STATIC void mm_gens_prop_msg_process_next(mm_gens_prop_env_t *p_env_prop)
{
    // Get first buffer in the list
    mesh_buf_t *p_buf = (mesh_buf_t *)list_pick(&p_env_prop->list_msg_rx);
    // Get environment provided by the Routing Manager
    mm_route_env_t *p_route_env = (mm_route_env_t *)&p_buf->env[0];
    // Indicate if message can be freed
    bool free_msg = true;

    // Call the appropriate handler
    switch (p_route_env->opcode)
    {
        case (MM_MSG_GEN_UPROPS_GET):
        case (MM_MSG_GEN_UPROP_GET):
        case (MM_MSG_GEN_UPROP_SET):
        case (MM_MSG_GEN_UPROP_SET_UNACK):
        {
            // Get environment allocated for the Generic User Property Server model
            mm_gens_prop_full_env_t *p_env_user
                    = (mm_gens_prop_full_env_t *)mm_state_get_env(p_env_prop->mdl_lid_user);

            free_msg = mm_gens_prop_handler_user(p_env_user, p_buf, p_route_env);
        } break;

        case (MM_MSG_GEN_APROPS_GET):
        case (MM_MSG_GEN_APROP_GET):
        case (MM_MSG_GEN_APROP_SET):
        case (MM_MSG_GEN_APROP_SET_UNACK):
        {
            if (p_env_prop->mdl_lid_admin != MESH_INVALID_LID)
            {
                // Get environment allocated for the Generic Admin Property Server model
                mm_gens_prop_full_env_t *p_env_admin
                        = (mm_gens_prop_full_env_t *)mm_state_get_env(p_env_prop->mdl_lid_admin);

                free_msg = mm_gens_prop_handler_admin(p_env_admin, p_buf, p_route_env);
            }
        } break;

        case (MM_MSG_GEN_MPROPS_GET):
        case (MM_MSG_GEN_MPROP_GET):
        case (MM_MSG_GEN_MPROP_SET):
        case (MM_MSG_GEN_MPROP_SET_UNACK):
        {
            if (p_env_prop->mdl_lid_manuf != MESH_INVALID_LID)
            {
                // Get environment allocated for the Generic Manufacturer Property Server model
                mm_gens_prop_full_env_t *p_env_manuf
                        = (mm_gens_prop_full_env_t *)mm_state_get_env(p_env_prop->mdl_lid_manuf);

                free_msg = mm_gens_prop_handler_manuf(p_env_manuf, p_buf, p_route_env);
            }
        } break;

        default:
        {
        } break;
    }

    if (free_msg)
    {
        // Free the message
        mm_gens_prop_msg_free(p_env_prop);
    }
    else
    {
        // Start timeout timer
        mesh_timer_set(&p_env_prop->tmr_to, MM_GENS_PROP_TMR_TO_DUR);
    }
}

/*
 * INTERNAL CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Check if a given opcode is handled by the Generic User Property Server model
 *
 * @param[in] p_env         Pointer to the environment allocated for the Generic User Property
 * Server model
 * @param[in] opcode        Opcode to check
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_gens_prop_user_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if ((opcode == MM_MSG_GEN_UPROPS_GET)
            || (opcode == MM_MSG_GEN_UPROP_GET)
            || (opcode == MM_MSG_GEN_UPROP_SET)
            || (opcode == MM_MSG_GEN_UPROP_SET_UNACK))
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
 * @brief Check if a given opcode is handled by the Generic Admin Property Server model
 *
 * @param[in] p_env         Pointer to the environment allocated for the Generic Admin Property
 * Server model
 * @param[in] opcode        Opcode to check
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_gens_prop_admin_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if ((opcode == MM_MSG_GEN_APROPS_GET)
            || (opcode == MM_MSG_GEN_APROP_GET)
            || (opcode == MM_MSG_GEN_APROP_SET)
            || (opcode == MM_MSG_GEN_APROP_SET_UNACK))
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
 * @brief Check if a given opcode is handled by the Generic Manufacturer Property Server model
 *
 * @param[in] p_env         Pointer to the environment allocated for the Generic Manufacturer
 * Property Server model
 * @param[in] opcode        Opcode to check
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_gens_prop_manuf_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if ((opcode == MM_MSG_GEN_MPROPS_GET)
            || (opcode == MM_MSG_GEN_MPROP_GET)
            || (opcode == MM_MSG_GEN_MPROP_SET)
            || (opcode == MM_MSG_GEN_MPROP_SET_UNACK))
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
 * @brief Inform Generic Client Property Server model about a received message
 *
 * @param[in] p_env         Pointer to the environment allocated for the Generic Client Property
 * Server model
 * @param[in] p_buf         Pointer to the buffer containing the received message
 * @param[in] p_route_env   Pointer to structure containing reception parameters provided
 * by the Mesh Profile block
 ****************************************************************************************
 */
__STATIC void mm_gens_prop_cli_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                                     mm_route_env_t *p_route_env)
{
    // Get environment allocated for Generic Client Property Server model
    mm_gens_prop_cli_env_t *p_env_prop_cli = (mm_gens_prop_cli_env_t *)p_env;

    // Send a Generic Client Properties Status message
    mm_gens_prop_send_status_list_cli(p_route_env, p_env_prop_cli);
}

/**
 ****************************************************************************************
 * @brief Check if a given opcode is handled by the Generic Client Property Server model
 *
 * @param[in] p_env         Pointer to the environment allocated for the Generic Client Property
 * Server model
 * @param[in] opcode        Opcode to check
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_gens_prop_cli_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status;

    if (opcode == MM_MSG_GEN_CPROPS_GET)
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
 * @brief Inform Generic User/Admin/Manufacturer/Client Property Server model about a received
 * message
 *
 * @param[in] p_env         Pointer to the environment allocated for the Generic Battery
 * Server model
 * @param[in] p_buf         Pointer to the buffer containing the received message
 * @param[in] p_route_env   Pointer to structure containing reception parameters provided
 * by the Mesh Profile block
 ****************************************************************************************
 */
__STATIC void mm_gens_prop_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                                 mm_route_env_t *p_route_env)
{
    // Map structure common for all kind of environments
    mm_gens_prop_common_env_t *p_env_common = (mm_gens_prop_common_env_t *)p_env;
    // Get environment allocated for the Generic Property models
    mm_gens_prop_env_t *p_env_prop = p_env_common->p_env_prop;

    // Check that list of received messages is not full
    if (p_env_prop->nb_msg_rx < p_env_prop->queue_len)
    {
        // Buffer that will contain a copy of the message
        mesh_buf_t *p_buf_cpy;
        // Allocate a buffer
        p_buf_cpy = mesh_buf_alloc(p_buf->head_len, p_buf->data_len, p_buf->tail_len);

        if (p_buf_cpy)
        {
            // Copy received message
            //uint8_t status = mesh_buf_copy(p_buf, p_buf_cpy, p_buf->data_len, false);
            mesh_buf_copy_data(p_buf_cpy, p_buf, p_buf->data_len);

            //if (status == MESH_ERR_NO_ERROR)
            {
                // Copy environment provided by the Routing Manager
                memcpy(&p_buf_cpy->env[0], p_route_env, sizeof(mm_route_env_t));

                // Store the buffer containing the copy of the received message
                list_push_back(&p_env_prop->list_msg_rx, &p_buf_cpy->hdr);
                p_env_prop->nb_msg_rx++;

                // If message is the first in the list, process it
                if (p_env_prop->nb_msg_rx == 1)
                {
                    mm_gens_prop_msg_process_next(p_env_prop);
                }
            }
        }
    }
    // else drop the message
}

/**
 ****************************************************************************************
 * @brief Callback function used for delaying of a task
 ****************************************************************************************
 */
__STATIC void mm_gens_prop_cb_djob(void *p_djob)
{
    // Get environment allocated for the Generic Property models
    mm_gens_prop_env_t *p_env_prop = MESH_DJOB2ENV(p_djob, mm_gens_prop_env_t, djob);

    // Handle next message
    if (p_env_prop->nb_msg_rx)
    {
        mm_gens_prop_msg_process_next(p_env_prop);
    }
}

/**
 ****************************************************************************************
 * @brief Callback function used for timeout timer when waiting for a confirmation from the
 * application.
 ****************************************************************************************
 */
__STATIC void mm_gens_prop_cb_tmr_to(void *p_tmr)
{
    mm_gens_prop_env_t *p_env_prop = MESH_TMR2ENV(p_tmr, mm_gens_prop_env_t, tmr_to);
    // Free first message in the queue
    mm_gens_prop_msg_free(p_env_prop);
}

/*
 * LOCAL FUNCTIONS (REGISTRATION)
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Register Generic User Property Server model
 *
 * @param[in] elmt_idx          Element index
 * @param[in] nb_prop_user      Number of Generic User Property states
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_gens_prop_register_user(uint8_t elmt_idx, uint8_t nb_prop_user, mm_prop_t *p_props,
                                             m_lid_t *p_mdl_lid, mm_gens_prop_env_t **pp_env_prop)
{
    // Returned status
    uint8_t status = MESH_ERR_COMMAND_DISALLOWED;
    // Environment length
    uint16_t env_length
            = sizeof(mm_gens_prop_full_env_t) + nb_prop_user * sizeof(mm_prop_t)
                                              + sizeof(mm_gens_prop_env_t);

    // Register Generic User Property Server model
    *p_mdl_lid = ms_register_model(MM_ID_GENS_UPROP, elmt_idx, MM_CFG_PUBLI_AUTH_BIT);

    if (*p_mdl_lid != MESH_INVALID_LID) //if (status == MESH_ERR_NO_ERROR)
    {
        // Inform the Model State Manager about registered model
        mm_gens_prop_full_env_t *p_env_prop_user = (mm_gens_prop_full_env_t *)mm_state_register(elmt_idx, MM_ID_GENS_UPROP, *p_mdl_lid, MM_ROLE_SRV_PUBLI,
                                      env_length);

        if (p_env_prop_user)
        {
            // Keep number of properties
            p_env_prop_user->nb_prop = nb_prop_user;

            // Keep property information
            memcpy(&p_env_prop_user->prop[0], p_props, nb_prop_user * sizeof(mm_prop_t));

            // Set internal callback functions
            p_env_prop_user->env.mdl_cb.cb_rx = mm_gens_prop_cb_rx;
            p_env_prop_user->env.mdl_cb.cb_opcode_check = mm_gens_prop_user_cb_opcode_check;
            //p_env_prop_user->env.mdl_cb.cb_publish_param = mm_gens_prop_cb_publish_param;

            status = MESH_ERR_NO_ERROR;
            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENS_UPROP, elmt_idx, *p_mdl_lid);

            // Return address of general environment
            p_env_prop_user->p_env_prop = (mm_gens_prop_env_t *)((uint32_t)p_env_prop_user
                                                            + sizeof(mm_gens_prop_full_env_t)
                                                            + (nb_prop_user * sizeof(mm_prop_t)));
            *pp_env_prop = p_env_prop_user->p_env_prop;
            p_env_prop_user->p_env_prop->nb_prop_user = nb_prop_user;
        }
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Register Generic Admin Property Server model
 *
 * @param[in] elmt_idx          Element index
 * @param[in] nb_prop_admin     Number of Generic Admin Property states
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_gens_prop_register_admin(uint8_t elmt_idx, uint8_t nb_prop_admin,
                                              mm_prop_t *p_props, mm_gens_prop_env_t *p_env_prop)
{
    // Returned status
    uint8_t status = MESH_ERR_COMMAND_DISALLOWED;
    // Environment length
    uint16_t env_length
            = sizeof(mm_gens_prop_full_env_t) + nb_prop_admin * sizeof(mm_prop_t);
    // Register Generic Admin Property Server model
    m_lid_t mdl_lid = ms_register_model(MM_ID_GENS_APROP, elmt_idx, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID) //if (status == MESH_ERR_NO_ERROR)
    {
        // Inform the Model State Manager about registered model
        mm_gens_prop_full_env_t *p_env_prop_admin = (mm_gens_prop_full_env_t *)mm_state_register(elmt_idx, MM_ID_GENS_APROP, mdl_lid,
                                      MM_ROLE_SRV_PUBLI, env_length);

        if (p_env_prop_admin)
        {
            // Counter
            uint8_t cnt;

            // Keep number of properties
            p_env_prop_admin->nb_prop = nb_prop_admin;

            // Keep property information
            memcpy(&p_env_prop_admin->prop[0], p_props, nb_prop_admin * sizeof(mm_prop_t));

            // Check if there are User Properties
            for (cnt = 0; cnt < nb_prop_admin; cnt++)
            {
                if (p_env_prop_admin->prop[cnt].user_access != 0)
                {
                    p_env_prop->nb_prop_user++;
                }
            }

            // Set internal callback functions
            p_env_prop_admin->env.mdl_cb.cb_rx = mm_gens_prop_cb_rx;
            p_env_prop_admin->env.mdl_cb.cb_opcode_check = mm_gens_prop_admin_cb_opcode_check;
            //p_env_prop_admin->env.mdl_cb.cb_publish_param = mm_gens_prop_cb_publish_param;

            status = MESH_ERR_NO_ERROR;
            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENS_APROP, elmt_idx, mdl_lid);

            p_env_prop_admin->p_env_prop = p_env_prop;
            p_env_prop->mdl_lid_admin = mdl_lid;
        }
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Register Generic Manufacturer Property Server model
 *
 * @param[in] elmt_idx          Element index
 * @param[in] nb_prop_manuf     Number of Generic Manufacturer Property states
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_gens_prop_register_manuf(uint8_t elmt_idx, uint8_t nb_prop_manuf,
                                              mm_prop_t *p_props, mm_gens_prop_env_t *p_env_prop)
{
    // Returned status
    uint8_t status = MESH_ERR_COMMAND_DISALLOWED;
    // Environment length
    uint16_t env_length
            = sizeof(mm_gens_prop_full_env_t) + nb_prop_manuf * sizeof(mm_prop_t);
    // Register Generic Manufacturer Property Server model
    m_lid_t mdl_lid = ms_register_model(MM_ID_GENS_MPROP, elmt_idx, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID) //if (status == MESH_ERR_NO_ERROR)
    {
        // Inform the Model State Manager about registered model
        mm_gens_prop_full_env_t *p_env_prop_manuf = (mm_gens_prop_full_env_t *)mm_state_register(elmt_idx, MM_ID_GENS_MPROP, mdl_lid,
                                      MM_ROLE_SRV_PUBLI, env_length);

        if (p_env_prop_manuf)
        {
            // Counter
            uint8_t cnt;

            // Keep number of properties
            p_env_prop_manuf->nb_prop = nb_prop_manuf;

            // Keep property information
            memcpy(&p_env_prop_manuf->prop[0], p_props, nb_prop_manuf * sizeof(mm_prop_t));

            // Check if there are User Properties
            for (cnt = 0; cnt < nb_prop_manuf; cnt++)
            {
                if (p_env_prop_manuf->prop[cnt].user_access != 0)
                {
                    p_env_prop->nb_prop_user++;
                }
            }

            // Set internal callback functions
            p_env_prop_manuf->env.mdl_cb.cb_rx = mm_gens_prop_cb_rx;
            p_env_prop_manuf->env.mdl_cb.cb_opcode_check = mm_gens_prop_manuf_cb_opcode_check;
            //p_env_prop_manuf->env.mdl_cb.cb_publish_param = mm_gens_prop_cb_publish_param;

            status = MESH_ERR_NO_ERROR;
            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENS_MPROP, elmt_idx, mdl_lid);

            p_env_prop_manuf->p_env_prop = p_env_prop;
            p_env_prop->mdl_lid_manuf = mdl_lid;
        }
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Register Generic Client Property Server model
 *
 * @param[in] elmt_idx          Element index
 * @param[in] nb_prop_cli       Number of Generic Client Property states
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_gens_prop_register_cli(uint8_t elmt_idx, uint8_t nb_prop_cli, mm_prop_t *p_props)
{
    // Returned status
    uint8_t status = MESH_ERR_COMMAND_DISALLOWED;
    // Environment length
    uint16_t env_length
            = sizeof(mm_gens_prop_cli_env_t) + nb_prop_cli * sizeof(mm_gens_prop_cli_env_t);
    // Register Generic Client Property Server model
    m_lid_t mdl_lid = ms_register_model(MM_ID_GENS_CPROP, elmt_idx, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID) //if (status == MESH_ERR_NO_ERROR)
    {
        // Inform the Model State Manager about registered model
        mm_gens_prop_cli_env_t *p_env_prop_cli = (mm_gens_prop_cli_env_t *)mm_state_register(elmt_idx, MM_ID_GENS_CPROP, mdl_lid,
                                      MM_ROLE_SRV_PUBLI, env_length);

        if (p_env_prop_cli)
        {
            // Counter
            uint8_t cnt;

            // Keep number of properties
            p_env_prop_cli->nb_prop = nb_prop_cli;

            // Keep property information
            for (cnt = 0; cnt < nb_prop_cli; cnt++)
            {
                p_env_prop_cli->prop[cnt] = p_props[cnt].prop_id;
            }

            // Set internal callback functions
            p_env_prop_cli->env.mdl_cb.cb_rx = mm_gens_prop_cli_cb_rx;
            p_env_prop_cli->env.mdl_cb.cb_opcode_check = mm_gens_prop_cli_cb_opcode_check;
            //p_env_prop_cli->env.mdl_cb.cb_publish_param = mm_gens_prop_cb_publish_param;

            status = MESH_ERR_NO_ERROR;
            // Inform application about registered model
            mm_register_ind_send(MM_ID_GENS_MPROP, elmt_idx, mdl_lid);
        }
    }

    return (status);
}

/*
 * GLOBAL FUNCTIONS
 ****************************************************************************************
 */

uint8_t mm_gens_prop_register(uint8_t elmt_idx, uint8_t req_queue_len,
                               uint8_t nb_prop_user, uint8_t nb_prop_admin,
                               uint8_t nb_prop_manuf, uint8_t nb_prop_cli, const mm_prop_t *p_props)
{
    uint8_t status = MESH_ERR_NO_ERROR;

    do
    {
        mm_prop_t *p_prop_cursor = (mm_prop_t *)p_props;

        if (nb_prop_user)
        {
            // Pointer to general environment
            mm_gens_prop_env_t *p_env_prop;
            // Model local index for the Generic User Property Server model
            m_lid_t mdl_lid_user;

            // Register Generic User Property Server model
            status = mm_gens_prop_register_user(elmt_idx, nb_prop_user, p_prop_cursor,
                                                &mdl_lid_user, &p_env_prop);

            if (status != MESH_ERR_NO_ERROR)
            {
                break;
            }

            // Fill general environment
            p_env_prop->queue_len = req_queue_len;
            p_env_prop->mdl_lid_user = mdl_lid_user;

            p_prop_cursor += nb_prop_user;

            if (nb_prop_admin)
            {
                // Register Generic Admin Property Server model
                status = mm_gens_prop_register_admin(elmt_idx, nb_prop_admin,
                                                     p_prop_cursor, p_env_prop);

                if (status != MESH_ERR_NO_ERROR)
                {
                    break;
                }

                p_prop_cursor += nb_prop_admin;
            }
            else
            {
                p_env_prop->mdl_lid_admin = MESH_INVALID_LID;
            }

            if (nb_prop_manuf)
            {
                // Register Generic Manufacturer Property Server model
                status = mm_gens_prop_register_manuf(elmt_idx, nb_prop_manuf,
                                                     p_prop_cursor, p_env_prop);

                if (status != MESH_ERR_NO_ERROR)
                {
                    break;
                }

                p_prop_cursor += nb_prop_manuf;
            }
            else
            {
                p_env_prop->mdl_lid_manuf = MESH_INVALID_LID;
            }

            // Prepare timeout timer structure
            p_env_prop->tmr_to.cb = mm_gens_prop_cb_tmr_to;
            //p_env_prop->tmr_to.p_env = p_env_prop;

            // Prepare delayed job structure
            p_env_prop->djob.cb = mm_gens_prop_cb_djob;
            //p_env_prop->djob.p_env = p_env_prop;
        }

        if (nb_prop_cli)
        {
            // Register Generic Client Property Server model
            status = mm_gens_prop_register_cli(elmt_idx, nb_prop_cli, p_prop_cursor);
        }
    } while (0);

    return (status);
}

void mm_gens_prop_cfm(uint8_t elmt_idx, uint8_t status, uint8_t prop_type, uint16_t prop_id,
                      uint16_t length, const uint8_t *p_val)
{
    // Get model local index for Generic User Property Server model on the element
    m_lid_t mdl_lid = mm_state_get_lid(elmt_idx, MM_ID_GENS_UPROP);

    if (mdl_lid != MESH_INVALID_LID)
    {
        // Get environment allocated for Generic User Property Server model
        mm_gens_prop_full_env_t *p_env_user
                        = (mm_gens_prop_full_env_t *)mm_state_get_env(mdl_lid);
        // Get environment allocated for the Generic Property models
        mm_gens_prop_env_t *p_env_prop = p_env_user->p_env_prop;

        if (p_env_prop->nb_msg_rx)
        {
            // Pick first message in the list
            mesh_buf_t *p_buf = (mesh_buf_t *)list_pick(&p_env_prop->list_msg_rx);
            // Get buffer environment
            mm_gens_prop_buf_env_t *p_buf_env = (mm_gens_prop_buf_env_t *)&p_buf->env[0];

            if ((elmt_idx == p_buf_env->elmt_idx)
                    && (prop_type == p_buf_env->prop_type)
                    && (prop_id == p_buf_env->prop_id))
            {
                // Model environment
                mm_gens_prop_full_env_t *p_env;
                // Get property information
                mm_prop_t *p_prop;

                // Stop the timeout timer
                mesh_timer_clear(&p_env_prop->tmr_to);

                if (status == MESH_ERR_NO_ERROR)
                {
                    if (prop_type == MM_PROP_TYPE_USER)
                    {
                        p_env = p_env_user;
                    }
                    else if (prop_type == MM_PROP_TYPE_ADMIN)
                    {
                        p_env = (mm_gens_prop_full_env_t *)mm_state_get_env(p_env_prop->mdl_lid_admin);
                    }
                    else // (prop_type == MM_PROP_TYPE_MANUF):
                    {
                        p_env = (mm_gens_prop_full_env_t *)mm_state_get_env(p_env_prop->mdl_lid_manuf);
                    }

                    p_prop = &p_env->prop[p_buf_env->prop_idx];

                    if (p_buf_env->ack)
                    {
                        // Send the status message
                        mm_gens_prop_send_status(&p_buf_env->route_env, false, p_buf_env->type,
                                                 p_buf_env->prop_id, true, p_prop->user_access, length, p_val);
                    }

                    // Send a publication
                    if (p_buf_env->set
                            && GETB(p_env->env.info, MM_INFO_PUBLI))
                    {
                        // Send the status message
                        mm_gens_prop_send_status(&p_buf_env->route_env, true, p_buf_env->type,
                                                 p_buf_env->prop_id, true, p_prop->user_access, length,
                                                 p_val);
                    }
                }

                // Free the message
                mm_gens_prop_msg_free(p_env_prop);
            }
        }
        // else confirmation is not expected
    }
}

/// @} end of group
