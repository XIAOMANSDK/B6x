/**
 ****************************************************************************************
 * @file mm_lights_ctl.c
 *
 * @brief Mesh Model Light CTL Server Module
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup MM_LIGHTS_CTL
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "mm_itf.h"
#include "mm_gens.h"
#include "mm_lights.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Validity of information provided to the Replay Manager
#define MM_LIGHTS_CTL_REPLAY_MS               (6000)
/// Minimum value for Light CTL Temperature state value
#define MM_LIGHTS_CTL_TEMP_MIN                (0x0320)
/// Maximum value for Light CTL Temperature state value
#define MM_LIGHTS_CTL_TEMP_MAX                (0x4E20)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Bit field content for status_info value present in environment for Light CTL Server model
///              7           6          5             4           3          2         1       0
/// +------------+-----------+----------+-------------+-----------+----------+---------+-------+
/// | Trans Temp | Trans DUV | Trans LN | Temp Status | Wait Temp | Wait DUV | Wait LN | Relay |
/// +------------+-----------+----------+-------------+-----------+----------+---------+-------+
enum mm_lights_ctl_sinfo_bf
{
    /// Relaying of sent status message is authorized
    MM_LIGHTS_CTL_SINFO_RELAY_POS = 0,
    MM_LIGHTS_CTL_SINFO_RELAY_BIT = (1 << MM_LIGHTS_CTL_SINFO_RELAY_POS),

    /// Wait for transition event for Light CTL Lightness state
    MM_LIGHTS_CTL_SINFO_WAIT_LN_POS = 1,
    MM_LIGHTS_CTL_SINFO_WAIT_LN_BIT = (1 << MM_LIGHTS_CTL_SINFO_WAIT_LN_POS),

    /// Wait for transition event for Light CTL Delta UV state
    MM_LIGHTS_CTL_SINFO_WAIT_DUV_POS = 2,
    MM_LIGHTS_CTL_SINFO_WAIT_DUV_BIT = (1 << MM_LIGHTS_CTL_SINFO_WAIT_DUV_POS),

    /// Wait for transition event for Light CTL Temperature state
    MM_LIGHTS_CTL_SINFO_WAIT_TEMP_POS = 3,
    MM_LIGHTS_CTL_SINFO_WAIT_TEMP_BIT = (1 << MM_LIGHTS_CTL_SINFO_WAIT_TEMP_POS),

    /// Wait for transition event mask
    MM_LIGHTS_CTL_SINFO_WAIT_LSB = 1,
    MM_LIGHTS_CTL_SINFO_WAIT_MASK = 0x0E,

    /// Indicate if Light CTL Status (0) or Light CTL Temperature Status message must be sent
    MM_LIGHTS_CTL_SINFO_TEMP_STATUS_POS = 4,
    MM_LIGHTS_CTL_SINFO_TEMP_STATUS_BIT = (1 << MM_LIGHTS_CTL_SINFO_TEMP_STATUS_POS),

    /// Transition for Light CTL Lightness state in progress
    MM_LIGHTS_CTL_SINFO_TRANS_LN_POS = 5,
    MM_LIGHTS_CTL_SINFO_TRANS_LN_BIT = (1 << MM_LIGHTS_CTL_SINFO_TRANS_LN_POS),

    /// Transition for Light CTL Delta UV state in progress
    MM_LIGHTS_CTL_SINFO_TRANS_DUV_POS = 6,
    MM_LIGHTS_CTL_SINFO_TRANS_DUV_BIT = (1 << MM_LIGHTS_CTL_SINFO_TRANS_DUV_POS),

    /// Transition for Light CTL Temperature in progress
    MM_LIGHTS_CTL_SINFO_TRANS_TEMP_POS = 7,
    MM_LIGHTS_CTL_SINFO_TRANS_TEMP_BIT = (1 << MM_LIGHTS_CTL_SINFO_TRANS_TEMP_POS),

    /// Transition in progress mask
    MM_LIGHTS_CTL_SINFO_TRANS_LSB = 5,
    MM_LIGHTS_CTL_SINFO_TRANS_MASK = 0xE0,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef struct mm_lights_ctl_env mm_lights_ctl_env_t;
typedef struct mm_lights_ctls_env mm_lights_ctls_env_t;
typedef struct mm_lights_ctlt_env mm_lights_ctlt_env_t;

/// Structure for Light CTL Server model environment
struct mm_lights_ctl_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
    /// Timer for sending of publications
    mesh_timer_t tmr_publi;
    /// Publication period in milliseconds
    uint32_t publi_period_ms;

    /// Environment for replay protection mechanism
    mm_replay_env_t replay_env;

    /// Pointer to environment of associated Light CTL Temperature model
    mm_lights_ctlt_env_t *p_env_ctlt;

    /// Delta value in case of move transition for Light CTL Lightness state value
    int16_t move_delta;
    /// Light CTL Lightness state value
    uint16_t ln;
    /// Target Light CTL Lightness state value
    uint16_t ln_tgt;
    /// Light CTL Delta UV state value
    int16_t delta_uv;
    /// Target Light CTL Delta UV state value
    int16_t delta_uv_tgt;
    /// Light CTL Delta Default state value
    int16_t delta_uv_dflt;

    /// Source address of set message that has triggered last or current transition
    uint8_t status_dst_addr;
    /// Application key local index to be used for transmission of Light CTL Status
    /// or Light CTL Linear Status message
    m_lid_t status_app_key_lid;
    /// Status information bitfield (@see enum mm_lights_ctl_sinfo_bf)
    uint8_t status_info;
};

/// Structure for Light CTL Setup Server model environment
struct mm_lights_ctls_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;

    /// Pointer to environment of associated Light CTL Server model
    mm_lights_ctl_env_t *p_env_ctl;
};

/// Structure for Light CTL Temperature Server model environment
struct mm_lights_ctlt_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t env;
    /// Timer for sending of publications
    mesh_timer_t tmr_publi;
    /// Publication period in milliseconds
    uint32_t publi_period_ms;

    /// Pointer to environment of associated Light CTL Temperature model
    mm_lights_ctl_env_t *p_env_ctl;

    /// Delta value in case of move transition for Light CTL Temperature state value
    int16_t move_delta;
    /// Light CTL Temperature
    uint16_t temp;
    /// Target Light CTL Temperature
    uint16_t temp_tgt;
    /// Target Generic Level value - Keep it in order to not accumulate error due to division
    int16_t lvl_tgt;
    /// Light CTL Temperature Default
    uint16_t temp_dflt;
    /// Light CTL Temperature Range Min state value
    uint16_t temp_min;
    /// Light CTL Temperature Range Max state value
    uint16_t temp_max;
};

/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Prepare and send a Light CTL Status message.
 *
 * @param[in] p_env_ctl         Pointer to Light CTL Server model environment
 * @param[in] p_route_env       Pointer to structure containing reception information provided
 * by Mesh Profile for received request message
 * @param[in] publish           Indicate if message is sent as a publication (true) or as
 * a response to a received request (false)
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_send_status(mm_lights_ctl_env_t *p_env_ctl,
                                        mm_route_env_t *p_route_env, bool publish)
{
    // Get environment for Light CTL Temperature Server model
    mm_lights_ctlt_env_t *p_env_ctlt = p_env_ctl->p_env_ctlt;
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_status;
    // Remaining time for Light CTL Lightness state and for Light CTL Temperature
    // state transition
    uint8_t ln_rem_time, temp_rem_time;
    // Transition type for Light CTL Lightness and for Light CTL Temperature states
    uint8_t ln_trans_type, temp_trans_type;
    // Data length
    uint8_t data_length;

    // Check if a transition has been started
    mm_bind_get_trans_info(p_env_ctl->env.grp_lid, &ln_trans_type, &ln_rem_time);
    mm_bind_get_trans_info(p_env_ctlt->env.grp_lid, &temp_trans_type, &temp_rem_time);

    // Deduce deduce data length
    data_length = ((ln_trans_type != MM_TRANS_TYPE_NONE) || (temp_trans_type != MM_TRANS_TYPE_NONE))
                                    ? MM_LIGHT_CTL_STATUS_LEN : MM_LIGHT_CTL_STATUS_MIN_LEN;

    p_buf_status = mm_route_buf_alloc(data_length);
    if (p_buf_status)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf_status);
        // Get pointer to environment
        mm_route_env_t *p_buf_env = (mm_route_env_t *)&p_buf_status->env;

        // Prepare environment
        if (p_route_env)
        {
            memcpy(p_buf_env, p_route_env, sizeof(mm_route_env_t));
        }
        else if (!publish)
        {
            p_buf_env->app_key_lid = p_env_ctl->status_app_key_lid;
            p_buf_env->u_addr.dst = p_env_ctl->status_dst_addr;
            SETB(p_buf_env->info, MM_ROUTE_INFO_RELAY,
                 GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_RELAY));
        }

        SETB(p_buf_env->info, MM_ROUTE_INFO_RX, 0);
        SETB(p_buf_env->info, MM_ROUTE_INFO_PUBLISH, publish);
        p_buf_env->mdl_lid = p_env_ctl->env.mdl_lid;
        p_buf_env->opcode = MM_MSG_LIGHT_CTL_STATUS;

        // Fill the message
        write16p(p_data + MM_LIGHT_CTL_STATUS_LIGHTNESS_POS, p_env_ctl->ln);
        write16p(p_data + MM_LIGHT_CTL_STATUS_TEMP_POS, p_env_ctlt->temp);

        if (data_length == MM_LIGHT_CTL_STATUS_LEN)
        {
            // Sent Target Light CTL Temperature and Light CTL Lightness state values
            uint16_t temp_tgt, ln_tgt;
            // Sent Remaining Time value
            uint8_t rem_time = 0;

            if (ln_trans_type != MM_TRANS_TYPE_NONE)
            {
                if (ln_trans_type == MM_TRANS_TYPE_MOVE)
                {
                    // Light Lightness Range
                    uint16_t ln_min = mm_lights_ln_get(p_env_ctl->env.elmt_idx,
                                                       MM_STATE_LIGHT_LN_RANGE_MIN);
                    uint16_t ln_max = mm_lights_ln_get(p_env_ctl->env.elmt_idx,
                                                       MM_STATE_LIGHT_LN_RANGE_MAX);

                    ln_tgt = (p_env_ctl->move_delta > 0) ? ln_max : ln_min;
                    rem_time = MM_TRANS_TIME_UNKNOWN;
                }
                else
                {
                    ln_tgt = p_env_ctl->ln_tgt;
                    rem_time = ln_rem_time;
                }
            }
            else
            {
                ln_tgt = p_env_ctl->ln;
            }

            if (temp_trans_type != MM_TRANS_TYPE_NONE)
            {
                if (temp_trans_type == MM_TRANS_TYPE_MOVE)
                {
                    temp_tgt = (p_env_ctlt->move_delta > 0) ? p_env_ctlt->temp_max
                                                            : p_env_ctlt->temp_min;
                    rem_time = MM_TRANS_TIME_UNKNOWN;
                }
                else
                {
                    temp_tgt = p_env_ctlt->temp_tgt;

                    if (rem_time != MM_TRANS_TIME_UNKNOWN)
                    {
                        if (temp_rem_time > ln_rem_time)
                        {
                            rem_time = temp_rem_time;
                        }
                    }
                }
            }
            else
            {
                temp_tgt = p_env_ctlt->temp;
            }

            write16p(p_data + MM_LIGHT_CTL_STATUS_TGT_LIGHTNESS_POS, ln_tgt);
            write16p(p_data + MM_LIGHT_CTL_STATUS_TGT_TEMP_POS, temp_tgt);
            *(p_data + MM_LIGHT_CTL_STATUS_REM_TIME_POS) = rem_time;
        }

        // Send the message
        mm_route_send(p_buf_status);
    }
}

/**
 ****************************************************************************************
 * @brief Prepare and send a Light CTL Temperature Status message
 *
 * @param[in] p_env_ctlt         Pointer to Light CTL Temperature Server model environment
 * @param[in] p_route_env        Pointer to structure containing reception information provided
 * by Mesh Profile for received request message
 * @param[in] publish            Indicate if message is sent as a publication (true) or as
 * a response to a received request (false)
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_send_status_temp(mm_lights_ctlt_env_t *p_env_ctlt,
                                             mm_route_env_t *p_route_env, bool publish)
{
    // Get environment for Light CTL Server model
    mm_lights_ctl_env_t *p_env_ctl = p_env_ctlt->p_env_ctl;
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_status;
    // Remaining time for Light CTL Lightness state and for Light CTL Temperature
    // state transition
    uint8_t ln_rem_time, temp_rem_time;;
    // Transition type for Light CTL Lightness and for Light CTL Temperature states
    uint8_t ln_trans_type, temp_trans_type;
    // Data length
    uint8_t data_length;

    // Check if a transition has been started
    mm_bind_get_trans_info(p_env_ctl->env.grp_lid, &ln_trans_type, &ln_rem_time);
    mm_bind_get_trans_info(p_env_ctlt->env.grp_lid, &temp_trans_type, &temp_rem_time);

    // Deduce deduce data length
    data_length = ((ln_trans_type != MM_TRANS_TYPE_NONE) || (temp_trans_type != MM_TRANS_TYPE_NONE))
                                ? MM_LIGHT_CTL_TEMP_STATUS_LEN : MM_LIGHT_CTL_TEMP_STATUS_MIN_LEN;

    p_buf_status = mm_route_buf_alloc(data_length);
    if (p_buf_status)
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf_status);
        // Get pointer to environment
        mm_route_env_t *p_buf_env = (mm_route_env_t *)&p_buf_status->env;

        // Prepare environment
        if (p_route_env)
        {
            memcpy(p_buf_env, p_route_env, sizeof(mm_route_env_t));
        }
        else if (!publish)
        {
            p_buf_env->app_key_lid = p_env_ctl->status_app_key_lid;
            p_buf_env->u_addr.dst = p_env_ctl->status_dst_addr;
            SETB(p_buf_env->info, MM_ROUTE_INFO_RELAY,
                 GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_RELAY));
        }

        SETB(p_buf_env->info, MM_ROUTE_INFO_RX, 0);
        SETB(p_buf_env->info, MM_ROUTE_INFO_PUBLISH, publish);
        p_buf_env->mdl_lid = p_env_ctlt->env.mdl_lid;
        p_buf_env->opcode = MM_MSG_LIGHT_CTL_TEMP_STATUS;

        // Fill the message
        write16p(p_data + MM_LIGHT_CTL_TEMP_STATUS_TEMP_POS, p_env_ctlt->temp);
        write16p(p_data + MM_LIGHT_CTL_TEMP_STATUS_DELTA_UV_POS, p_env_ctl->delta_uv);

        if (data_length == MM_LIGHT_CTL_TEMP_STATUS_LEN)
        {
            // Sent Target Light CTL Temperature and Light CTL Delta UV state values
            uint16_t temp_tgt, delta_uv_tgt;
            // Sent Remaining Time value
            uint8_t rem_time = 0;

            if (ln_trans_type != MM_TRANS_TYPE_NONE)
            {
                delta_uv_tgt = p_env_ctl->delta_uv_tgt;
                rem_time = ln_rem_time;
            }
            else
            {
                delta_uv_tgt = p_env_ctl->delta_uv;
            }

            if (temp_trans_type != MM_TRANS_TYPE_NONE)
            {
                if (temp_trans_type == MM_TRANS_TYPE_MOVE)
                {
                    temp_tgt = (p_env_ctlt->move_delta > 0) ? p_env_ctlt->temp_max
                                                            : p_env_ctlt->temp_min;
                    rem_time = MM_TRANS_TIME_UNKNOWN;
                }
                else
                {
                    temp_tgt = p_env_ctlt->temp_tgt;

                    if (temp_rem_time > ln_rem_time)
                    {
                        rem_time = temp_rem_time;
                    }
                }
            }
            else
            {
                temp_tgt = p_env_ctlt->temp;
            }

            write16p(p_data + MM_LIGHT_CTL_TEMP_STATUS_TGT_TEMP_POS, temp_tgt);
            write16p(p_data + MM_LIGHT_CTL_TEMP_STATUS_TGT_DELTA_UV_POS, delta_uv_tgt);
            *(p_data + MM_LIGHT_CTL_TEMP_STATUS_REM_TIME_POS) = rem_time;
        }

        // Send the message
        mm_route_send(p_buf_status);
    }
}

/**
 ****************************************************************************************
 * @brief Prepare and send a Light CTL Default Status message
 *
 * @param[in] p_env_ctl          Pointer to Light CTL Server model environment
 * @param[in] p_route_env       Pointer to structure containing reception information provided
 * by Mesh Profile for received request message
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_send_status_dflt(mm_lights_ctl_env_t *p_env_ctl,
                                             mm_route_env_t *p_route_env)
{
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_status = mm_route_buf_alloc_status(MM_LIGHT_CTL_DFLT_STATUS_LEN, p_route_env);

    if (p_buf_status)
    {
        // Get pointer to environment for Light Lightness CTL Temperature Server model
        mm_lights_ctlt_env_t *p_env_ctlt = p_env_ctl->p_env_ctlt;
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf_status);
        // Get pointer to environment
        mm_route_env_t *p_buf_env = (mm_route_env_t *)&p_buf_status->env;

        // Prepare environment
        p_buf_env->mdl_lid = p_env_ctl->env.mdl_lid;
        p_buf_env->opcode = MM_MSG_LIGHT_CTL_DFLT_STATUS;

        // Fill the message
        write16p(p_data + MM_LIGHT_CTL_DFLT_STATUS_LIGHTNESS_POS,
                    mm_lights_ln_get(p_env_ctl->env.elmt_idx, MM_STATE_LIGHT_LN_DFLT));
        write16p(p_data + MM_LIGHT_CTL_DFLT_STATUS_TEMP_POS, p_env_ctlt->temp_dflt);
        write16p(p_data + MM_LIGHT_CTL_DFLT_STATUS_DELTA_UV_POS, p_env_ctl->delta_uv_dflt);

        // Send the message
        mm_route_send(p_buf_status);
    }
}

/**
 ****************************************************************************************
 * @brief Prepare and send a Light CTL Temperature Range Status message
 *
 * @param[in] p_env_ctlt        Pointer to Light CTL Temperature Server model environment
 * @param[in] p_route_env       Pointer to structure containing reception information provided
 * by Mesh Profile for received request message
 * @param[in] status            Status sent in the Light CTL Temperature Status message
 * (@see enum mm_status)
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_send_status_temp_range(mm_lights_ctl_env_t *p_env_ctl,
                                                   mm_route_env_t *p_route_env,
                                                   uint8_t status)
{
    // Pointer to the buffer that will contain the message
    mesh_buf_t *p_buf_status = mm_route_buf_alloc_status(MM_LIGHT_CTL_TEMP_RANGE_STATUS_LEN, p_route_env);

    if (p_buf_status)
    {
        // Pointer to environment for Light CTL Temperature Server model
        mm_lights_ctlt_env_t *p_env_ctlt = p_env_ctl->p_env_ctlt;
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf_status);
        // Get pointer to environment
        mm_route_env_t *p_buf_env = (mm_route_env_t *)&p_buf_status->env;

        // Prepare environment
        p_buf_env->mdl_lid = p_env_ctl->env.mdl_lid;
        p_buf_env->opcode = MM_MSG_LIGHT_CTL_TEMP_RANGE_STATUS;

        // Fill the message
        *(p_data + MM_LIGHT_CTL_TEMP_RANGE_STATUS_CODE_POS) = status;
        write16p(p_data + MM_LIGHT_CTL_TEMP_RANGE_STATUS_MIN_POS, p_env_ctlt->temp_min);
        write16p(p_data + MM_LIGHT_CTL_TEMP_RANGE_STATUS_MAX_POS, p_env_ctlt->temp_max);

        // Send the message
        mm_route_send(p_buf_status);
    }
}

/**
 ****************************************************************************************
 * @brief Publish Light CTL state value if sending of publications is enabled
 *
 * @param[in] p_env_ctl         Pointer to Light CTL Server model environment
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_publish(mm_lights_ctl_env_t *p_env_ctl)
{
    // Check if sending of publication is enabled
    if (GETB(p_env_ctl->env.info, MM_INFO_PUBLI))
    {
        mm_lights_ctl_send_status(p_env_ctl, NULL, true);
    }
}

/**
 ****************************************************************************************
 * @brief Publish Light CTL Temperature state value if sending of publications is enabled
 *
 * @param[in] p_env_ctlt         Pointer to Light CTL Temperature Server model environment
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_publish_temp(mm_lights_ctlt_env_t *p_env_ctlt)
{
    // Check if sending of publication is enabled
    if (GETB(p_env_ctlt->env.info, MM_INFO_PUBLI))
    {
        mm_lights_ctl_send_status_temp(p_env_ctlt, NULL, true);
    }
}

/**
 ****************************************************************************************
 * @brief Check if a Light CTL Status or a Light CTL Temperature Status message must be sent
 * and send it if it is the case
 *
 * @param[in] p_env_ctl        Pointer to Light CTL Server model environment
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_check_status(mm_lights_ctl_env_t *p_env_ctl, uint8_t event)
{
    if (!GETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT))
    {
        // Get environment for Light CTL Temperature Server model
        mm_lights_ctlt_env_t *p_env_ctlt = p_env_ctl->p_env_ctlt;

        if (p_env_ctl->status_dst_addr != MESH_UNASSIGNED_ADDR)
        {
            // Send a response to the node that has required the transition
            if (!GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TEMP_STATUS))
            {
                mm_lights_ctl_send_status(p_env_ctl, NULL, false);
            }
            else
            {
                mm_lights_ctl_send_status_temp(p_env_ctl->p_env_ctlt, NULL, false);
            }

            p_env_ctl->status_dst_addr = MESH_UNASSIGNED_ADDR;
        }

        if (GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_LN)
                || GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_TEMP))
        {
            mm_lights_ctl_publish(p_env_ctl);
        }

        if (GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_DUV)
                || GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_TEMP))
        {
            // Send a publication
            mm_lights_ctl_publish_temp(p_env_ctlt);
        }

        if (event == MM_GRP_EVENT_TRANS_STARTED)
        {
            SETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT,
                 GETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS));
        }
        else
        {
            SETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT, 0);
            SETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS, 0);
        }
    }
}

/**
 ****************************************************************************************
 * @brief Convert a Generic Level value into a Light CTL Temperature value
 *
 * @param[in] lvl           Generic level value
 * @param[in] temp_min      Light CTL Temperature Min value
 * @param[in] temp_max      Light CTL Temperature Max value
 *
 * @return Light CTL Temperature value
 ****************************************************************************************
 */
__STATIC uint16_t mm_lights_ctl_lvl_to_temp(int16_t lvl, uint16_t temp_min, uint16_t temp_max)
{
    // Temperature value
    uint32_t temp = (int32_t)lvl + 32768;

    temp *= (temp_max - temp_min);
    temp /= 65535;
    temp += temp_min;

    return ((uint16_t)temp);
}

/**
 ****************************************************************************************
 * @brief Convert a Light CTL Temperature value into a Generic Level value
 *
 * @param[in] temp          Light CTL Temperature value
 * @param[in] temp_min      Light CTL Temperature Min value
 * @param[in] temp_max      Light CTL Temperature Max value
 *
 * @return Generic Level value
 ****************************************************************************************
 */
__STATIC int16_t mm_lights_ctl_temp_to_lvl(uint16_t temp, uint16_t temp_min, uint16_t temp_max)
{
    // Level value
    int32_t lvl = ((int32_t)temp - temp_min);

    lvl *= 65535;
    lvl /= (temp_max - temp_min);
    lvl -= 32768;

    return ((int16_t)lvl);
}

/**
 ****************************************************************************************
 * @brief Convert a Light CTL Temperature value into a Generic Level value
 *
 * @param[in] temp          Light CTL Temperature value
 * @param[in] temp_min      Light CTL Temperature Min value
 * @param[in] temp_max      Light CTL Temperature Max value
 *
 * @return Generic Level value
 ****************************************************************************************
 */
__STATIC uint16_t mm_lights_ctl_temp_add_delta(uint16_t temp, int32_t delta,
                                               uint16_t temp_min, uint16_t temp_max)
{
    delta *= (temp_max - temp_min);
    delta /= 65535;
    delta += temp;

    return ((uint16_t)delta);
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handler for Light CTL Set/Set Unacknowledged message
 *
 * @param[in] p_env         Pointer to environment of model for which message has been received
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] p_route_env   Pointer to routing information for the received buffer
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_handler_set(mm_lights_ctl_env_t *p_env_ctl, mesh_buf_t *p_buf,
                                        mm_route_env_t *p_route_env)
{
    do
    {
        // Get environment for Light CTL Temperature Server model
        mm_lights_ctlt_env_t *p_env_ctlt = p_env_ctl->p_env_ctlt;
        // Check if a status message must be sent
        bool send_status = (p_route_env->opcode == MM_MSG_LIGHT_CTL_SET);
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // TID value
        uint8_t tid = *(p_data + MM_LIGHT_CTL_SET_TID_POS);
        // Light CTL Lightness, Light CTL Temperature and Light CTL Delta UV value
        uint16_t ln, temp;
        int16_t delta_uv;
        // Transition time
        uint8_t trans_time;
        // Delay
        uint8_t delay;

        // Check if request can be processed
        if ((p_env_ctl->status_dst_addr != MESH_UNASSIGNED_ADDR)
               || (mm_replay_is_retx(&p_env_ctl->replay_env, p_route_env->u_addr.src, tid)))
       {
            // Send a Light CTL Status message
            if (send_status)
            {
                mm_lights_ctl_send_status(p_env_ctl, p_route_env, false);
            }
            break;
       }

        // Extract and check optional parameters if present
        if (p_buf->data_len == MM_LIGHT_CTL_SET_LEN)
        {
            trans_time = (uint16_t)(*(p_data + MM_LIGHT_CTL_SET_TRANS_TIME_POS));

            // Check received value
            if (GETF(trans_time, MM_TRANS_TIME_STEP_NB) > MM_TRANS_TIME_STEPS_MAX)
            {
                // Drop the message
                break;
            }

            delay = *(p_data + MM_LIGHT_CTL_SET_DELAY_POS);
        }
        else
        {
            trans_time = MM_TRANS_TIME_UNKNOWN;
            delay = 0;
        }

        ln = read16p(p_data + MM_LIGHT_CTL_SET_LIGHTNESS_POS);
        temp = read16p(p_data + MM_LIGHT_CTL_SET_TEMP_POS);
        delta_uv = read16p(p_data + MM_LIGHT_CTL_SET_DELTA_UV_POS);

        // Ensure that Light CTL Temperature state value is between Light CTL Temperature
        // Range Min and Max values
        if (temp > p_env_ctlt->temp_max)
        {
            temp = p_env_ctlt->temp_max;
        }
        else if (temp < p_env_ctlt->temp_min)
        {
            temp = p_env_ctlt->temp_min;
        }

        // Check if at least one of the states is modified
        if ((ln == p_env_ctl->ln)
                && (temp == p_env_ctlt->temp)
                && (delta_uv == p_env_ctl->delta_uv))
        {
            // Send a Light CTL Status message
            if (send_status)
            {
                mm_lights_ctl_send_status(p_env_ctl, p_route_env, false);
            }
            break;
        };

        if (send_status)
        {
            // Keep information for transmission of status
            p_env_ctl->status_dst_addr = p_route_env->u_addr.src;
            p_env_ctl->status_app_key_lid = p_route_env->app_key_lid;
            SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TEMP_STATUS, 0);
            SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_RELAY,
                 GETB(p_route_env->info, MM_ROUTE_INFO_RELAY));
        }

        SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_LN, (ln != p_env_ctl->ln));
        SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_DUV, (delta_uv != p_env_ctl->delta_uv));
        SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_TEMP, (temp != p_env_ctlt->temp));
        SETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT,
             GETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS));

        if (GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_LN)
                || GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_DUV))
        {
            // Update target state
            p_env_ctl->ln_tgt = ln;
            p_env_ctl->delta_uv_tgt = delta_uv;

            // Inform the Binding Manager about new transition
            mm_bind_trans_new(p_env_ctl->env.grp_lid, MM_TRANS_TYPE_CLASSIC, trans_time, delay);
        }

        if (GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_TEMP))
        {
            // Update target state
            p_env_ctlt->temp_tgt = temp;
            p_env_ctlt->lvl_tgt = mm_lights_ctl_temp_to_lvl(p_env_ctlt->temp_tgt, p_env_ctlt->temp_min,
                                                            p_env_ctlt->temp_max);

            // Inform the Binding Manager about new transition
            mm_bind_trans_new(p_env_ctlt->env.grp_lid, MM_TRANS_TYPE_CLASSIC, trans_time, delay);
        }
    } while (0);
}

/**
 ****************************************************************************************
 * @brief Handler for Light CTL Temperature Set/Set Unacknowledged message
 *
 * @param[in] p_env         Pointer to environment of model for which message has been received
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] p_route_env   Pointer to routing information for the received buffer
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_handler_set_temp(mm_lights_ctlt_env_t *p_env_ctlt, mesh_buf_t *p_buf,
                                             mm_route_env_t *p_route_env)
{
    do
    {
        // Get pointer to environment for Light Lightness CTL Server model
        mm_lights_ctl_env_t *p_env_ctl = p_env_ctlt->p_env_ctl;
        // Check if a status message must be sent
        bool send_status = (p_route_env->opcode == MM_MSG_LIGHT_CTL_TEMP_SET);
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Received ight CTL Temperature and Light CTL Delta UV state values
        uint16_t temp;
        int16_t delta_uv;
        // Transition time
        uint8_t trans_time = MM_TRANS_TIME_UNKNOWN;
        // Delay
        uint8_t delay = 0;
        // TID value
        uint8_t tid = *(p_data + MM_LIGHT_CTL_TEMP_SET_TID_POS);;

        // Check if request can be processed
        if ((p_env_ctl->status_dst_addr != MESH_UNASSIGNED_ADDR)
               || (mm_replay_is_retx(&p_env_ctl->replay_env, p_route_env->u_addr.src, tid)))
       {
            // Send a Light CTL Temperature Status message
            if (send_status)
            {
                mm_lights_ctl_send_status_temp(p_env_ctlt, p_route_env, false);
            }
            break;
       }

        // Extract and check optional parameters if present
        if (p_buf->data_len == MM_LIGHT_CTL_TEMP_SET_LEN)
        {
            trans_time = (uint16_t)(*(p_data + MM_LIGHT_CTL_TEMP_SET_TRANS_TIME_POS));

            // Check received value
            if (GETF(trans_time, MM_TRANS_TIME_STEP_NB) > MM_TRANS_TIME_STEPS_MAX)
            {
                // Drop the message
                break;
            }

            delay = *(p_data + MM_LIGHT_CTL_TEMP_SET_DELAY_POS);
        }

        temp = read16p(p_data + MM_LIGHT_CTL_TEMP_SET_TEMP_POS);
        delta_uv = read16p(p_data + MM_LIGHT_CTL_TEMP_SET_DELTA_UV_POS);

        // Ensure that Light CTL Temperature state value is between Light CTL Temperature Range
        // Min and Max values
        if ((temp > p_env_ctlt->temp_max)
                || ((temp < p_env_ctlt->temp_min)))
        {
            break;
        }

        // Check if at least one of the states is modified
        if ((temp == p_env_ctlt->temp)
                && (delta_uv == p_env_ctl->delta_uv))
        {
            // Send a Light CTL Temperature Status message
            if (send_status)
            {
                mm_lights_ctl_send_status_temp(p_env_ctlt, p_route_env, false);
            }
            break;
        };

        if (send_status)
        {
            // Keep information for transmission of status
            p_env_ctl->status_dst_addr = p_route_env->u_addr.src;
            p_env_ctl->status_app_key_lid = p_route_env->app_key_lid;
            SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TEMP_STATUS, 1);
            SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_RELAY,
                 GETB(p_route_env->info, MM_ROUTE_INFO_RELAY));
        }

        SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_LN, 0);
        SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_DUV, (delta_uv != p_env_ctl->delta_uv));
        SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_TEMP, (temp != p_env_ctlt->temp));
        SETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT,
             GETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS));

        // Check if Light CTL Delta UV value is modified
        if (GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_DUV))
        {
            // Update target state
            p_env_ctl->delta_uv_tgt = delta_uv;
            p_env_ctl->ln_tgt = p_env_ctl->ln;

            // Inform the Binding Manager about new transition
            mm_bind_trans_new(p_env_ctl->env.grp_lid, MM_TRANS_TYPE_CLASSIC, trans_time, delay);
        }

        // Check if Light CTL Temperature value is modified
        if (GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_TEMP))
        {
            // Update target state
            p_env_ctlt->temp_tgt = temp;
            p_env_ctlt->lvl_tgt = mm_lights_ctl_temp_to_lvl(p_env_ctlt->temp_tgt, p_env_ctlt->temp_min,
                                                            p_env_ctlt->temp_max);

            // Inform the Binding Manager about new transition
            mm_bind_trans_new(p_env_ctlt->env.grp_lid, MM_TRANS_TYPE_CLASSIC, trans_time, delay);
        }
    } while (0);
}

/**
 ****************************************************************************************
 * @brief Handler for Light CTL Default Set/Set Unacknowledged message
 *
 * @param[in] p_env         Pointer to environment of model for which message has been received
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] p_route_env   Pointer to routing information for the received buffer
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_handler_set_dflt(mm_lights_ctl_env_t *p_env_ctl,
                                             mesh_buf_t *p_buf, mm_route_env_t *p_route_env)
{
    do
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Get environment for Light Lightness CTL Temperature Server model
        mm_lights_ctlt_env_t *p_env_ctlt = p_env_ctl->p_env_ctlt;
        // Extract received state values
        uint16_t ln_dflt = read16p(p_data + MM_LIGHT_CTL_DFLT_SET_LIGHTNESS_POS);
        uint16_t temp_dflt = read16p(p_data + MM_LIGHT_CTL_DFLT_SET_TEMP_POS);
        uint16_t delta_uv_dflt = read16p(p_data + MM_LIGHT_CTL_DFLT_SET_DELTA_UV_POS);

        if ((temp_dflt > p_env_ctlt->temp_max)
                || (temp_dflt < p_env_ctlt->temp_min))
        {
            break;
        }

        // Inform Light Lightness Server model about received Light Lightness Default state value
        mm_lights_ln_set_dflt(p_env_ctl->env.elmt_idx, ln_dflt);

        if (temp_dflt != p_env_ctlt->temp_dflt)
        {
            // Keep received value
            p_env_ctlt->temp_dflt = temp_dflt;

            // Inform application about received value
            mm_srv_state_upd_ind_send(MM_STATE_LIGHT_CTL_TEMP_DFLT,
                                          p_env_ctlt->env.elmt_idx, temp_dflt, 0);
        }

        if (delta_uv_dflt != p_env_ctl->delta_uv_dflt)
        {
            // Keep received value
            p_env_ctl->delta_uv_dflt = delta_uv_dflt;

            // Inform application about received value
            mm_srv_state_upd_ind_send(MM_STATE_LIGHT_CTL_DELTA_UV_DFLT,
                                          p_env_ctl->env.elmt_idx, delta_uv_dflt, 0);
        }

        // If needed, send a Light CTL Default Status message to the requester
        if (p_route_env->opcode == MM_MSG_LIGHT_CTL_DFLT_SET)
        {
            mm_lights_ctl_send_status_dflt(p_env_ctl, p_route_env);
        }
    } while (0);
}

/**
 ****************************************************************************************
 * @brief Handler for Light CTL Temperature Range Set/Set Unacknowledged message
 *
 * @param[in] p_env         Pointer to environment of model for which message has been received
 * @param[in] p_buf         Pointer to buffer containing the received message
 * @param[in] p_route_env   Pointer to routing information for the received buffer
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_handler_set_temp_range(mm_lights_ctl_env_t *p_env_ctl,
                                                   mesh_buf_t *p_buf,
                                                   mm_route_env_t *p_route_env)
{
    do
    {
        // Get pointer to data
        uint8_t *p_data = MESH_BUF_DATA(p_buf);
        // Get environment for Light Lightness CTL Temperature Server model
        mm_lights_ctlt_env_t *p_env_ctlt = p_env_ctl->p_env_ctlt;
        // Status
        uint8_t status = MM_STATUS_SUCCESS;
        // Extract Light CTL Temperature Range state value
        uint16_t temp_min = read16p(p_data + MM_LIGHT_CTL_TEMP_RANGE_SET_MIN_POS);
        // Extract Light CTL Range state value
        uint16_t temp_max = read16p(p_data + MM_LIGHT_CTL_TEMP_RANGE_SET_MAX_POS);

        // Check provided values
        if (temp_min < MM_LIGHTS_CTL_TEMP_MIN)
        {
            status = MM_STATUS_ERROR_RANGE_MIN;
        }
        else if (temp_max > MM_LIGHTS_CTL_TEMP_MAX)
        {
            status = MM_STATUS_ERROR_RANGE_MAX;
        }
        else if (temp_min > temp_max)
        {
            // Drop the message
            break;
        }

        if ((status == MM_STATUS_SUCCESS)
                && ((p_env_ctlt->temp_min != temp_min)
                        || (p_env_ctlt->temp_max != temp_max)))
        {
            p_env_ctlt->temp_min = temp_min;
            p_env_ctlt->temp_max = temp_max;

            // Inform application about received value
            mm_srv_state_upd_ind_send(MM_STATE_LIGHT_CTL_TEMP_RANGE, p_env_ctlt->env.elmt_idx,
                                          (uint32_t)temp_min | ((uint32_t)temp_max << 16), 0);
        }

        // If needed, send a Light CTL Temperature Range Status message to the requester
        if (p_route_env->opcode == MM_MSG_LIGHT_CTL_TEMP_RANGE_SET)
        {
            mm_lights_ctl_send_status_temp_range(p_env_ctl, p_route_env, status);
        }
    } while (0);
}

/*
 * INTERNAL CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Callback function called when timer monitoring publication duration for
 * Light CTL Server model or for Light CTL Temperature expires
 *
 * @param[in] p_env     Pointer to model environment for Light CTL Server or Light CTL
 * Temperature model
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_cb_tmr_publi(void *p_tmr)
{
    mm_lights_ctl_env_t *p_env = MESH_TMR2ENV(p_tmr, mm_lights_ctl_env_t, tmr_publi);
    // Get allocated environment
    mm_mdl_publi_env_t *p_env_publi = (mm_mdl_publi_env_t *)p_env;

    if (p_env_publi->publi_period_ms)
    {
        if (p_env_publi->env.model_id == MM_ID_LIGHTS_CTL)
        {
            // Publish a Light CTL Status message
            mm_lights_ctl_publish((mm_lights_ctl_env_t *)p_env);
        }
        else // (p_env_publi->env.model_id == MM_ID_LIGHTS_CTLT)
        {
            // Publish a Light CTL Temperature Status message
            mm_lights_ctl_publish_temp((mm_lights_ctlt_env_t *)p_env);
        }

        // Restart the timer
        mesh_timer_set(&p_env_publi->tmr_publi, p_env_publi->publi_period_ms);
    }
}

/**
 ****************************************************************************************
 * @brief Inform Light CTL Server model about a received message
 *
 * @param[in] p_env         Pointer to the environment allocated for the Light CTL
 * Server model
 * @param[in] p_buf         Pointer to the buffer containing the received message
 * @param[in] p_route_env   Pointer to structure containing reception parameters provided
 * by the Mesh Profile block
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_cb_rx(mm_mdl_env_t *p_env, mesh_buf_t *p_buf,
                                  mm_route_env_t *p_route_env)
{
    do
    {
        // Environment for Light CTL Server model
        mm_lights_ctl_env_t *p_env_ctl;

        if (p_env->model_id == MM_ID_LIGHTS_CTL)
        {
            p_env_ctl = (mm_lights_ctl_env_t *)p_env;

            switch (p_route_env->opcode)
            {
                case (MM_MSG_LIGHT_CTL_GET):
                {
                    // Send a Light CTL Status message
                    mm_lights_ctl_send_status(p_env_ctl, p_route_env, false);
                } break;

                case (MM_MSG_LIGHT_CTL_SET):
                case (MM_MSG_LIGHT_CTL_SET_UNACK):
                {
                    // Handle the message
                    mm_lights_ctl_handler_set(p_env_ctl, p_buf, p_route_env);
                } break;

                case (MM_MSG_LIGHT_CTL_TEMP_RANGE_GET):
                {
                    // Send a Light CTL Temperature Range Status message
                    mm_lights_ctl_send_status_temp_range(p_env_ctl, p_route_env, MM_STATUS_SUCCESS);
                } break;

                case (MM_MSG_LIGHT_CTL_DFLT_GET):
                {
                    // Send a Light CTL Default Status message
                    mm_lights_ctl_send_status_dflt(p_env_ctl, p_route_env);
                } break;

                default:
                {
                } break;
            }
        }
        else if (p_env->model_id == MM_ID_LIGHTS_CTLS)
        {
            // Environment for Light CTL Setup Server model
            mm_lights_ctls_env_t *p_env_ctls = (mm_lights_ctls_env_t *)p_env;

            p_env_ctl = p_env_ctls->p_env_ctl;

            switch (p_route_env->opcode)
            {
                case (MM_MSG_LIGHT_CTL_DFLT_SET):
                case (MM_MSG_LIGHT_CTL_DFLT_SET_UNACK):
                {
                    // Handle the message
                    mm_lights_ctl_handler_set_dflt(p_env_ctl, p_buf, p_route_env);
                } break;

                case (MM_MSG_LIGHT_CTL_TEMP_RANGE_SET):
                case (MM_MSG_LIGHT_CTL_TEMP_RANGE_SET_UNACK):
                {
                    // Handle the message
                    mm_lights_ctl_handler_set_temp_range(p_env_ctl, p_buf, p_route_env);
                } break;

                default:
                {
                } break;
            }
        }
        else // (p_env->model_id == MM_ID_LIGHTS_CTLT)
        {
            // Environment for Light CTL Temperature model
            mm_lights_ctlt_env_t *p_env_ctlt = (mm_lights_ctlt_env_t *)p_env;

            switch (p_route_env->opcode)
            {
                case (MM_MSG_LIGHT_CTL_TEMP_GET):
                {
                    // Send a Light CTL Temperature Status message
                    mm_lights_ctl_send_status_temp(p_env_ctlt, p_route_env, false);
                } break;

                case (MM_MSG_LIGHT_CTL_TEMP_SET):
                case (MM_MSG_LIGHT_CTL_TEMP_SET_UNACK):
                {
                    // Handle the message
                    mm_lights_ctl_handler_set_temp(p_env_ctlt, p_buf, p_route_env);
                } break;

                default:
                {
                } break;
            }
        }
    } while (0);
}

/**
 ****************************************************************************************
 * @brief Check if a given opcode is handled by the Light CTL Server model
 *
 * @param[in] p_env         Pointer to the environment allocated for the Light CTL
 * Server model
 * @param[in] opcode        Opcode to check
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_lights_ctl_cb_opcode_check(mm_mdl_env_t *p_env, uint32_t opcode)
{
    uint8_t status = MESH_ERR_MDL_INVALID_OPCODE;

    if (p_env->model_id == MM_ID_LIGHTS_CTL)
    {
        if ((opcode == MM_MSG_LIGHT_CTL_GET)
                || (opcode == MM_MSG_LIGHT_CTL_SET)
                || (opcode == MM_MSG_LIGHT_CTL_SET_UNACK)
                || (opcode == MM_MSG_LIGHT_CTL_TEMP_RANGE_GET)
                || (opcode == MM_MSG_LIGHT_CTL_DFLT_GET))
        {
            status = MESH_ERR_NO_ERROR;
        }
    }
    else if (p_env->model_id == MM_ID_LIGHTS_CTLS)
    {
        if ((opcode == MM_MSG_LIGHT_CTL_DFLT_SET)
                || (opcode == MM_MSG_LIGHT_CTL_DFLT_SET_UNACK)
                || (opcode == MM_MSG_LIGHT_CTL_TEMP_RANGE_SET)
                || (opcode == MM_MSG_LIGHT_CTL_TEMP_RANGE_SET_UNACK))
        {
            status = MESH_ERR_NO_ERROR;
        }
    }
    else if (p_env->model_id == MM_ID_LIGHTS_CTLT)
    {
        if ((opcode == MM_MSG_LIGHT_CTL_TEMP_GET)
                || (opcode == MM_MSG_LIGHT_CTL_TEMP_SET)
                || (opcode == MM_MSG_LIGHT_CTL_TEMP_SET_UNACK))
        {
            status = MESH_ERR_NO_ERROR;
        }
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief
 *
 * @param[in] p_env         Pointer the the environment allocated for the Generic OnOff
 * Server model
 * @param[in] state_id      State identifier
 * @param[in] state         Light CTL Actual or Light CTL Default or Light
 * Lightness Range state value
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_lights_ctl_cb_set(mm_mdl_env_t *p_env, uint16_t state_id, uint32_t state)
{
    // Returned status
    uint8_t status = MESH_ERR_NO_ERROR;
    // Get environment for the Light CTL Server model
    mm_lights_ctl_env_t *p_env_ctl = (mm_lights_ctl_env_t *)p_env;
    // Get environment for the Light CTL Temperature Server model
    mm_lights_ctlt_env_t *p_env_ctlt = p_env_ctl->p_env_ctlt;

    switch (state_id)
    {
        case (MM_STATE_LIGHT_CTL_LN):
        {
            // Keep the provided state value
            p_env_ctl->ln = state;

            // Set the Light Lightness state value
            mm_bind_set_state(p_env_ctl->env.grp_lid, MM_STATE_TYPE_CURRENT,
                                 MM_ID_LIGHTS_LN, state);
        } break;

        case (MM_STATE_LIGHT_CTL_DELTA_UV):
        {
            // Keep the provided state value
            p_env_ctl->delta_uv = state;
        } break;

        case (MM_STATE_LIGHT_CTL_DELTA_UV_DFLT):
        {
            // Keep the provided state value
            p_env_ctl->delta_uv_dflt = state;
        } break;

        case (MM_STATE_LIGHT_CTL_TEMP):
        {
            // Keep the provided state value
            p_env_ctlt->temp = state;
        } break;

        case (MM_STATE_LIGHT_CTL_TEMP_DFLT):
        {
            // Keep the provided state value
            p_env_ctlt->temp_dflt = state;
        } break;

        case (MM_STATE_LIGHT_CTL_TEMP_RANGE):
        {
            // Keep the provided state value
            p_env_ctlt->temp_min = state;
            p_env_ctlt->temp_max = state;
        } break;

        default:
        {
            status = MESH_ERR_INVALID_PARAM;
        } break;
    }

    return (status);
}

/*
 * CALLBACK FUNCTIONS FOR BINDING MANAGER
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Callback function provided to the Binding Manager so that Light CTL Server
 * model can be informed about group event
 *
 * @param[in] mdl_lid       Model Local Index
 * @param[in] event         Event
 * @param[in] info          Information (depends on the event type)
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_cb_grp_event(m_lid_t mdl_lid, uint8_t event, uint8_t info)
{
    // Get environment for Light CTL Server model
    mm_lights_ctl_env_t *p_env_ctl = (mm_lights_ctl_env_t *)mm_state_get_env(mdl_lid);

    switch (event)
    {
        case (MM_GRP_EVENT_TRANS_DELAY_EXPIRED):
        {
            if (GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_LN))
            {
                // Set the targeted Light Lightness state value
                mm_bind_set_state(p_env_ctl->env.grp_lid, MM_STATE_TYPE_TARGET, MM_ID_LIGHTS_LN,
                                     p_env_ctl->ln_tgt);
            }

            // Start the transition
            mm_bind_trans_start(p_env_ctl->env.grp_lid);
        } break;

        case (MM_GRP_EVENT_TRANS_IMMEDIATE):
        {
            p_env_ctl->ln = p_env_ctl->ln_tgt;
            p_env_ctl->delta_uv = p_env_ctl->delta_uv_tgt;
        } // no break;

        case (MM_GRP_EVENT_TRANS_STARTED):
        {
            uint8_t trans_time = info;

            // Inform application about state update
            if (GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT_LN))
            {
                // Expected event has been received
                SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT_LN, 0);

                // Inform application
                mm_srv_state_upd_ind_send(MM_STATE_LIGHT_CTL_LN, p_env_ctl->env.elmt_idx,
                                              p_env_ctl->ln_tgt,
                                              (trans_time) ? mm_get_trans_time_ms(trans_time) : 0);
            }

            if (GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT_DUV))
            {
                // Expected event has been received
                SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT_DUV, 0);

                // Inform application
                mm_srv_state_upd_ind_send(MM_STATE_LIGHT_CTL_DELTA_UV, p_env_ctl->env.elmt_idx,
                                              p_env_ctl->delta_uv_tgt,
                                              (trans_time) ? mm_get_trans_time_ms(trans_time) : 0);
            }

            // Check if status message must be sent
            mm_lights_ctl_check_status(p_env_ctl, event);
        } break;

        case (MM_GRP_EVENT_TRANS_END):
        {
            p_env_ctl->ln = p_env_ctl->ln_tgt;
            p_env_ctl->delta_uv = p_env_ctl->delta_uv_tgt;

            // Inform application about state update
            if (GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT_LN))
            {
                // Expected event has been received
                SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT_LN, 0);

                mm_srv_state_upd_ind_send(MM_STATE_LIGHT_CTL_LN,
                                              p_env_ctl->env.elmt_idx, p_env_ctl->ln, 0);
            }
            if (GETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT_DUV))
            {
                // Expected event has been received
                SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT_DUV, 0);

                mm_srv_state_upd_ind_send(MM_STATE_LIGHT_CTL_DELTA_UV,
                                              p_env_ctl->env.elmt_idx, p_env_ctl->delta_uv, 0);
            }

            // Check if status message must be sent
            mm_lights_ctl_check_status(p_env_ctl, event);
        } break;

        case (MM_GRP_EVENT_GROUP_FULL):
        {
            // Set the targeted Light Lightness state value
            mm_bind_set_state(p_env_ctl->env.grp_lid, MM_STATE_TYPE_CURRENT, MM_ID_LIGHTS_LN,
                                 p_env_ctl->ln);
        } break;

        //case (MM_GRP_EVENT_TRANS_ABORTED):
        default:
        {
        } break;
    }
}

/**
 ****************************************************************************************
 * @brief Callback function provided to the Binding Manager so that Light CTL Temperature Server
 * model can be informed about group event
 *
 * @param[in] mdl_lid       Model Local Index
 * @param[in] event         Event
 * @param[in] info          Information (depends on the event type)
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_cb_grp_event_temp(m_lid_t mdl_lid, uint8_t event, uint8_t info)
{
    // Get environment for Light CTL Temperature Server model
    mm_lights_ctlt_env_t *p_env_ctlt = (mm_lights_ctlt_env_t *)mm_state_get_env(mdl_lid);
    // Get environment for Light CTL Server model
    mm_lights_ctl_env_t *p_env_ctl = p_env_ctlt->p_env_ctl;

    switch (event)
    {
        case (MM_GRP_EVENT_TRANS_DELAY_EXPIRED):
        {
            // Set the targeted Generic Level state value
            mm_bind_set_state(p_env_ctlt->env.grp_lid, MM_STATE_TYPE_TARGET, MM_ID_GENS_LVL,
                                 p_env_ctlt->lvl_tgt);

            // Start the transition
            mm_bind_trans_start(p_env_ctlt->env.grp_lid);
        } break;

        case (MM_GRP_EVENT_TRANS_IMMEDIATE):
        {
            p_env_ctlt->temp = p_env_ctlt->temp_tgt;
        } // no break;

        case (MM_GRP_EVENT_TRANS_STARTED):
        {
            uint8_t trans_time = info;

            // Mark transition as finished
            SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT_TEMP, 0);

            // Inform application about state update
            mm_srv_state_upd_ind_send(MM_STATE_LIGHT_CTL_TEMP, p_env_ctlt->env.elmt_idx,
                                          p_env_ctlt->temp_tgt,
                                          (trans_time) ? mm_get_trans_time_ms(trans_time) : 0);

            // Check if status message must be sent
            mm_lights_ctl_check_status(p_env_ctl, event);
        } break;

        case (MM_GRP_EVENT_TRANS_END):
        {
            p_env_ctlt->temp = p_env_ctlt->temp_tgt;

            // Mark transition as finished
            SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT_TEMP, 0);

            // Inform application about state update
            mm_srv_state_upd_ind_send(MM_STATE_LIGHT_CTL_TEMP, p_env_ctlt->env.elmt_idx,
                                          p_env_ctlt->temp, 0);

            // Check if status message must be sent
            mm_lights_ctl_check_status(p_env_ctl, event);
        } break;

        case (MM_GRP_EVENT_GROUP_FULL):
        {
            // Deduce Generic Level state value from Light CTL Temperature state value
            int16_t lvl = mm_lights_ctl_temp_to_lvl(p_env_ctlt->temp, p_env_ctlt->temp_min,
                                                    p_env_ctlt->temp_max);

            // Set the current Generic Level state value
            mm_bind_set_state(p_env_ctlt->env.grp_lid, MM_STATE_TYPE_CURRENT, MM_ID_GENS_LVL, lvl);
        };

        //case (MM_GRP_EVENT_TRANS_ABORTED):
        default:
        {
        } break;
    }
}

/**
 ****************************************************************************************
 * @brief Callback function provided to the Binding Manager so that Light Lighting state
 * value can be set by main model of the group
 *
 * @param[in] mdl_lid       Model Local Index
 * @param[in] type          Type
 * @param[in] state         State value
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_cb_trans_req(m_lid_t main_mdl_lid, uint32_t req_model_id, uint8_t trans_type,
                                uint32_t state_delta)
{
    // Get environment for Light CTL Server model
    mm_lights_ctl_env_t *p_env_ctl = (mm_lights_ctl_env_t *)mm_state_get_env(main_mdl_lid);
    // Targeted Light CTL Actual state value
    uint16_t ln_tgt;

    if (req_model_id == MM_ID_LIGHTS_LN)
    {
        ln_tgt = (uint16_t)state_delta;
    }
    else
    {
        if (req_model_id == MM_ID_GENS_OO)
        {
            // Requested Generic OnOff state value
            uint8_t onoff = (uint8_t)state_delta;

            if (onoff == 0)
            {
                ln_tgt = 0;
            }
            else
            {
                ln_tgt = 0;
            }
        }
        else // (req_model_id == MM_ID_GENS_LVL)
        {
            if (trans_type == MM_TRANS_TYPE_CLASSIC)
            {
                // Requested Generic Level state value
                int16_t level = (int16_t)state_delta;

                // Light CTL Actual = Generic Level + 32768
                ln_tgt = 32768 + level;
            }
            else // ((trans_type == MM_TRANS_TYPE_DELTA) || trans_type == MM_TRANS_TYPE_MOVE))
            {
                // Delta value
                int32_t delta;

                if (trans_type == MM_TRANS_TYPE_MOVE)
                {
                    delta = (int16_t)state_delta;

                    // Keep the provided delta value
                    p_env_ctl->move_delta = (int16_t)state_delta;
                }
                else
                {
                    delta = (int32_t)state_delta;
                }

                // Add the Light CTL Actual state value to the received delta value
                delta += p_env_ctl->ln;

                // The Light CTL Actual state value cannot wrap
                if (delta < 0)
                {
                    ln_tgt = 0;
                }
                else
                {
                    ln_tgt = (delta > 0xFFFF) ? 0xFFFF : (uint16_t)delta;
                }
            }
        }

        if (ln_tgt != 0)
        {
            // Get range value
            uint16_t ln_min = mm_lights_ln_get(p_env_ctl->env.elmt_idx, MM_STATE_LIGHT_LN_RANGE_MIN);
            uint16_t ln_max = mm_lights_ln_get(p_env_ctl->env.elmt_idx, MM_STATE_LIGHT_LN_RANGE_MAX);

            // Ensure that Light CTL Lightness state value is between Light CTL Lightness Range
            // Min and Max values
            if (ln_tgt > ln_max)
            {
                ln_tgt = ln_max;
            }
            else if (ln_tgt < ln_min)
            {
                ln_tgt = ln_min;
            }
        }

    }

    // Check if Light CTL Actual state value is modified
    if (ln_tgt != p_env_ctl->ln)
    {
        p_env_ctl->ln_tgt = ln_tgt;

        SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_LN, 1);
        SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_DUV, 0);
        SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_TEMP, 0);
        SETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT,
             GETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS));

        // Start a new transition
        mm_bind_trans_new(p_env_ctl->env.grp_lid, trans_type, 0, 0);
    }
    else
    {
        // Reject the transition
        mm_bind_trans_reject(p_env_ctl->env.grp_lid);
    }
}

/**
 ****************************************************************************************
 * @brief Inform Light CTL Temperature Server model that a transition has been requested
 * using Generic Level model
 *
 * @param[in] main_mdl_lid      Light CTL Temperature Server model local index
 * @param[in] req_model_id      Model ID of model requesting the transition
 * @param[in] trans_type        Transition type
 * @param[in] state_delta       State or delta value
 ****************************************************************************************
 */
__STATIC void mm_lights_ctl_cb_trans_req_temp(m_lid_t main_mdl_lid, uint32_t req_model_id, uint8_t trans_type,
                                     uint32_t state_delta)
{
    // Get environment for Light CTL Temperature Server model
    mm_lights_ctlt_env_t *p_env_ctlt = (mm_lights_ctlt_env_t *)mm_state_get_env(main_mdl_lid);
    // Requested Generic Level state value
    int16_t lvl = (int16_t)state_delta;
    // Targeted Light CTL Temperature state value
    uint16_t temp_tgt;

    if (trans_type == MM_TRANS_TYPE_CLASSIC)
    {
        temp_tgt = mm_lights_ctl_lvl_to_temp(lvl, p_env_ctlt->temp_min, p_env_ctlt->temp_max);
    }
    else // ((trans_type == MM_TRANS_TYPE_DELTA) || trans_type == MM_TRANS_TYPE_MOVE))
    {
        // Delta value
        int32_t delta;

        if (trans_type == MM_TRANS_TYPE_MOVE)
        {
            delta = (int16_t)state_delta;

            // Keep the provided delta value
            p_env_ctlt->move_delta = delta;
        }
        else
        {
            delta = (int32_t)state_delta;
        }

        temp_tgt = mm_lights_ctl_temp_add_delta(p_env_ctlt->temp, delta,
                                                p_env_ctlt->temp_min, p_env_ctlt->temp_max);
    }

    // Ensure that Light CTL Temperature state value is between Light CTL Temperature Range
    // Min and Max values
    if (temp_tgt > p_env_ctlt->temp_max)
    {
        temp_tgt = p_env_ctlt->temp_max;
    }
    else if (temp_tgt < p_env_ctlt->temp_min)
    {
        temp_tgt = p_env_ctlt->temp_min;
    }

    // Check if Light CTL Temperature state value is modified
    if (temp_tgt != p_env_ctlt->temp)
    {
        // Get environment for Light CTL Server model
        mm_lights_ctl_env_t *p_env_ctl = p_env_ctlt->p_env_ctl;

        SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_LN, 0);
        SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_DUV, 0);
        SETB(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS_TEMP, 1);
        SETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_WAIT,
             GETF(p_env_ctl->status_info, MM_LIGHTS_CTL_SINFO_TRANS));

        p_env_ctlt->temp_tgt = temp_tgt;
        p_env_ctlt->lvl_tgt = lvl;

        // Start a new transition
        mm_bind_trans_new(p_env_ctlt->env.grp_lid, trans_type, 0, 0);
    }
    else
    {
        // Reject the transition
        mm_bind_trans_reject(p_env_ctlt->env.grp_lid);
    }
}

/**
 ****************************************************************************************
 * @brief Register Light CTL Server, Light CTL Setup Server models on a given element and
 * Light CTL Temperature Server model on next element
 *
 * @param[in] elmt_idx      Element Index
 * @param[in] p_mdl_lid     Pointer to the variable in which allocated model local index
 * will be written
 *
 * @return An error status (@see mesh_err)
 ****************************************************************************************
 */
__STATIC uint8_t mm_lights_ctls_register(uint8_t elmt_idx)
{
    m_lid_t mdl_lid, ctls_lid, ctlt_lid;

    // Register Light CTL Server model
    mdl_lid = ms_register_model(MM_ID_LIGHTS_CTL, elmt_idx, MM_CFG_PUBLI_AUTH_BIT);

    if (mdl_lid != MESH_INVALID_LID)
    {
        // Inform the Model State Manager about registered model
        mm_lights_ctl_env_t *p_env_ctl = (mm_lights_ctl_env_t *)mm_state_register(elmt_idx, MM_ID_LIGHTS_CTL, mdl_lid,
                                      MM_ROLE_SRV_PUBLI, sizeof(mm_lights_ctl_env_t));

        if (p_env_ctl)
        {
            // Get server-specific callback functions
            //p_cb_srv = p_env_ctl->env.cb.u.p_cb_srv;

            // Prepare environment for Replay Manager
            p_env_ctl->replay_env.delay_ms = MM_LIGHTS_CTL_REPLAY_MS;

            // Prepare timer for publications
            p_env_ctl->tmr_publi.cb = mm_lights_ctl_cb_tmr_publi;
            //p_env_ctl->tmr_publi.p_env = (void *)p_env_ctl;

            // Set internal callback functions
            p_env_ctl->env.mdl_cb.cb_rx = mm_lights_ctl_cb_rx;
            p_env_ctl->env.mdl_cb.cb_opcode_check = mm_lights_ctl_cb_opcode_check;
            //p_env_ctl->env.mdl_cb.cb_publish_param = mm_lights_ctl_cb_publish_param;
            //p_cb_srv->cb_set = mm_lights_ctl_cb_set;
            p_env_ctl->env.mdl_cb.cb_srv_set = mm_lights_ctl_cb_set;

            // Inform application about registered model
            mm_register_ind_send(MM_ID_LIGHTS_CTL, elmt_idx, mdl_lid);
        }

        // Register Light CTL Setup Server model
        ctls_lid = ms_register_model(MM_ID_LIGHTS_CTLS, elmt_idx, 0);

        if (ctls_lid != MESH_INVALID_LID)
        {
            // Inform the Model State Manager about registered model
            mm_lights_ctls_env_t *p_env_ctls = (mm_lights_ctls_env_t *)mm_state_register(elmt_idx, MM_ID_LIGHTS_CTLS, ctls_lid,
                                          MM_ROLE_SRV, sizeof(mm_lights_ctls_env_t));

            if (p_env_ctls)
            {
                // Set internal callback functions
                p_env_ctls->env.mdl_cb.cb_rx = mm_lights_ctl_cb_rx;
                p_env_ctls->env.mdl_cb.cb_opcode_check = mm_lights_ctl_cb_opcode_check;

                // Link environment
                p_env_ctls->p_env_ctl = p_env_ctl;

                // Inform application about registered model
                mm_register_ind_send(MM_ID_LIGHTS_CTLS, elmt_idx, ctls_lid);
            }
        }

        // Register Light CTL Temperature Server model
        ctlt_lid = ms_register_model(MM_ID_LIGHTS_CTLT, elmt_idx + 1, MM_CFG_PUBLI_AUTH_BIT);

        if (ctlt_lid != MESH_INVALID_LID)
        {
            // Inform the Model State Manager about registered model
            mm_lights_ctlt_env_t *p_env_ctlt = (mm_lights_ctlt_env_t *)mm_state_register(elmt_idx + 1, MM_ID_LIGHTS_CTLT, ctlt_lid,
                                          MM_ROLE_SRV_PUBLI, sizeof(mm_lights_ctlt_env_t));

            if (p_env_ctlt)
            {
                // Get server-specific callback functions
                //p_cb_srv = p_env_ctlt->env.cb.u.p_cb_srv;

                // Set initial range
                p_env_ctlt->temp_min = MM_LIGHTS_CTL_TEMP_MIN;
                p_env_ctlt->temp_max = MM_LIGHTS_CTL_TEMP_MAX;

                // And initial value
                p_env_ctlt->temp = MM_LIGHTS_CTL_TEMP_MIN;

                // Prepare timer for publications
                p_env_ctlt->tmr_publi.cb = mm_lights_ctl_cb_tmr_publi;
                //p_env_ctlt->tmr_publi.p_env = (void *)p_env_ctlt;

                // Set internal callback functions
                p_env_ctlt->env.mdl_cb.cb_rx = mm_lights_ctl_cb_rx;
                p_env_ctlt->env.mdl_cb.cb_opcode_check = mm_lights_ctl_cb_opcode_check;
                //p_env_ctlt->env.mdl_cb.cb_publish_param = mm_lights_ctl_cb_publish_param;
                //p_cb_srv->cb_set = mm_lights_ctl_cb_set;
                p_env_ctlt->env.mdl_cb.cb_srv_set = mm_lights_ctl_cb_set;

                // Inform application about registered model
                mm_register_ind_send(MM_ID_LIGHTS_CTLT, elmt_idx + 1, ctlt_lid);

                // Bound models together
                p_env_ctlt->p_env_ctl = p_env_ctl;
                p_env_ctl->p_env_ctlt = p_env_ctlt;
            }
        }
    }

    return (mdl_lid);
}

/*
 * GLOBAL FUNCTIONS
 ****************************************************************************************
 */

uint8_t mm_lights_ctl_register(uint8_t elmt_idx, bool main)
{
    m_lid_t mdl_lid = MESH_INVALID_LID;

    do
    {
        // Model local index for Light CTL Server model and for Light CTL Tempereture Server model
        // and for Light Lightness Server model
        m_lid_t ln_lid, lvl1_lid, ctl_lid, ctlt_lid;

        // Register Light Lightness Server model and associated models
        ln_lid = mm_lights_ln_register(elmt_idx, false);
        if (ln_lid == MESH_INVALID_LID)
        {
            break;
        }

        // Register Generic Level Server model
        lvl1_lid = mm_gens_lvl_register(elmt_idx + 1, false);
        if (lvl1_lid == MESH_INVALID_LID)
        {
            break;
        }

        // Register the Light CTL Server model and associated models
        ctl_lid = mm_lights_ctls_register(elmt_idx);

        if (main && (ctl_lid != MESH_INVALID_LID))
        {
            mdl_lid = ctl_lid;
            ctlt_lid = ctl_lid + 2;
            
            // Create group and set Light CTL Server model as main model
            m_lid_t grp_lid = mm_bind_add_group(3, elmt_idx, ctl_lid,
                                 mm_lights_ctl_cb_grp_event, mm_lights_ctl_cb_trans_req);

            // Add Generic Level Server model to the group
            mm_gens_lvl_bind_group(grp_lid, mm_state_get_lid(elmt_idx, MM_ID_GENS_LVL));
            // Add Generic OnOff Server model to the group
            mm_gens_oo_bind_group(grp_lid, mm_state_get_lid(elmt_idx, MM_ID_GENS_OO));
            // Add Light Lightness Server model to the group
            mm_lights_ln_bind_group(grp_lid, ln_lid);

            // Create group and set Light CTL Temperature Server model as main model
            grp_lid = mm_bind_add_group(1, elmt_idx + 1, ctlt_lid,
                                 mm_lights_ctl_cb_grp_event_temp, mm_lights_ctl_cb_trans_req_temp);

            // Add Generic Level Server model to the group
            mm_gens_lvl_bind_group(grp_lid, lvl1_lid);

        }
    } while (0);

    return (mdl_lid);
}

/// @} end of group
