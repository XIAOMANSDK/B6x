/**
 ****************************************************************************************
 *
 * @file mm_light.h
 *
 * @brief Header file for Mesh Light Model Definitions
 *
 ****************************************************************************************
 */

#ifndef MM_LIGHT_H_
#define MM_LIGHT_H_

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

/// ************************** Model IDs for Lighting Models ****************************

/// Lighting Server - Light Lightness
#define MM_ID_LIGHTS_LN                     (0x1300)
/// Lighting Server - Light Lightness Setup
#define MM_ID_LIGHTS_LNS                    (0x1301)
/// Lighting Server - Light CTL
#define MM_ID_LIGHTS_CTL                    (0x1303)
/// Lighting Server - Light CTL Temperature
#define MM_ID_LIGHTS_CTLT                   (0x1306)
/// Lighting Server - Light CTL Setup
#define MM_ID_LIGHTS_CTLS                   (0x1304)
/// Lighting Server - Light HSL
#define MM_ID_LIGHTS_HSL                    (0x1307)
/// Lighting Server - Light HSL Hue
#define MM_ID_LIGHTS_HSLH                   (0x130A)
/// Lighting Server - Light HSL Saturation
#define MM_ID_LIGHTS_HSLSAT                 (0x130B)
/// Lighting Server - Light HSL Setup
#define MM_ID_LIGHTS_HSLS                   (0x1308)
/// Lighting Server - Light xyL
#define MM_ID_LIGHTS_XYL                    (0x130C)
/// Lighting Server - Light xyL Setup
#define MM_ID_LIGHTS_XYLS                   (0x130D)
/// Lighting Server - Light LC
#define MM_ID_LIGHTS_LC                     (0x130F)
/// Lighting Server - Light LC Setup
#define MM_ID_LIGHTS_LCS                    (0x1310)

/// Lighting Client - Light Lightness
#define MM_ID_LIGHTC_LN                     (0x1302)
/// Lighting Client - Light CTL
#define MM_ID_LIGHTC_CTL                    (0x1305)
/// Lighting Client - Light HSL
#define MM_ID_LIGHTC_HSL                    (0x1309)
/// Lighting Client - Light xyL
#define MM_ID_LIGHTC_XYL                    (0x130E)
/// Lighting Client - Light LC
#define MM_ID_LIGHTC_LC                     (0x1311)

/*
 * DEFINES (MESSAGE IDS)
 ****************************************************************************************
 */

/// ********************** Message IDs for Light Lightness Model **********************

/// Light Lightness Get
#define MM_MSG_LIGHT_LN_GET                 (0x4B82)
/// Light Lightness Set
#define MM_MSG_LIGHT_LN_SET                 (0x4C82)
/// Light Lightness Set Unacknowledged
#define MM_MSG_LIGHT_LN_SET_UNACK           (0x4D82)
/// Light Lightness Status
#define MM_MSG_LIGHT_LN_STATUS              (0x4E82)
/// Light Lightness Linear Get
#define MM_MSG_LIGHT_LN_LINEAR_GET          (0x4F82)
/// Light Lightness Linear Set
#define MM_MSG_LIGHT_LN_LINEAR_SET          (0x5082)
/// Light Lightness Linear Set Unacknowledged
#define MM_MSG_LIGHT_LN_LINEAR_SET_UNACK    (0x5182)
/// Light Lightness Linear Status
#define MM_MSG_LIGHT_LN_LINEAR_STATUS       (0x5282)
/// Light Lightness Last Get
#define MM_MSG_LIGHT_LN_LAST_GET            (0x5382)
/// Light Lightness Last Status
#define MM_MSG_LIGHT_LN_LAST_STATUS         (0x5482)
/// Light Lightness Default Get
#define MM_MSG_LIGHT_LN_DFLT_GET            (0x5582)
/// Light Lightness Default Status
#define MM_MSG_LIGHT_LN_DFLT_STATUS         (0x5682)
/// Light Lightness Range Get
#define MM_MSG_LIGHT_LN_RANGE_GET           (0x5782)
/// Light Lightness Range Status
#define MM_MSG_LIGHT_LN_RANGE_STATUS        (0x5882)

/// ***************** Message IDs for Light Lightness Setup Model *********************

/// Light Lightness Default Set
#define MM_MSG_LIGHT_LN_DFLT_SET            (0x5982)
/// Light Lightness Default Set Unacknowledged
#define MM_MSG_LIGHT_LN_DFLT_SET_UNACK      (0x5A82)
/// Light Lightness Range Set
#define MM_MSG_LIGHT_LN_RANGE_SET           (0x5B82)
/// Light Lightness Range Set Unacknowledged
#define MM_MSG_LIGHT_LN_RANGE_SET_UNACK     (0x5C82)

/// ************************* Message IDs for Light CTL Model *************************

/// Light CTL Get
#define MM_MSG_LIGHT_CTL_GET                (0x5D82)
/// Light CTL Set
#define MM_MSG_LIGHT_CTL_SET                (0x5E82)
/// Light CTL Set Unacknowledged
#define MM_MSG_LIGHT_CTL_SET_UNACK          (0x5F82)
/// Light CTL Status
#define MM_MSG_LIGHT_CTL_STATUS             (0x6082)
/// Light CTL Temperature Get
#define MM_MSG_LIGHT_CTL_TEMP_GET           (0x6182)
/// Light CTL Temperature Set
#define MM_MSG_LIGHT_CTL_TEMP_SET           (0x6482)
/// Light CTL Temperature Set Unacknowledged
#define MM_MSG_LIGHT_CTL_TEMP_SET_UNACK     (0x6582)
/// Light CTL Temperature Status
#define MM_MSG_LIGHT_CTL_TEMP_STATUS        (0x6682)
/// Light CTL Temperature Range Get
#define MM_MSG_LIGHT_CTL_TEMP_RANGE_GET     (0x6282)
/// Light CTL Temperature Range Status
#define MM_MSG_LIGHT_CTL_TEMP_RANGE_STATUS  (0x6382)
/// Light CTL Default Get
#define MM_MSG_LIGHT_CTL_DFLT_GET           (0x6782)
/// Light CTL Default Status
#define MM_MSG_LIGHT_CTL_DFLT_STATUS        (0x6882)

/// ********************* Message IDs for Light CTL Setup Model ***********************

/// Light CTL Default Set
#define MM_MSG_LIGHT_CTL_DFLT_SET             (0x6982)
/// Light CTL Default Set Unacknowledged
#define MM_MSG_LIGHT_CTL_DFLT_SET_UNACK       (0x6A82)
/// Light CTL Temperature Range Set
#define MM_MSG_LIGHT_CTL_TEMP_RANGE_SET       (0x6B82)
/// Light CTL Temperature Range Set Unacknowledged
#define MM_MSG_LIGHT_CTL_TEMP_RANGE_SET_UNACK (0x6C82)

/// ************************* Message IDs for Light HSL Model *************************

/// Light HSL Get
#define MM_MSG_LIGHT_HSL_GET                (0x6D82)
/// Light HSL Hue Get
#define MM_MSG_LIGHT_HSL_HUE_GET            (0x6E82)
/// Light HSL Hue Set
#define MM_MSG_LIGHT_HSL_HUE_SET            (0x6F82)
/// Light HSL Hue Set Unacknowledged
#define MM_MSG_LIGHT_HSL_HUE_SET_UNACK      (0x7082)
/// Light HSL Hue Status
#define MM_MSG_LIGHT_HSL_HUE_STATUS         (0x7182)
/// Light HSL Saturation Get
#define MM_MSG_LIGHT_HSL_SAT_GET            (0x7282)
/// Light HSL Saturation Set
#define MM_MSG_LIGHT_HSL_SAT_SET            (0x7382)
/// Light HSL Saturation Set Unacknowledged
#define MM_MSG_LIGHT_HSL_SAT_SET_UNACK      (0x7482)
/// Light HSL Saturation Status
#define MM_MSG_LIGHT_HSL_SAT_STATUS         (0x7582)
/// Light HSL Set
#define MM_MSG_LIGHT_HSL_SET                (0x7682)
/// Light HSL Set Unacknowledged
#define MM_MSG_LIGHT_HSL_SET_UNACK          (0x7782)
/// Light HSL Status
#define MM_MSG_LIGHT_HSL_STATUS             (0x7882)
/// Light HSL Target Get
#define MM_MSG_LIGHT_HSL_TGT_GET            (0x7982)
/// Light HSL Target Status
#define MM_MSG_LIGHT_HSL_TGT_STATUS         (0x7A82)
/// Light HSL Default Get
#define MM_MSG_LIGHT_HSL_DFLT_GET           (0x7B82)
/// Light HSL Default Status
#define MM_MSG_LIGHT_HSL_DFLT_STATUS        (0x7C82)
/// Light HSL Range Get
#define MM_MSG_LIGHT_HSL_RANGE_GET          (0x7D82)
/// Light HSL Range Status
#define MM_MSG_LIGHT_HSL_RANGE_STATUS       (0x7E82)

/// ********************* Message IDs for Light HSL Setup Model ***********************

/// Light HSL Default Set
#define MM_MSG_LIGHT_HSL_DFLT_SET           (0x7F82)
/// Light HSL Default Set Unacknowledged
#define MM_MSG_LIGHT_HSL_DFLT_SET_UNACK     (0x8082)
/// Light HSL Range Set
#define MM_MSG_LIGHT_HSL_RANGE_SET          (0x8182)
/// Light HSL Range Set Unacknowledged
#define MM_MSG_LIGHT_HSL_RANGE_SET_UNACK    (0x8282)

/// ************************* Message IDs for Light xyL Model *************************

/// Light xyL Get
#define MM_MSG_LIGHT_XYL_GET                (0x8382)
/// Light xyL Set
#define MM_MSG_LIGHT_XYL_SET                (0x8482)
/// Light xyL Set Unacknowledged
#define MM_MSG_LIGHT_XYL_SET_UNACK          (0x8582)
/// Light xyL Status
#define MM_MSG_LIGHT_XYL_STATUS             (0x8682)
/// Light xyL Target Get
#define MM_MSG_LIGHT_XYL_TGT_GET            (0x8782)
/// Light xyL Target Status
#define MM_MSG_LIGHT_XYL_TGT_STATUS         (0x8882)
/// Light HSL Default Get
#define MM_MSG_LIGHT_XYL_DFLT_GET           (0x8982)
/// Light xyL Default Status
#define MM_MSG_LIGHT_XYL_DFLT_STATUS        (0x8A82)
/// Light xyL Range Get
#define MM_MSG_LIGHT_XYL_RANGE_GET          (0x8B82)
/// Light xyL Range Status
#define MM_MSG_LIGHT_XYL_RANGE_STATUS       (0x8C82)

/// ********************* Message IDs for Light xyL Setup Model ***********************

/// Light xyL Default Set
#define MM_MSG_LIGHT_XYL_DFLT_SET           (0x8D82)
/// Light xyL Default Set Unacknowledged
#define MM_MSG_LIGHT_XYL_DFLT_SET_UNACK     (0x8E82)
/// Light xyL Range Set
#define MM_MSG_LIGHT_XYL_RANGE_SET          (0x8F82)
/// Light xyL Range Set Unacknowledged
#define MM_MSG_LIGHT_XYL_RANGE_SET_UNACK    (0x9082)

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * DEFINES (MESSAGE LENGTH)
 ****************************************************************************************
 */

/// Minimal length of Light Lightness Set/Set Unacknowledged message
#define MM_LIGHT_LN_SET_MIN_LEN             (3)
/// Length of Light Lightness Set/Set Unacknowledged message
#define MM_LIGHT_LN_SET_LEN                 (5)
/// Minimal length of Light Lightness Status message
#define MM_LIGHT_LN_STATUS_MIN_LEN          (2)
/// Length of Light Lightness Status message
#define MM_LIGHT_LN_STATUS_LEN              (5)
/// Minimal length of Light Lightness Linear Set/Set Unacknowledged message
#define MM_LIGHT_LN_LIN_SET_MIN_LEN         (3)
/// Length of Light Lightness Linear Set/Set Unacknowledged message
#define MM_LIGHT_LN_LIN_SET_LEN             (5)
/// Minimal length of Light Lightness Linear Status message
#define MM_LIGHT_LN_LIN_STATUS_MIN_LEN      (2)
/// Length of Light Lightness Linear Status message
#define MM_LIGHT_LN_LIN_STATUS_LEN          (5)
/// Length of Light Lightness Last Status message
#define MM_LIGHT_LN_LAST_STATUS_LEN         (2)
/// Length of Light Lightness Default Set/Set Unacknowledged message
#define MM_LIGHT_LN_DFLT_SET_LEN            (2)
/// Length of Light Lightness Default Status message
#define MM_LIGHT_LN_DFLT_STATUS_LEN         (2)
/// Length of Light Lightness Range Set/Set Unacknowledged message
#define MM_LIGHT_LN_RANGE_SET_LEN           (4)
/// Length of Light Lightness Range Status message
#define MM_LIGHT_LN_RANGE_STATUS_LEN        (5)

/// Minimal length of Light CTL Set/Set Unacknowledged message
#define MM_LIGHT_CTL_SET_MIN_LEN            (7)
/// Length of Light CTL Set//Set Unacknowledged message
#define MM_LIGHT_CTL_SET_LEN                (9)
/// Minimal length of Light CTL Status message
#define MM_LIGHT_CTL_STATUS_MIN_LEN         (4)
/// Length of Light CTL Status message
#define MM_LIGHT_CTL_STATUS_LEN             (9)
/// Minimal length of Light CTL Temperature Set/Set Unacknowledged message
#define MM_LIGHT_CTL_TEMP_SET_MIN_LEN       (5)
/// Length of Light CTL Temperature Set//Set Unacknowledged message
#define MM_LIGHT_CTL_TEMP_SET_LEN           (7)
/// Minimal length of Light CTL Temperature Status message
#define MM_LIGHT_CTL_TEMP_STATUS_MIN_LEN    (4)
/// Length of Light CTL Temperature Status message
#define MM_LIGHT_CTL_TEMP_STATUS_LEN        (9)
/// Length of Light CTL Temperature Range Set/Set Unacknowledged
#define MM_LIGHT_CTL_TEMP_RANGE_SET_LEN     (4)
/// Length of Light CTL Temperature Range Status
#define MM_LIGHT_CTL_TEMP_RANGE_STATUS_LEN  (5)
/// Length of Light CTL Default Set/Unacknowledged message
#define MM_LIGHT_CTL_DFLT_SET_LEN           (6)
/// Length of Light CTL Default Status message
#define MM_LIGHT_CTL_DFLT_STATUS_LEN        (6)

/// Minimal length of Light HSL Set/Set Unacknowledged message
#define MM_LIGHT_HSL_SET_MIN_LEN            (7)
/// Length of Light HSL Set//Set Unacknowledged message
#define MM_LIGHT_HSL_SET_LEN                (9)
/// Minimal length of Light HSL Status message
#define MM_LIGHT_HSL_STATUS_MIN_LEN         (6)
/// Length of Light HSL Status message
#define MM_LIGHT_HSL_STATUS_LEN             (7)
/// Minimal length of Light HSL Target Status message
#define MM_LIGHT_HSL_TGT_STATUS_MIN_LEN     (6)
/// Length of Light HSL Target Status message
#define MM_LIGHT_HSL_TGT_STATUS_LEN         (7)
/// Minimal length of Light HSL Hue Set/Set Unacknowledged message
#define MM_LIGHT_HSL_HUE_SET_MIN_LEN        (3)
/// Length of Light HSL Hue Set//Set Unacknowledged message
#define MM_LIGHT_HSL_HUE_SET_LEN            (5)
/// Minimal length of Light HSL Hue Status message
#define MM_LIGHT_HSL_HUE_STATUS_MIN_LEN     (2)
/// Length of Light HSL Hue Status message
#define MM_LIGHT_HSL_HUE_STATUS_LEN         (5)
/// Minimal length of Light HSL Saturation Set/Set Unacknowledged message
#define MM_LIGHT_HSL_SAT_SET_MIN_LEN        (3)
/// Length of Light HSL Saturation Set//Set Unacknowledged message
#define MM_LIGHT_HSL_SAT_SET_LEN            (5)
/// Minimal length of Light HSL Saturation Status message
#define MM_LIGHT_HSL_SAT_STATUS_MIN_LEN     (2)
/// Length of Light HSL Saturation Status message
#define MM_LIGHT_HSL_SAT_STATUS_LEN         (5)
/// Length of Light HSL Default Set/Set Unacknowledged message
#define MM_LIGHT_HSL_DFLT_SET_LEN           (6)
/// Length of Light HSL Default Status message
#define MM_LIGHT_HSL_DFLT_STATUS_LEN        (6)
/// Length of Light HSL Range Set/Set Unacknowledged message
#define MM_LIGHT_HSL_RANGE_SET_LEN          (8)
/// Length of Light HSL Range Status message
#define MM_LIGHT_HSL_RANGE_STATUS_LEN       (9)

/// Minimal length of Light xyL Set/Set Unacknowledged message
#define MM_LIGHT_XYL_SET_MIN_LEN            (7)
/// Length of Light xyL Set//Set Unacknowledged message
#define MM_LIGHT_XYL_SET_LEN                (9)
/// Minimal length of Light xyL Status message
#define MM_LIGHT_XYL_STATUS_MIN_LEN         (6)
/// Length of Light xyL Status message
#define MM_LIGHT_XYL_STATUS_LEN             (7)
/// Minimal length of Light xyL Target Status message
#define MM_LIGHT_XYL_TGT_STATUS_MIN_LEN     (6)
/// Length of Light xyL Target Status message
#define MM_LIGHT_XYL_TGT_STATUS_LEN         (7)
/// Length of Light xyL Default Set/Set Unacknowledged message
#define MM_LIGHT_XYL_DFLT_SET_LEN           (6)
/// Length of Light xyL Default Status message
#define MM_LIGHT_XYL_DFLT_STATUS_LEN        (6)
/// Length of Light xyL Range Set/Set Unacknowledged message
#define MM_LIGHT_XYL_RANGE_SET_LEN          (8)
/// Length of Light xyL Range Status message
#define MM_LIGHT_XYL_RANGE_STATUS_LEN       (9)

/*
 * ENUMERATIONS (MESSAGE CONTENT)
 ****************************************************************************************
 */

/// Positions in Light Lightness Set/Set Unacknowledged message
enum mm_light_ln_set_pos
{
    /// Lightness
    MM_LIGHT_LN_SET_LIGHTNESS_POS           = 0,
    /// TID
    MM_LIGHT_LN_SET_TID_POS                 = 2,
    /// Transition Time
    MM_LIGHT_LN_SET_TRANS_TIME_POS          = 3,
    /// Delay
    MM_LIGHT_LN_SET_DELAY_POS               = 4
};

/// Positions in Light Lightness Status message
enum mm_light_ln_status_pos
{
    /// Present Lightness
    MM_LIGHT_LN_STATUS_LIGHTNESS_POS        = 0,
    /// Target Lightness
    MM_LIGHT_LN_STATUS_TGT_LIGHTNESS_POS    = 2,
    /// Remaining Time
    MM_LIGHT_LN_STATUS_REM_TIME_POS         = 4,
};

/// Positions in Light Lightness Linear Set/Set Unacknowledged message
enum mm_light_ln_lin_set_pos
{
    /// Lightness
    MM_LIGHT_LN_LIN_SET_LIGHTNESS_POS       = 0,
    /// TID
    MM_LIGHT_LN_LIN_SET_TID_POS             = 2,
    /// Transition Time
    MM_LIGHT_LN_LIN_SET_TRANS_TIME_POS      = 3,
    /// Delay
    MM_LIGHT_LN_LIN_SET_DELAY_POS           = 4
};

/// Positions in Light Lightness Linear Status message
enum mm_light_ln_lin_status_pos
{
    /// Present Lightness
    MM_LIGHT_LN_LIN_STATUS_LIGHTNESS_POS     = 0,
    /// Target Lightness
    MM_LIGHT_LN_LIN_STATUS_TGT_LIGHTNESS_POS = 2,
    /// Remaining Time
    MM_LIGHT_LN_LIN_STATUS_REM_TIME_POS      = 4,
};

/// Positions in Light Lightness Last Status message
enum mm_light_ln_last_status_pos
{
    /// Lightness
    MM_LIGHT_LN_LAST_STATUS_LIGHTNESS_POS   = 0,
};

/// Positions in Light Lightness Default Set/Set Unacknowledged message
enum mm_light_ln_dflt_set_pos
{
    /// Lightness
    MM_LIGHT_LN_DFLT_SET_LIGHTNESS_POS      = 0,
};

/// Positions in Light Lightness Linear Status message
enum mm_light_ln_dflt_status_pos
{
    /// Lightness
    MM_LIGHT_LN_DFLT_STATUS_LIGHTNESS_POS   = 0,
};

/// Positions in Light Lightness Range Set/Set Unacknowledged message
enum mm_light_ln_range_set_pos
{
    /// Range Min
    MM_LIGHT_LN_RANGE_SET_MIN_POS           = 0,
    /// Range Max
    MM_LIGHT_LN_RANGE_SET_MAX_POS           = 2,
};

/// Positions in Light Lightness Range Status message
enum mm_light_ln_range_status_pos
{
    /// Status Code
    MM_LIGHT_LN_RANGE_STATUS_CODE_POS       = 0,
    /// Range Min
    MM_LIGHT_LN_RANGE_STATUS_MIN_POS        = 1,
    /// Range Max
    MM_LIGHT_LN_RANGE_STATUS_MAX_POS        = 3,
};

/// Positions in Light CTL Set/Set Unacknowledged message
enum mm_light_ctl_set_pos
{
    /// CTL Lightness
    MM_LIGHT_CTL_SET_LIGHTNESS_POS          = 0,
    /// CTL Temperature
    MM_LIGHT_CTL_SET_TEMP_POS               = 2,
    /// CTL Delta UV
    MM_LIGHT_CTL_SET_DELTA_UV_POS           = 4,
    /// TID
    MM_LIGHT_CTL_SET_TID_POS                = 6,
    /// Transition Time
    MM_LIGHT_CTL_SET_TRANS_TIME_POS         = 7,
    /// Delay
    MM_LIGHT_CTL_SET_DELAY_POS              = 8
};

/// Positions in Light CTL Status message
enum mm_light_ctl_status_pos
{
    /// Present CTL Lightness
    MM_LIGHT_CTL_STATUS_LIGHTNESS_POS       = 0,
    /// Present CTL Temperature
    MM_LIGHT_CTL_STATUS_TEMP_POS            = 2,
    /// Target CTL Lightness
    MM_LIGHT_CTL_STATUS_TGT_LIGHTNESS_POS   = 4,
    /// Target CTL Temperature
    MM_LIGHT_CTL_STATUS_TGT_TEMP_POS        = 6,
    /// Remaining Time
    MM_LIGHT_CTL_STATUS_REM_TIME_POS        = 8,
};

/// Positions in Light CTL Temperature Set/Set Unacknowledged message
enum mm_light_ctl_temp_set_pos
{
    /// CTL Temperature
    MM_LIGHT_CTL_TEMP_SET_TEMP_POS          = 0,
    /// CTL Delta UV
    MM_LIGHT_CTL_TEMP_SET_DELTA_UV_POS      = 2,
    /// TID
    MM_LIGHT_CTL_TEMP_SET_TID_POS           = 4,
    /// Transition Time
    MM_LIGHT_CTL_TEMP_SET_TRANS_TIME_POS    = 5,
    /// Delay
    MM_LIGHT_CTL_TEMP_SET_DELAY_POS         = 6
};

/// Positions in Light CTL Temperature Status message
enum mm_light_ctl_temp_status_pos
{
    /// Present CTL Temperature
    MM_LIGHT_CTL_TEMP_STATUS_TEMP_POS         = 0,
    /// Present CTL Delta UV
    MM_LIGHT_CTL_TEMP_STATUS_DELTA_UV_POS     = 2,
    /// Target CTL Temperature
    MM_LIGHT_CTL_TEMP_STATUS_TGT_TEMP_POS     = 4,
    /// Target CTL Delta UV
    MM_LIGHT_CTL_TEMP_STATUS_TGT_DELTA_UV_POS = 6,
    /// Remaining Time
    MM_LIGHT_CTL_TEMP_STATUS_REM_TIME_POS     = 8,
};

/// Positions in Light CTL Temperature Range Set/Set Unacknowledged message
enum mm_light_ctl_temp_range_set_pos
{
    /// Range Min
    MM_LIGHT_CTL_TEMP_RANGE_SET_MIN_POS     = 0,
    /// Range Max
    MM_LIGHT_CTL_TEMP_RANGE_SET_MAX_POS     = 2,
};

/// Positions in Light CTL Temperature Range Status message
enum mm_light_ctl_temp_range_status_pos
{
    /// Status Code
    MM_LIGHT_CTL_TEMP_RANGE_STATUS_CODE_POS = 0,
    /// Range Min
    MM_LIGHT_CTL_TEMP_RANGE_STATUS_MIN_POS  = 1,
    /// Range Max
    MM_LIGHT_CTL_TEMP_RANGE_STATUS_MAX_POS  = 3,
};

/// Positions in Light CTL Default Set/Set Unacknowledged message
enum mm_light_ctl_dflt_set_pos
{
    /// Lightness
    MM_LIGHT_CTL_DFLT_SET_LIGHTNESS_POS     = 0,
    /// Temperature
    MM_LIGHT_CTL_DFLT_SET_TEMP_POS          = 2,
    /// Delta UV
    MM_LIGHT_CTL_DFLT_SET_DELTA_UV_POS      = 4,
};

/// Positions in Light CTL Default Status message
enum mm_light_ctl_dflt_status_pos
{
    /// Lightness
    MM_LIGHT_CTL_DFLT_STATUS_LIGHTNESS_POS  = 0,
    /// Temperature
    MM_LIGHT_CTL_DFLT_STATUS_TEMP_POS       = 2,
    /// Delta UV
    MM_LIGHT_CTL_DFLT_STATUS_DELTA_UV_POS   = 4,
};

/// Positions in Light HSL Set/Set Unacknowledged message
enum mm_light_hsl_set_pos
{
    /// HSL Lightness
    MM_LIGHT_HSL_SET_LIGHTNESS_POS          = 0,
    /// HSL Hue
    MM_LIGHT_HSL_SET_HUE_POS                = 2,
    /// HSL Saturation
    MM_LIGHT_HSL_SET_SAT_POS                = 4,
    /// TID
    MM_LIGHT_HSL_SET_TID_POS                = 6,
    /// Transition Time
    MM_LIGHT_HSL_SET_TRANS_TIME_POS         = 7,
    /// Delay
    MM_LIGHT_HSL_SET_DELAY_POS              = 8
};

/// Positions in Light HSL Status message
enum mm_light_hsl_status_pos
{
    /// HSL Lightness
    MM_LIGHT_HSL_STATUS_LIGHTNESS_POS       = 0,
    /// HSL Hue
    MM_LIGHT_HSL_STATUS_HUE_POS             = 2,
    /// HSL Saturation
    MM_LIGHT_HSL_STATUS_SAT_POS             = 4,
    /// Remaining Time
    MM_LIGHT_HSL_STATUS_REM_TIME_POS        = 6,
};

/// Positions in Light HSL Target Status message
enum mm_light_hsl_tgt_status_pos
{
    /// Target HSL Lightness
    MM_LIGHT_HSL_TGT_STATUS_LIGHTNESS_POS   = 0,
    /// Target HSL Hue
    MM_LIGHT_HSL_TGT_STATUS_HUE_POS         = 2,
    /// Target HSL Saturation
    MM_LIGHT_HSL_TGT_STATUS_SAT_POS         = 4,
    /// Remaining Time
    MM_LIGHT_HSL_TGT_STATUS_REM_TIME_POS    = 6,
};

/// Positions in Light HSL Hue Set/Set Unacknowledged message
enum mm_light_hsl_hue_set_pos
{
    /// Hue
    MM_LIGHT_HSL_HUE_SET_HUE_POS            = 0,
    /// TID
    MM_LIGHT_HSL_HUE_SET_TID_POS            = 2,
    /// Transition Time
    MM_LIGHT_HSL_HUE_SET_TRANS_TIME_POS     = 3,
    /// Delay
    MM_LIGHT_HSL_HUE_SET_DELAY_POS          = 4
};

/// Positions in Light HSL Hue Status message
enum mm_light_hsl_hue_status_pos
{
    /// Present Hue
    MM_LIGHT_HSL_HUE_STATUS_HUE_POS         = 0,
    /// Target Hue
    MM_LIGHT_HSL_HUE_STATUS_TGT_HUE_POS     = 2,
    /// Remaining Time
    MM_LIGHT_HSL_HUE_STATUS_REM_TIME_POS    = 4,
};

/// Positions in Light HSL Saturation Set/Set Unacknowledged message
enum mm_light_hsl_sat_set_pos
{
    /// Saturation
    MM_LIGHT_HSL_SAT_SET_SAT_POS            = 0,
    /// TID
    MM_LIGHT_HSL_SAT_SET_TID_POS            = 2,
    /// Transition Time
    MM_LIGHT_HSL_SAT_SET_TRANS_TIME_POS     = 3,
    /// Delay
    MM_LIGHT_HSL_SAT_SET_DELAY_POS          = 4
};

/// Positions in Light HSL Saturation Status message
enum mm_light_hsl_sat_status_pos
{
    /// Present Saturation
    MM_LIGHT_HSL_SAT_STATUS_SAT_POS         = 0,
    /// Target Saturation
    MM_LIGHT_HSL_SAT_STATUS_TGT_SAT_POS     = 2,
    /// Remaining Time
    MM_LIGHT_HSL_SAT_STATUS_REM_TIME_POS    = 4,
};

/// Positions in Light HSL Default Set/Set Unacknowledged message
enum mm_light_hsl_dflt_set_pos
{
    /// Lightness
    MM_LIGHT_HSL_DFLT_SET_LIGHTNESS_POS     = 0,
    /// Hue
    MM_LIGHT_HSL_DFLT_SET_HUE_POS           = 2,
    /// Saturation
    MM_LIGHT_HSL_DFLT_SET_SAT_POS           = 4,
};

/// Positions in Light HSL Default Status message
enum mm_light_hsl_dflt_status_pos
{
    /// Lightness
    MM_LIGHT_HSL_DFLT_STATUS_LIGHTNESS_POS  = 0,
    /// Hue
    MM_LIGHT_HSL_DFLT_STATUS_HUE_POS        = 2,
    /// Saturation
    MM_LIGHT_HSL_DFLT_STATUS_SAT_POS        = 4,
};

/// Positions in Light HSL Range Set/Set Unacknowledged message
enum mm_light_hsl_range_set_pos
{
    /// Hue Range Min
    MM_LIGHT_HSL_RANGE_SET_HUE_MIN_POS      = 0,
    /// Hue Range Max
    MM_LIGHT_HSL_RANGE_SET_HUE_MAX_POS      = 2,
    /// Saturation Range Min
    MM_LIGHT_HSL_RANGE_SET_SAT_MIN_POS      = 4,
    /// Saturation Range Max
    MM_LIGHT_HSL_RANGE_SET_SAT_MAX_POS      = 6,
};

/// Positions in Light HSL Range Status message
enum mm_light_hsl_range_status_pos
{
    /// Status
    MM_LIGHT_HSL_RANGE_STATUS_CODE_POS      = 0,
    /// Hue Range Min
    MM_LIGHT_HSL_RANGE_STATUS_HUE_MIN_POS   = 1,
    /// Hue Range Max
    MM_LIGHT_HSL_RANGE_STATUS_HUE_MAX_POS   = 3,
    /// Saturation Range Min
    MM_LIGHT_HSL_RANGE_STATUS_SAT_MIN_POS   = 5,
    /// Saturation Range Max
    MM_LIGHT_HSL_RANGE_STATUS_SAT_MAX_POS   = 7,
};

/// Positions in Light xyL Set/Set Unacknowledged message
enum mm_light_xyl_set_pos
{
    /// xyL Lightness
    MM_LIGHT_XYL_SET_LIGHTNESS_POS          = 0,
    /// xyL x
    MM_LIGHT_XYL_SET_X_POS                  = 2,
    /// xyL y
    MM_LIGHT_XYL_SET_Y_POS                  = 4,
    /// TID
    MM_LIGHT_XYL_SET_TID_POS                = 6,
    /// Transition Time
    MM_LIGHT_XYL_SET_TRANS_TIME_POS         = 7,
    /// Delay
    MM_LIGHT_XYL_SET_DELAY_POS              = 8
};

/// Positions in Light xyL Status message
enum mm_light_xyl_status_pos
{
    /// xyL Lightness
    MM_LIGHT_XYL_STATUS_LIGHTNESS_POS       = 0,
    /// xyL x
    MM_LIGHT_XYL_STATUS_X_POS               = 2,
    /// xyL y
    MM_LIGHT_XYL_STATUS_Y_POS               = 4,
    /// Remaining Time
    MM_LIGHT_XYL_STATUS_REM_TIME_POS        = 6,
};

/// Positions in Light xyL Target Status message
enum mm_light_xyl_tgt_status_pos
{
    /// Target xyL Lightness
    MM_LIGHT_XYL_TGT_STATUS_LIGHTNESS_POS   = 0,
    /// Target xyL x
    MM_LIGHT_XYL_TGT_STATUS_X_POS           = 2,
    /// Target xyL y
    MM_LIGHT_XYL_TGT_STATUS_Y_POS           = 4,
    /// Remaining Time
    MM_LIGHT_XYL_TGT_STATUS_REM_TIME_POS    = 6,
};

/// Positions in Light xyL Default Set/Set Unacknowledged message
enum mm_light_xyl_dflt_set_pos
{
    /// Lightness
    MM_LIGHT_XYL_DFLT_SET_LIGHTNESS_POS     = 0,
    /// xyL x
    MM_LIGHT_XYL_DFLT_SET_X_POS             = 2,
    /// xyL y
    MM_LIGHT_XYL_DFLT_SET_Y_POS             = 4,
};

/// Positions in Light xyL Default Status message
enum mm_light_xyl_dflt_status_pos
{
    /// Lightness
    MM_LIGHT_XYL_DFLT_STATUS_LIGHTNESS_POS  = 0,
    /// xyL x
    MM_LIGHT_XYL_DFLT_STATUS_X_POS          = 2,
    /// xyL y
    MM_LIGHT_XYL_DFLT_STATUS_Y_POS          = 4,
};

/// Positions in Light xyL Range Set/Set Unacknowledged message
enum mm_light_xyl_range_set_pos
{
    /// xyL x Range Min
    MM_LIGHT_XYL_RANGE_SET_X_MIN_POS        = 0,
    /// xyL x Range Max
    MM_LIGHT_XYL_RANGE_SET_X_MAX_POS        = 2,
    /// xyL y Range Min
    MM_LIGHT_XYL_RANGE_SET_Y_MIN_POS        = 4,
    /// xyL y Range Max
    MM_LIGHT_XYL_RANGE_SET_Y_MAX_POS        = 6,
};

/// Positions in Light xyL Range Status message
enum mm_light_xyl_range_status_pos
{
    /// Status code
    MM_LIGHT_XYL_RANGE_STATUS_CODE          = 0,
    /// xyL x Range Min
    MM_LIGHT_XYL_RANGE_STATUS_X_MIN_POS     = 1,
    /// xyL x Range Max
    MM_LIGHT_XYL_RANGE_STATUS_X_MAX_POS     = 3,
    /// xyL y Range Min
    MM_LIGHT_XYL_RANGE_STATUS_Y_MIN_POS     = 5,
    /// xyL y Range Max
    MM_LIGHT_XYL_RANGE_STATUS_Y_MAX_POS     = 7,
};

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Get type values for Light Lightness Client model
enum mm_get_type_light_ln
{
    /// Get Light Lightness state value
    MM_GET_TYPE_LIGHT_LN_ACTUAL             = 0,
    /// Get Light Lightness Linear state value
    MM_GET_TYPE_LIGHT_LN_LINEAR,
    /// Get Light Lightness Default state value
    MM_GET_TYPE_LIGHT_LN_DFLT,
    /// Get Light Lightness Last state value
    MM_GET_TYPE_LIGHT_LN_LAST,
    /// Get Light Lightness Range state value
    MM_GET_TYPE_LIGHT_LN_RANGE,

    /// Last option value
    MM_GET_TYPE_LIGHT_LN_MAX = MM_GET_TYPE_LIGHT_LN_RANGE,
};

/// Get type values for Light CTL Client model
enum mm_get_type_light_ctl
{
    /// Get Light CTL Lightness and Light CTL Temperature state value
    MM_GET_TYPE_LIGHT_CTL                   = 0,
    /// Get Light CTL Temperature and Light CTL Delta UV state value
    MM_GET_TYPE_LIGHT_CTL_TEMP,
    /// Get Light CTL Temperature Range state value
    MM_GET_TYPE_LIGHT_CTL_TEMP_RANGE,
    /// Get Light Lightness Default and Light CTL Temperature Default and Light CTL
    /// Delta UV Default state values
    MM_GET_TYPE_LIGHT_CTL_DFLT,

    /// Last option value
    MM_GET_TYPE_LIGHT_CTL_MAX = MM_GET_TYPE_LIGHT_CTL_DFLT,
};

/// Get type values for Light HSL Client model
enum mm_get_type_light_hsl
{
    /// Get Light HSL Lightness and Light HSL Hue and Light HSL Saturation state values
    MM_GET_TYPE_LIGHT_HSL                   = 0,
    /// Get Light HSL Hue state value
    MM_GET_TYPE_LIGHT_HSL_HUE,
    /// Get Light HSL Saturation state value
    MM_GET_TYPE_LIGHT_HSL_SAT,
    /// Get Light HSL Lightness and Light HSL Hue and Light HSL Saturation target state values
    MM_GET_TYPE_LIGHT_HSL_TGT,
    /// Get Light Lightness and Light HSL Hue and Light HSL Saturation default state values
    MM_GET_TYPE_LIGHT_HSL_DFLT,
    /// Get Light HSL Hue and Light HSL Saturation state range values
    MM_GET_TYPE_LIGHT_HSL_RANGE,

    /// Last option value
    MM_GET_TYPE_LIGHT_HSL_MAX = MM_GET_TYPE_LIGHT_HSL_RANGE,
};

/// Get type values for Light xyL Client model
enum mm_get_type_light_xyl
{
    /// Get Light xyL Lightness and Light xyL x and Light xyL y state values
    MM_GET_TYPE_LIGHT_XYL                   = 0,
    /// Get Light xyL Lightness and Light xyL x and Light xyL y state target values
    MM_GET_TYPE_LIGHT_XYL_TGT,
    /// Get Light Lightness and Light xyL x and Light xyL y state default values
    MM_GET_TYPE_LIGHT_XYL_DFLT,
    /// Get Light xyL x and Light xyL y state range values
    MM_GET_TYPE_LIGHT_XYL_RANGE,

    /// Last option value
    MM_GET_TYPE_LIGHT_XYL_MAX = MM_GET_TYPE_LIGHT_XYL_RANGE,
};

/// Set type values for the Light Lightness Client model
enum mm_set_type_light_ln
{
    /// Set Light Lightness Default state value
    MM_SET_TYPE_LIGHT_LN_DFLT               = 0,
    /// Set Light Lightness Range state value
    MM_SET_TYPE_LIGHT_LN_RANGE,

    /// Last option value
    MM_SET_TYPE_LIGHT_LN_MAX = MM_SET_TYPE_LIGHT_LN_RANGE,
};

/// Set type values for the Light CTL Client model
enum mm_set_type_light_ctl
{
    /// Set Light CTL Temperature Range state value
    MM_SET_TYPE_LIGHT_CTL_TEMP_RANGE        = 0,
    /// Set Light CTL Default state value
    MM_SET_TYPE_LIGHT_CTL_DFLT,

    /// Last option value
    MM_SET_TYPE_LIGHT_CTL_MAX = MM_SET_TYPE_LIGHT_CTL_DFLT,
};

/// Set type values for the Light HSL Client model
enum mm_set_type_light_hsl
{
    /// Set Light HSL Hue and Light HSL Saturation state range values
    MM_SET_TYPE_LIGHT_HSL_RANGE             = 0,
    /// Set Light Lightness and Light HSL Hue and Light HSL Saturation default state values
    MM_SET_TYPE_LIGHT_HSL_DFLT,

    /// Last option value
    MM_SET_TYPE_LIGHT_HSL_MAX = MM_SET_TYPE_LIGHT_HSL_DFLT,
};

/// Set type values for the Light xyL Client model
enum mm_set_type_light_xyl
{
    /// Set Light xyL x and Light xyL y state range values
    MM_SET_TYPE_LIGHT_XYL_RANGE             = 0,
    /// Set Light Lightness and Light xyL x and Light xyL y state default values
    MM_SET_TYPE_LIGHT_XYL_DFLT,

    /// Last option value
    MM_SET_TYPE_LIGHT_XYL_MAX = MM_SET_TYPE_LIGHT_XYL_DFLT,
};

/// Transition type values for the Light Lightness Client model
enum mm_trans_type_light_ln
{
    /// Set Light Lightness state value
    MM_TRANS_TYPE_LIGHT_LN                  = 0,
    /// Set Light Lightness Linear state value
    MM_TRANS_TYPE_LIGHT_LN_LIN,

    /// Last option value
    MM_TRANS_TYPE_LIGHT_LN_MAX = MM_TRANS_TYPE_LIGHT_LN_LIN,
};

/// Transition type values for the Light CTL Client model
enum mm_trans_type_light_ctl
{
    /// Set Light CTL Lightness and Light CTL Temperature and Light CTL Delta UV state values
    MM_TRANS_TYPE_LIGHT_CTL                 = 0,
    /// Set Light CTL Temperature and Light CTL Delta UV state values
    MM_TRANS_TYPE_LIGHT_CTL_TEMP,

    /// Last option value
    MM_TRANS_TYPE_LIGHT_CTL_MAX = MM_TRANS_TYPE_LIGHT_CTL_TEMP,
};

/// Transition type values for the Light HSL Client model
enum mm_trans_type_light_hsl
{
    /// Set Light HSL Lightness and Light HSL Hue and Light HSL Saturation state values
    MM_TRANS_TYPE_LIGHT_HSL                 = 0,
    /// Set Light HSL Hue state value
    MM_TRANS_TYPE_LIGHT_HSL_HUE,
    /// Set Light HSL Saturation state value
    MM_TRANS_TYPE_LIGHT_HSL_SAT,

    /// Last option value
    MM_TRANS_TYPE_LIGHT_HSL_MAX = MM_TRANS_TYPE_LIGHT_HSL_SAT,
};

/// @} MM_DEFINES

#endif /* MM_LIGHT_H_ */
