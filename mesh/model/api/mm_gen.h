/**
 ****************************************************************************************
 *
 * @file mm_gen.h
 *
 * @brief Header file for Mesh Generic Model Definitions
 *
 ****************************************************************************************
 */

#ifndef MM_GEN_H_
#define MM_GEN_H_

/**
 ****************************************************************************************
 * @defgroup MM_DEFINES Mesh Model Definitions
 * @ingroup MESH_MDL
 * @brief  Mesh Model Defines
 * @{
 ****************************************************************************************
 */

/*
 * DEFINES (MODEL IDS)
 ****************************************************************************************
 */

/// ************************** Model IDs for Generic Models *****************************

/// Generic Server - OnOff
#define MM_ID_GENS_OO                   (0x1000)
/// Generic Server - Level
#define MM_ID_GENS_LVL                  (0x1002)
/// Generic Server - Default Transition Time
#define MM_ID_GENS_DTT                  (0x1004)
/// Generic Server - Power OnOff
#define MM_ID_GENS_POO                  (0x1006)
/// Generic Server - Power OnOff Setup
#define MM_ID_GENS_POOS                 (0x1007)
/// Generic Server - Power Level
#define MM_ID_GENS_PLVL                 (0x1009)
/// Generic Server - Power Level Setup
#define MM_ID_GENS_PLVLS                (0x100A)
/// Generic Server - Battery
#define MM_ID_GENS_BAT                  (0x100C)
/// Generic Server - Location
#define MM_ID_GENS_LOC                  (0x100E)
/// Generic Server - Location Setup
#define MM_ID_GENS_LOCS                 (0x100F)
/// Generic Server - User Property
#define MM_ID_GENS_UPROP                (0x1013)
/// Generic Server - Admin Property
#define MM_ID_GENS_APROP                (0x1011)
/// Generic Server - Manufacturer Property
#define MM_ID_GENS_MPROP                (0x1012)
/// Generic Server - Client Property
#define MM_ID_GENS_CPROP                (0x1014)

/// Generic Client - OnOff
#define MM_ID_GENC_OO                   (0x1001)
/// Generic Client - Level
#define MM_ID_GENC_LVL                  (0x1003)
/// Generic Client - Default Transition Time
#define MM_ID_GENC_DTT                  (0x1005)
/// Generic Client - Power OnOff
#define MM_ID_GENC_POO                  (0x1008)
/// Generic Client - Power Level
#define MM_ID_GENC_PLVL                 (0x100B)
/// Generic Client - Location
#define MM_ID_GENC_LOC                  (0x1010)
/// Generic Client - Battery
#define MM_ID_GENC_BAT                  (0x100D)
/// Generic Client - Property
#define MM_ID_GENC_PROP                 (0x1015)

/*
 * DEFINES (MESSAGE IDS)
 ****************************************************************************************
 */

/// ********************* Message IDs for Generic OnOff Model **************************

/// Generic OnOff Get
#define MM_MSG_GEN_OO_GET               (0x0182)
/// Generic OnOff Set
#define MM_MSG_GEN_OO_SET               (0x0282)
/// Generic OnOff Set Unacknowledged
#define MM_MSG_GEN_OO_SET_UNACK         (0x0382)
/// Generic OnOff Status
#define MM_MSG_GEN_OO_STATUS            (0x0482)

/// ********************* Message IDs for Generic Level Model ***************************

/// Generic Level Get
#define MM_MSG_GEN_LVL_GET              (0x0582)
/// Generic Level Set
#define MM_MSG_GEN_LVL_SET              (0x0682)
/// Generic Level Set Unacknowledged
#define MM_MSG_GEN_LVL_SET_UNACK        (0x0782)
/// Generic Level Status
#define MM_MSG_GEN_LVL_STATUS           (0x0882)
/// Generic Delta Set
#define MM_MSG_GEN_DELTA_SET            (0x0982)
/// Generic Delta Set Unacknowledged
#define MM_MSG_GEN_DELTA_SET_UNACK      (0x0A82)
/// Generic Move Set
#define MM_MSG_GEN_MOVE_SET             (0x0B82)
/// Generic Move Set Unacknowledged
#define MM_MSG_GEN_MOVE_SET_UNACK       (0x0C82)

/// ************** Message IDs for Generic Default Transition Time Model ****************

/// Generic Default Transition Time Get
#define MM_MSG_GEN_DTT_GET              (0x0D82)
/// Generic Default Transition Time Set
#define MM_MSG_GEN_DTT_SET              (0x0E82)
/// Generic Default Transition Time Set Unacknowledged
#define MM_MSG_GEN_DTT_SET_UNACK        (0x0F82)
/// Generic Default Transition Time Status
#define MM_MSG_GEN_DTT_STATUS           (0x1082)

/// ******************** Message IDs for Generic Power OnOff Model *********************

/// Generic OnPowerUp Get
#define MM_MSG_GEN_ONPUP_GET            (0x1182)
/// Generic OnPowerUp Status
#define MM_MSG_GEN_ONPUP_STATUS         (0x1282)

/// ******************** Message IDs for Generic Power OnOff Model *********************

/// Generic OnPowerUp Set
#define MM_MSG_GEN_ONPUP_SET            (0x1382)
/// Generic OnPowerUp Set Unacknowledged
#define MM_MSG_GEN_ONPUP_SET_UNACK      (0x1482)

/// ******************** Message IDs for Generic Power Level Model **********************

/// Generic Power Level Get
#define MM_MSG_GEN_PLVL_GET             (0x1582)
/// Generic Power Level Set
#define MM_MSG_GEN_PLVL_SET             (0x1682)
/// Generic Power Level Set Unacknowledged
#define MM_MSG_GEN_PLVL_SET_UNACK       (0x1782)
/// Generic Power Level Status
#define MM_MSG_GEN_PLVL_STATUS          (0x1882)
/// Generic Power Last Get
#define MM_MSG_GEN_PLAST_GET            (0x1982)
/// Generic Power Last Status
#define MM_MSG_GEN_PLAST_STATUS         (0x1A82)
/// Generic Power Default Get
#define MM_MSG_GEN_PDFLT_GET            (0x1B82)
/// Generic Power Default Status
#define MM_MSG_GEN_PDFLT_STATUS         (0x1C82)
/// Generic Power Range Get
#define MM_MSG_GEN_PRANGE_GET           (0x1D82)
/// Generic Power Range Status
#define MM_MSG_GEN_PRANGE_STATUS        (0x1E82)

/// ***************** Message IDs for Generic Power Level Setup Model *******************

/// Generic Power Default Set
#define MM_MSG_GEN_PDFLT_SET            (0x1F82)
/// Generic Power Default Set Unacknowledged
#define MM_MSG_GEN_PDFLT_SET_UNACK      (0x2082)
/// Generic Power Range Set
#define MM_MSG_GEN_PRANGE_SET           (0x2182)
/// Generic Power Range Set Unacknowledged
#define MM_MSG_GEN_PRANGE_SET_UNACK     (0x2282)

/// ***************** Message IDs for Generic Battery Model *******************

/// Generic Battery Get
#define MM_MSG_GEN_BAT_GET              (0x2382)
/// Generic Battery Status
#define MM_MSG_GEN_BAT_STATUS           (0x2482)

/// ***************** Message IDs for Generic Location Model *******************

/// Generic Location Global Get
#define MM_MSG_GEN_LOCG_GET             (0x2582)
/// Generic Location Global Status
#define MM_MSG_GEN_LOCG_STATUS          (0x40)
/// Generic Location Local Get
#define MM_MSG_GEN_LOCL_GET             (0x2682)
/// Generic Location Local Status
#define MM_MSG_GEN_LOCL_STATUS          (0x2782)

/// ***************** Message IDs for Generic Location Setup Model *******************

/// Generic Location Global Set
#define MM_MSG_GEN_LOCG_SET             (0x41)
/// Generic Location Global Set Unacknowledged
#define MM_MSG_GEN_LOCG_SET_UNACK       (0x42)
/// Generic Location Local Set
#define MM_MSG_GEN_LOCL_SET             (0x2882)
/// Generic Location Local Set Unacknowledged
#define MM_MSG_GEN_LOCL_SET_UNACK       (0x2982)

/// ***************** Message IDs for Generic Manufacturer Property Model *******************

/// Generic Manufacturer Properties Get
#define MM_MSG_GEN_MPROPS_GET           (0x2A82)
/// Generic Manufacturer Properties Status
#define MM_MSG_GEN_MPROPS_STATUS        (0x43)
/// Generic Manufacturer Property Get
#define MM_MSG_GEN_MPROP_GET            (0x2B82)
/// Generic Manufacturer Property Set
#define MM_MSG_GEN_MPROP_SET            (0x44)
/// Generic Manufacturer Property Set Unacknowledged
#define MM_MSG_GEN_MPROP_SET_UNACK      (0x45)
/// Generic Manufacturer Property Status
#define MM_MSG_GEN_MPROP_STATUS         (0x46)

/// ***************** Message IDs for Generic Admin Property Model *******************

/// Generic Admin Properties Get
#define MM_MSG_GEN_APROPS_GET           (0x2C82)
/// Generic Admin Properties Status
#define MM_MSG_GEN_APROPS_STATUS        (0x47)
/// Generic Admin Property Get
#define MM_MSG_GEN_APROP_GET            (0x2D82)
/// Generic Admin Property Set
#define MM_MSG_GEN_APROP_SET            (0x48)
/// Generic Admin Property Set Unacknowledged
#define MM_MSG_GEN_APROP_SET_UNACK      (0x49)
/// Generic Admin Property Status
#define MM_MSG_GEN_APROP_STATUS         (0x4A)

/// ***************** Message IDs for Generic User Property Model *******************

/// Generic User Properties Get
#define MM_MSG_GEN_UPROPS_GET           (0x2E82)
/// Generic User Properties Status
#define MM_MSG_GEN_UPROPS_STATUS        (0x4B)
/// Generic User Property Get
#define MM_MSG_GEN_UPROP_GET            (0x2F82)
/// Generic User Property Set
#define MM_MSG_GEN_UPROP_SET            (0x4C)
/// Generic User Property Set Unacknowledged
#define MM_MSG_GEN_UPROP_SET_UNACK      (0x4D)
/// Generic User Property Status
#define MM_MSG_GEN_UPROP_STATUS         (0x4E)

/// ***************** Message IDs for Generic Client Property Model *******************

/// Generic Client Properties Get
#define MM_MSG_GEN_CPROPS_GET           (0x4F)
/// Generic Client Properties Status
#define MM_MSG_GEN_CPROPS_STATUS        (0x50)

/*
 * DEFINES
 ****************************************************************************************
 */

/// Location Global Latitude Not Configured
#define MM_LOC_GLOBAL_LAT_NOT_CONFIG    (0x80000000)
/// Location Global Longitude Not Configured
#define MM_LOC_GLOBAL_LONG_NOT_CONFIG   (0x80000000)
/// Location Global Altitude Not Configured
#define MM_LOC_GLOBAL_ALT_NOT_CONFIG    (0x7FFF)
/// Location Local North Not Configured
#define MM_LOC_LOCAL_NORTH_NOT_CONFIG   (0x8000)
/// Location Local East Not Configured
#define MM_LOC_LOCAL_EAST_NOT_CONFIG    (0x8000)
/// Location Local Altitude Not Configured
#define MM_LOC_LOCAL_ALT_NOT_CONFIG     (0x7FFF)
/// Floor Number Not Configured
#define MM_LOC_FLOOR_NOT_CONFIG         (0xFF)

/// Battery Level Max value
#define MM_BAT_LVL_MAX                  (0x64)
/// Battery Level Unknown
#define MM_BAT_LVL_UNKNOWN              (0xFF)
/// Battery Flags (all unknown)
#define MM_BAT_FLAGS_UNKNOWN            (0xFF)
/// Battery Time to discharge unknown
#define MM_BAT_TIME_DISCHRG_UNKNOWN     (0x00FFFFFF)
/// Battery Time to charge unknown
#define MM_BAT_TIME_CHRG_UNKNOWN        (0x00FFFFFF)

/*
 * DEFINES (MESSAGE LENGTH)
 ****************************************************************************************
 */

/// Minimal length of Generic OnOff Set/Set Unacknowledged message
#define MM_GEN_OO_SET_MIN_LEN           (2)
/// Length of Generic OnOff Set/Set Unacknowledged message when all field are present
#define MM_GEN_OO_SET_LEN               (4)
/// Minimal length of Generic OnOff Status message
#define MM_GEN_OO_STATUS_MIN_LEN        (1)
/// Length of Generic OnOff Status message when all fields are present
#define MM_GEN_OO_STATUS_LEN            (3)

/// Minimal length of Generic Level Set/Set Unacknowledged message
#define MM_GEN_LVL_SET_MIN_LEN          (3)
/// Length of Generic Level Set/Set Unacknowledged message when all field are present
#define MM_GEN_LVL_SET_LEN              (5)
/// Minimal length of Generic Delta Set/Set Unacknowledged message
#define MM_GEN_LVL_SET_DELTA_MIN_LEN    (5)
/// Length of Generic Delta Set/Set Unacknowledged message when all field are present
#define MM_GEN_LVL_SET_DELTA_LEN        (7)
/// Minimal length of Generic Move Set/Set Unacknowledged message
#define MM_GEN_LVL_SET_MOVE_MIN_LEN     (3)
/// Length of Generic Move Set/Set Unacknowledged message when all field are present
#define MM_GEN_LVL_SET_MOVE_LEN         (5)
/// Minimal length of Generic Level Status message
#define MM_GEN_LVL_STATUS_MIN_LEN       (2)
/// Length of Generic Level Status message when all fields are present
#define MM_GEN_LVL_STATUS_LEN           (5)

/// Length of Generic Default Transition Time Set/Set Unacknowledged message
#define MM_GEN_DTT_SET_LEN              (1)
/// Length of Generic Default Transition Time Status message
#define MM_GEN_DTT_STATUS_LEN           (1)

/// Length of Generic OnPowerUp Set/Set Unacknowledged message
#define MM_GEN_POO_SET_LEN              (1)
/// Length of Generic OnPowerUp Status message
#define MM_GEN_POO_STATUS_LEN           (1)

/// Length of Generic Battery Status message
#define MM_GEN_BAT_STATUS_LEN           (8)

/// Length of Generic Location Global Set/Set Unacknowledged message
#define MM_GEN_LOC_SET_GLOB_LEN         (10)
/// Length of Generic Location Local Set/Set Unacknowledged message
#define MM_GEN_LOC_SET_LOC_LEN          (9)
/// Length of Generic Location Global Status message
#define MM_GEN_LOC_STATUS_GLOB_LEN      (10)
/// Length of Generic Location Local Status message
#define MM_GEN_LOC_STATUS_LOC_LEN       (9)

/// Minimal length of Generic Power Set/Set Unacknowledged message
#define MM_GEN_PLVL_SET_MIN_LEN         (3)
/// Length of Generic Power Set/Set Unacknowledged message when all field are present
#define MM_GEN_PLVL_SET_LEN             (5)
/// Minimal length of Generic Power Level Status message
#define MM_GEN_PLVL_STATUS_MIN_LEN      (2)
/// Length of Generic Power Level Status message when all fields are present
#define MM_GEN_PLVL_STATUS_LEN          (5)
/// Length of Generic Power Last Status
#define MM_GEN_PLVL_LAST_STATUS_LEN     (2)
/// Length of Generic Power Default Set/Set Unacknowledged
#define MM_GEN_PLVL_DFLT_SET_LEN        (2)
/// Length of Generic Power Default Status
#define MM_GEN_PLVL_DFLT_STATUS_LEN     (2)
/// Length of Generic Power Range Set/Set Unacknowledged
#define MM_GEN_PLVL_RANGE_SET_LEN       (4)
/// Length of Generic Power Range Status
#define MM_GEN_PLVL_RANGE_STATUS_LEN    (5)

/// Minimal length of Generic User Property Set/Set Unacknowledged message
#define MM_GEN_PROP_USER_SET_MIN_LEN    (2)
/// Minimal length of Generic Admin Property Set/Set Unacknowledged message
#define MM_GEN_PROP_ADMIN_SET_MIN_LEN   (3)
/// Length of Generic Manufacturer Property Set/Set Unacknowledged message
#define MM_GEN_PROP_MANUF_SET_LEN       (3)
/// Minimal length of Generic User/Admin/Manufacturer Property Status message
#define MM_GEN_PROP_STATUS_MIN_LEN      (2)

/*
 * ENUMERATIONS (MESSAGE CONTENT)
 ****************************************************************************************
 */

/// Positions in Generic OnOff Set/Set Unacknowledged message
enum mm_gen_oo_set_pos
{
    /// OnOff state value
    MM_GEN_OO_SET_OO_POS                = 0,
    /// TID value
    MM_GEN_OO_SET_TID_POS,
    /// Transition time value
    MM_GEN_OO_SET_TRANS_TIME_POS,
    /// Delay value
    MM_GEN_OO_SET_DELAY_POS,
};

/// Positions in Generic OnOff Status message
enum mm_gen_oo_status_pos
{
    /// Present OnOff state value
    MM_GEN_OO_STATUS_OO_POS             = 0,
    /// Target OnOff state value
    MM_GEN_OO_STATUS_TGT_OO_POS,
    /// Remaining time value
    MM_GEN_OO_STATUS_REM_TIME_POS,
};

/// Positions in Generic Level Set/Set Unacknowledged message
enum mm_gen_lvl_set_pos
{
    /// Level value
    MM_GEN_LVL_SET_LVL_POS              = 0,
    /// TID value
    MM_GEN_LVL_SET_TID_POS              = 2,
    /// Transition time value
    MM_GEN_LVL_SET_TRANS_TIME_POS,
    /// Delay value
    MM_GEN_LVL_SET_DELAY_POS,
};

/// Positions in Generic Delta Set/Set Unacknowledged message
enum mm_gen_lvl_set_delta_pos
{
    /// Delta level value
    MM_GEN_LVL_SET_DELTA_LVL_POS        = 0,
    /// TID value
    MM_GEN_LVL_SET_DELTA_TID_POS        = 4,
    /// Transition time value
    MM_GEN_LVL_SET_DELTA_TRANS_TIME_POS,
    /// Delay value
    MM_GEN_LVL_SET_DELTA_DELAY_POS,
};

/// Positions in Generic Move Set/Set Unacknowledged message
enum mm_gen_lvl_set_move_pos
{
    /// Delta level value
    MM_GEN_LVL_SET_MOVE_DELTA_LVL_POS   = 0,
    /// TID value
    MM_GEN_LVL_SET_MOVE_TID_POS         = 2,
    /// Transition time value
    MM_GEN_LVL_SET_MOVE_TRANS_TIME_POS,
    /// Delay value
    MM_GEN_LVL_SET_MOVE_DELAY_POS,
};

/// Positions in Generic Level Status message
enum mm_gen_lvl_status_pos
{
    /// Present Level value
    MM_GEN_LVL_STATUS_LVL_POS           = 0,
    /// Target Level value
    MM_GEN_LVL_STATUS_TGT_LVL_POS       = 2,
    /// Remaining time value
    MM_GEN_LVL_STATUS_REM_TIME_POS      = 4,
};

/// Positions in Generic Power Level Set/Set Unacknowledged message
enum mm_gen_plvl_set_pos
{
    /// Power value
    MM_GEN_PLVL_SET_POWER_POS           = 0,
    /// TID value
    MM_GEN_PLVL_SET_TID_POS             = 2,
    /// Transition time value
    MM_GEN_PLVL_SET_TRANS_TIME_POS,
    /// Delay value
    MM_GEN_PLVL_SET_DELAY_POS,
};

/// Positions in Generic Power Level Status message
enum mm_gen_plvl_status_pos
{
    /// Present Power value
    MM_GEN_PLVL_STATUS_PRES_POWER_POS   = 0,
    /// Target Power value
    MM_GEN_PLVL_STATUS_TGT_POWER_POS    = 2,
    /// Remaining time value
    MM_GEN_PLVL_STATUS_REM_TIME_POS     = 4,
};

/// Positions in Generic Power Last Status message
enum mm_gen_plvl_last_status_pos
{
    /// Power value
    MM_GEN_PLVL_LAST_STATUS_POWER_POS   = 0,
};

/// Positions in Generic Power Default Set/Set Unacknowledged message
enum mm_gen_plvl_dflt_set_pos
{
    /// Power value
    MM_GEN_PLVL_DFLT_SET_POWER_POS      = 0,
};

/// Positions in Generic Power Default Status message
enum mm_gen_plvl_dflt_status_pos
{
    /// Power value
    MM_GEN_PLVL_DFLT_STATUS_POWER_POS   = 0,
};

/// Positions in Generic Power Range Set/Set Unacknowledged message
enum mm_gen_plvl_range_set_pos
{
    /// Range Min
    MM_GEN_PLVL_RANGE_SET_MIN_POS       = 0,
    /// Range Max
    MM_GEN_PLVL_RANGE_SET_MAX_POS       = 2,
};

/// Positions in Generic Power Range Status message
enum mm_gen_plvl_range_status_pos
{
    /// Status code
    MM_GEN_PLVL_RANGE_STATUS_CODE_POS   = 0,
    /// Range Min
    MM_GEN_PLVL_RANGE_STATUS_MIN_POS    = 1,
    /// Range Max
    MM_GEN_PLVL_RANGE_STATUS_MAX_POS    = 3,
};

/// Positions in Generic Default Transition Time Set/Set Unacknowledged message
enum mm_gen_ddt_set_pos
{
    /// Default transition time state value
    MM_GEN_DDT_SET_DDT_POS              = 0,
};

/// Positions in Generic Default Transition Time Status message
enum mm_gen_ddt_status_pos
{
    /// Default transition time state value
    MM_GEN_DDT_STATUS_DDT_POS           = 0,
};

/// Positions in Generic OnPowerUp Set/Set Unacknowledged message
enum mm_gen_poo_set_pos
{
    /// OnPowerUp state value
    MM_GEN_POO_SET_OPU_POS              = 0,
};

/// Positions in Generic OnPowerUp Status message
enum mm_gen_poo_status_pos
{
    /// OnPowerUp state value
    MM_GEN_POO_STATUS_OPU_POS           = 0,
};

/// Positions in Generic Battery Status message
enum mm_gen_bat_status_pos
{
    /// Battery Level
    MM_GEN_BAT_STATUS_LEVEL_POS         = 0,
    /// Time to discharge
    MM_GEN_BAT_STATUS_TIME_DISCHRG_POS  = 1,
    /// Time to charge
    MM_GEN_BAT_STATUS_TIME_CHRG_POS     = 4,
    /// Flags
    MM_GEN_BAT_STATUS_FLAGS_POS         = 7,
};

/// Positions in Generic Location Global Set/Set Unacknowledged message
enum mm_gen_loc_set_glob_pos
{
    /// Global Latitude
    MM_GEN_LOC_SET_GLOB_LAT_POS         = 0,
    /// Global Longitude
    MM_GEN_LOC_SET_GLOB_LONG_POS        = 4,
    /// Global Altitude
    MM_GEN_LOC_SET_GLOB_ALT_POS         = 8,
};

/// Positions in Generic Location Global Status message
enum mm_gen_loc_status_glob_pos
{
    /// Global Latitude
    MM_GEN_LOC_STATUS_GLOB_LAT_POS      = 0,
    /// Global Longitude
    MM_GEN_LOC_STATUS_GLOB_LONG_POS     = 4,
    /// Global Altitude
    MM_GEN_LOC_STATUS_GLOB_ALT_POS      = 8,
};

/// Positions in Generic Location Local Set/Set Unacknowledged message
enum mm_gen_loc_set_loc_pos
{
    /// Local North
    MM_GEN_LOC_SET_LOC_NORTH_POS        = 0,
    /// Local East
    MM_GEN_LOC_SET_LOC_EAST_POS         = 2,
    /// Local Altitude
    MM_GEN_LOC_SET_LOC_ALT_POS          = 4,
    /// Floor Number
    MM_GEN_LOC_SET_LOC_FLOOR_POS        = 6,
    /// Uncertainty
    MM_GEN_LOC_SET_LOC_UNCERT_POS       = 7,
};

/// Positions in Generic Location Local Status message
enum mm_gen_loc_status_loc_pos
{
    /// Local North
    MM_GEN_LOC_STATUS_LOC_NORTH_POS     = 0,
    /// Local East
    MM_GEN_LOC_STATUS_LOC_EAST_POS      = 2,
    /// Local Altitude
    MM_GEN_LOC_STATUS_LOC_ALT_POS       = 4,
    /// Floor Number
    MM_GEN_LOC_STATUS_LOC_FLOOR_POS     = 6,
    /// Uncertainty
    MM_GEN_LOC_STATUS_LOC_UNCERT_POS    = 7,
};

/// Positions in Generic User Property Set/Set Unacknowledged message
enum mm_gen_prop_set_user_pos
{
    /// User Property ID
    MM_GEN_PROP_SET_USER_ID_POS         = 0,
    /// User Property value
    MM_GEN_PROP_SET_USER_VAL_POS        = 2,
};

/// Positions in Generic Admin Property Set/Set Unacknowledged message
enum mm_gen_prop_set_admin_pos
{
    /// Admin Property ID
    MM_GEN_PROP_SET_ADMIN_ID_POS        = 0,
    /// Admin User Access
    MM_GEN_PROP_SET_ADMIN_ACCESS_POS    = 2,
    /// Admin Property Value
    MM_GEN_PROP_SET_ADMIN_VAL_POS       = 3,
};

/// Positions in Generic Manufacturer Property Set/Set Unacknowledged message
enum mm_gen_prop_set_manuf_pos
{
    /// Manufacturer Property ID
    MM_GEN_PROP_SET_MANUF_ID_POS        = 0,
    /// Manufacturer User Access
    MM_GEN_PROP_SET_MANUF_ACCESS_POS    = 2,
};

/// Positions in Generic User/Admin/Manufacturer Property Status message
enum mm_gen_prop_status_pos
{
    /// Property ID
    MM_GEN_PROP_STATUS_ID_POS           = 0,
    /// User Access
    MM_GEN_PROP_STATUS_ACCESS_POS       = 2,
    /// Property value
    MM_GEN_PROP_STATUS_VALUE_POS        = 3,
};

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Generic OnPowerUp state values
enum mm_on_power_up
{
    /// Off. After being powered up, the element is in an off state
    MM_ON_POWER_UP_OFF                  = 0,
    /// Default. After being powered up, the element is in an On state and uses default
    /// state values
    MM_ON_POWER_UP_DEFAULT,
    /// Restore. If a transition was in progress when powered down, the element restores
    /// the target state when powered up. Otherwise the element restores the state it
    /// was in when powered down.
    MM_ON_POWER_UP_RESTORE,
};

/// Get type values for Generic Power Level Client model
enum mm_get_type_plvl
{
    /// Get Generic Power Actual state value
    MM_GET_TYPE_PLVL_ACTUAL             = 0,
    /// Get Generic Power Last state value
    MM_GET_TYPE_PLVL_LAST,
    /// Get Generic Power Default state value
    MM_GET_TYPE_PLVL_DFLT,
    /// Get Generic Power Range state value
    MM_GET_TYPE_PLVL_RANGE,

    /// Last option value
    MM_GET_TYPE_PLVL_MAX = MM_GET_TYPE_PLVL_RANGE,
};

/// Get type values for Generic Location Client model
enum mm_get_type_loc
{
    /// Get Generic Location Global state value
    MM_GET_TYPE_LOC_GLOBAL              = 0,
    /// Get Generic Power Last state value
    MM_GET_TYPE_LOC_LOCAL,

    /// Last option value
    MM_GET_TYPE_LOC_MAX = MM_GET_TYPE_LOC_LOCAL,
};

/// Get type values for Generic Property Client model
enum mm_get_type_prop
{
    /// Send Generic User Properties Get message
    MM_GET_TYPE_PROP_UPROPS             = 0,
    /// Send Generic User Property Get message
    MM_GET_TYPE_PROP_UPROP,
    /// Send Generic Admin Properties Get message
    MM_GET_TYPE_PROP_APROPS,
    /// Send Generic Admin Property Get message
    MM_GET_TYPE_PROP_APROP,
    /// Send Generic Manufacturer Properties Get message
    MM_GET_TYPE_PROP_MPROPS,
    /// Send Generic Manufacturer Property Get message
    MM_GET_TYPE_PROP_MPROP,
    /// Send Generic Client Properties Get message
    MM_GET_TYPE_PROP_CPROPS,

    /// Last option value
    MM_GET_TYPE_PROP_MAX = MM_GET_TYPE_PROP_CPROPS,
};

/// Set type values for the Generic Power Level Client model
enum mm_set_type_plvl
{
    /// Set Generic Power Default state value
    MM_SET_TYPE_PLVL_DFLT               = 0,
    /// Set Generic Power Range state value
    MM_SET_TYPE_PLVL_RANGE,

    /// Last option value
    MM_SET_TYPE_PLVL_MAX = MM_SET_TYPE_PLVL_RANGE,
};

/// Generic Property type
enum mm_prop_type
{
    /// User Property
    MM_PROP_TYPE_USER                   = 0,
    /// Admin Property
    MM_PROP_TYPE_ADMIN,
    /// Manufacturer Property
    MM_PROP_TYPE_MANUF,
    /// Client Property
    MM_PROP_TYPE_CLI,
};

/// @} MM_DEFINES

#endif /* MM_DEF_H_ */
