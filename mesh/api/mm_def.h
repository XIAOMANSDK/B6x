/**
 ****************************************************************************************
 *
 * @file mm_def.h
 *
 * @brief Header file for Mesh Model Definitions
 *
 ****************************************************************************************
 */

#ifndef MM_DEF_H_
#define MM_DEF_H_

/**
 ****************************************************************************************
 * @defgroup MM_DEFINES Mesh Model Definitions
 * @ingroup MESH_MDL
 * @brief  Mesh Model Defines
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of steps for transition time value
#define MM_TRANS_TIME_STEPS_MAX     (0x3E)
/// Transition time not provided
#define MM_TRANS_TIME_UNKNOWN       (0x3F)


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// State identifier values for MESH_MDL_IND message
enum mm_state_id
{
    /// Generic OnOff state
    MM_STATE_GEN_ONOFF              = 0,
    /// Generic Level state
    MM_STATE_GEN_LVL,
    /// Generic Default Transition Time state
    MM_STATE_GEN_DTT,
    /// Generic Power Actual state
    MM_STATE_GEN_POWER_ACTUAL,
    /// Generic Power Last state
    MM_STATE_GEN_POWER_LAST,
    /// Generic Power Default state
    MM_STATE_GEN_POWER_DFLT,
    /// Generic Power Range state
    MM_STATE_GEN_POWER_RANGE,
    /// Generic OnPowerUp state
    MM_STATE_GEN_ONPOWERUP,

    /// Light Lightness
    MM_STATE_LIGHT_LN               = 50,
    /// Light Lightness Linear
    MM_STATE_LIGHT_LN_LIN,
    /// Light Lightness Last
    MM_STATE_LIGHT_LN_LAST,
    /// Light Lightness Default
    MM_STATE_LIGHT_LN_DFLT,
    /// Light Lightness Range
    MM_STATE_LIGHT_LN_RANGE,
    /// Light Lightness Range Min
    MM_STATE_LIGHT_LN_RANGE_MIN,
    /// Light Lightness Range Max
    MM_STATE_LIGHT_LN_RANGE_MAX,

    /// Light CTL Lightness
    MM_STATE_LIGHT_CTL_LN           = 100,
    /// Light CTL Temperature
    MM_STATE_LIGHT_CTL_TEMP,
    /// Light CTL Delta UV
    MM_STATE_LIGHT_CTL_DELTA_UV,
    /// Light CTL Temperature Default
    MM_STATE_LIGHT_CTL_TEMP_DFLT,
    /// Light CTL Temperature Range
    MM_STATE_LIGHT_CTL_TEMP_RANGE,
    /// Light CTL Delta UV Default
    MM_STATE_LIGHT_CTL_DELTA_UV_DFLT,

    /// Light HSL Lightness
    MM_STATE_LIGHT_HSL_LN           = 150,
    /// Light HSL Hue
    MM_STATE_LIGHT_HSL_HUE,
    /// Light HSL Saturation
    MM_STATE_LIGHT_HSL_SAT,
    /// Light HSL Target
    MM_STATE_LIGHT_HSL_TGT,
    /// Light HSL Default (Lightness + Hue + Saturation)
    MM_STATE_LIGHT_HSL_DFLT,
    /// Light HSL Lightness Default
    MM_STATE_LIGHT_HSL_DFLT_LN,
    /// Light HSL Hue Default
    MM_STATE_LIGHT_HSL_DFLT_HUE,
    /// Light HSL Saturation Default
    MM_STATE_LIGHT_HSL_DFLT_SAT,
    /// Light HSL Hue Range
    MM_STATE_LIGHT_HSL_RANGE_HUE,
    /// Light HSL Saturation Range
    MM_STATE_LIGHT_HSL_RANGE_SAT,

    /// Light xyL Lightness
    MM_STATE_LIGHT_XYL_LN           = 200,
    /// Light xyL x and y
    MM_STATE_LIGHT_XYL_XY,
    /// Light xyL Lightness Target
    MM_STATE_LIGHT_XYL_LN_TGT,
    /// Light xyL x and y Target
    MM_STATE_LIGHT_XYL_XY_TGT,
    /// Light xyL Lightness Default
    MM_STATE_LIGHT_XYL_LN_DFLT,
    /// Light xyL x and y Default
    MM_STATE_LIGHT_XYL_XY_DFLT,
    /// Light xyL x and y Range
    MM_STATE_LIGHT_XYL_XY_RANGE

    /// User add Vendor States, more...

};

/// Type of state that can be set
enum mm_state_type
{
    /// Current state value
    MM_STATE_TYPE_CURRENT           = 0,
    /// Targeted state value
    MM_STATE_TYPE_TARGET,
    /// Targeted state value during move transition
    MM_STATE_TYPE_TARGET_MOVE,
};

/// Transition type
enum mm_trans_type
{
    /// Classic Set
    MM_TRANS_TYPE_CLASSIC           = 0,
    /// Delta Set
    MM_TRANS_TYPE_DELTA,
    /// Move Set
    MM_TRANS_TYPE_MOVE,
    /// No transition
    MM_TRANS_TYPE_NONE,
};

/// Transition time step resolution values
enum mm_trans_time_step
{
    /// Resolution is 100 milliseconds
    MM_TRANS_TIME_STEP_100MS        = 0,
    /// Resolution is 1 second
    MM_TRANS_TIME_STEP_1S,
    /// Resolution is 10 seconds
    MM_TRANS_TIME_STEP_10S,
    /// Resolution is 10 minutes
    MM_TRANS_TIME_STEP_10M
};

/// Transition time format bit-field
/// 7            6                   0
/// +------------+-------------------+
/// | Resolution |  Number of steps  |
/// +------------+-------------------+
enum mm_trans_time_pos
{
    /// Number of steps
    MM_TRANS_TIME_STEP_NB_LSB       = 0,
    MM_TRANS_TIME_STEP_NB_MASK      = 0x3F,

    /// Resolution (@see enum mm_trans_time_step)
    MM_TRANS_TIME_RESOL_LSB         = 6,
    MM_TRANS_TIME_RESOL_MASK        = 0xC0,
};

/// Transition information bit-field
/// 16    8     7      6     2      0
/// +-----+-----+------+-----+------+
/// | TID | Ack | Long | RFU | Type |
/// +-----+-----+------+-----+------+
enum mm_trans_info
{
    /// Type (value depends on model for which transition is required)
    /// @see enum mm_trans_type
    MM_TRANS_INFO_TYPE_LSB          = 0,
    MM_TRANS_INFO_TYPE_MASK         = 0x0003,

    /// Include transition time and delay
    MM_TRANS_INFO_LONG_POS          = 6,
    MM_TRANS_INFO_LONG_BIT          = 0x0040,

    /// Set or Set Unacknowledged
    MM_TRANS_INFO_ACK_POS           = 7,
    MM_TRANS_INFO_ACK_BIT           = 0x0080,

    /// TID
    MM_TRANS_INFO_TID_LSB           = 8,
    MM_TRANS_INFO_TID_MASK          = 0xFF00,
};

/// Set information bit-field
/// 8     7     0
/// +-----+------+
/// | Ack | Type |
/// +-----+------+
enum mm_set_info
{
    /// Type (value depends on model for which operation is required)
    MM_SET_INFO_TYPE_LSB            = 0,
    MM_SET_INFO_TYPE_MASK           = 0x7F,

    /// Set or Set Unacknowledged
    MM_SET_INFO_ACK_POS             = 7,
    MM_SET_INFO_ACK_BIT             = 0x80,
};

/// Status of Set command
enum mm_set_status
{
    /// Command successfully processed
    MM_STATUS_SUCCESS               = 0,
    /// The provided value for Range Min cannot be set
    MM_STATUS_ERROR_RANGE_MIN,
    /// The provided value for Range Max cannot be set
    MM_STATUS_ERROR_RANGE_MAX,
};

/// Group events for communication between Binding Manager and a model
enum mm_group_event
{
    /// New transition has been rejected by the main model, sent to bound model
    MM_GRP_EVENT_TRANS_REJECTED     = 0,
    /// Delay period has expired, sent to main model
    MM_GRP_EVENT_TRANS_DELAY_EXPIRED,
    /// New Transition has been started by the main model
    MM_GRP_EVENT_TRANS_STARTED,
    /// Immediate transmission has been started by the main model
    MM_GRP_EVENT_TRANS_IMMEDIATE,
    /// Transition stopped after transition time
    MM_GRP_EVENT_TRANS_END,
    /// Transition aborted
    MM_GRP_EVENT_TRANS_ABORTED,

    /// All bound models have been registered
    MM_GRP_EVENT_GROUP_FULL,
};

/// Model register configuration
/// 7     1            0
/// +-----+------------+
/// | RFU | Publi Auth |
/// +-----+------------+
enum mm_register_config
{
    /// Indicate if sending of publications is authorized or not
    MM_CFG_PUBLI_AUTH_POS           = 0,
    MM_CFG_PUBLI_AUTH_BIT           = 0x01,
};

/// Model role
enum mm_role
{
    /// Client role
    MM_ROLE_CLI                     = 0x00,
    /// Server role
    MM_ROLE_SRV                     = 0x01,
    /// Server role with Publication
    MM_ROLE_SRV_PUBLI               = 0x03,
};

/// Information bit field for model environment - add Bit[7:4]
/// 7          6          4       3      2       1      0
/// +----------+----------+-------+------+-------+------+
/// | Wait App | On Publi | Publi | Main |     Role     |
/// +----------+----------+-------+------+-------+------+
enum mm_info
{
    /// Model Role @see mm_role
    MM_INFO_ROLE_LSB                = 0,
    MM_INFO_ROLE_MASK               = 0x03,

    /// Model is main model of its group - set by Binding Manager
    MM_INFO_MAIN_POS                = 2,
    MM_INFO_MAIN_BIT                = (1 << MM_INFO_MAIN_POS),

    /// Publication are enabled for the model
    MM_INFO_PUBLI_POS               = 3,
    MM_INFO_PUBLI_BIT               = (1 << MM_INFO_PUBLI_POS),

    /// Publication are ongoing @see mm_gens_loc_info mm_gens_bat_info
    MM_INFO_ONPUBLI_LSB             = 4,
    MM_INFO_ONPUBLI_MASK            = 0x30,
    /// Wait for confirmation from application
    MM_INFO_WAITAPP_LSB             = 6,
    MM_INFO_WAITAPP_MASK            = 0xC0,
};

/// Routing Buffer information bitfield
/// 15    3         2       1       0
/// +-----+---------+-------+-------+
/// | RFU | PUBLISH | RELAY | RX/TX |
/// +-----+---------+-------+-------+
enum mm_route_info
{
    /// RX or TX buffer (1 when RX)
    MM_ROUTE_INFO_RX_POS            = 0,
    MM_ROUTE_INFO_RX_BIT            = 0x01,

    /// Has been relayed (RX) or can be relayed (TX, only when response is sent)
    MM_ROUTE_INFO_RELAY_POS         = 1,
    MM_ROUTE_INFO_RELAY_BIT         = 0x02,

    /// Publication or response (for TX)
    MM_ROUTE_INFO_PUBLISH_POS       = 2,
    MM_ROUTE_INFO_PUBLISH_BIT       = 0x04,
};


/// @} end of group

#endif /* MM_DEF_H_ */
