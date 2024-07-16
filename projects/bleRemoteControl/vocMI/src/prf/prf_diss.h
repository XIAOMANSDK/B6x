/**
 ****************************************************************************************
 *
 * @file prf_diss.h
 *
 * @brief Header file - Device Information Service Server.
 *
 ****************************************************************************************
 */

#ifndef PRF_DISS_H_
#define PRF_DISS_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Service Start Handle(0 = dynamically allocated)
#if !defined(DIS_START_HDL)
    #define DIS_START_HDL           (0)
#endif

/// Information Characteristic Support
#if !defined(DIS_FEATURES)
#define DIS_FEATURES                ( DIS_FEAT_MANUF_NAME_BIT     |\
                                      DIS_FEAT_SERIAL_NB_STR_BIT  |\
                                      DIS_FEAT_HW_REV_STR_BIT     |\
                                      DIS_FEAT_FW_REV_STR_BIT     |\
                                      DIS_FEAT_PNP_ID_BIT )
#endif

/// Maximal length for Char. values(20 bytes default)
#if !defined(DIS_VAL_MAX_LEN)
#define DIS_VAL_MAX_LEN             (20)
#endif
/// System ID string length
#define DIS_SYS_ID_LEN              (0x08)
/// IEEE Certif length (min 6 bytes)
#define DIS_IEEE_CERTIF_MIN_LEN     (0x06)
/// PnP ID length
#define DIS_PNP_ID_LEN              (0x07)


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Supported Feature Flags, Bits of Index @see enum dis_values
#define DIS_FEAT_MANUF_NAME_BIT     (0x0001) //(1 << DIS_MANUF_NAME_CHAR)
#define DIS_FEAT_MODEL_NB_STR_BIT   (0x0002) //(1 << DIS_MODEL_NB_STR_CHAR)
#define DIS_FEAT_SERIAL_NB_STR_BIT  (0x0004) //(1 << DIS_SERIAL_NB_STR_CHAR)
#define DIS_FEAT_HW_REV_STR_BIT     (0x0008) //(1 << DIS_HW_REV_STR_CHAR)
#define DIS_FEAT_FW_REV_STR_BIT     (0x0010) //(1 << DIS_FW_REV_STR_CHAR)
#define DIS_FEAT_SW_REV_STR_BIT     (0x0020) //(1 << DIS_SW_REV_STR_CHAR)
#define DIS_FEAT_SYS_ID_BIT         (0x0040) //(1 << DIS_SYS_ID_CHAR)
#define DIS_FEAT_IEEE_BIT           (0x0080) //(1 << DIS_IEEE_CHAR)
#define DIS_FEAT_PNP_ID_BIT         (0x0100) //(1 << DIS_PNP_ID_CHAR)
#define DIS_FEAT_ALL_SUP            (0x01FF) // All features are supported

#define DIS_FEAT_SUP(val)           (DIS_FEATURES & DIS_FEAT_##val##_BIT)

/// Device Value Indexes
enum dis_values
{
    /// Manufacturer Name String
    DIS_MANUF_NAME_CHAR,
    /// Model Number String
    DIS_MODEL_NB_STR_CHAR,
    /// Serial Number String
    DIS_SERIAL_NB_STR_CHAR,
    /// Hardware Revision String
    DIS_HW_REV_STR_CHAR,
    /// Firmware Revision String
    DIS_FW_REV_STR_CHAR,
    /// Software Revision String
    DIS_SW_REV_STR_CHAR,
    /// System Identifier
    DIS_SYS_ID_CHAR,
    /// IEEE Certificate
    DIS_IEEE_CHAR,
    /// Plug and Play Identifier
    DIS_PNP_ID_CHAR,

    DIS_CHAR_MAX,
};


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Add Device Information Profile in the DB.
 *        Customize via pre-define @see DIS_START_HDL @see DIS_FEATURES
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t diss_svc_init(void);

/**
 ****************************************************************************************
 * @brief Get value for attribute read (__weak func)
 *
 * @param[in] val_idx  Index of Value to set @see enum dis_values
 * @param[out] p_len   Value Length
 *
 * @return Value data pointer
 ****************************************************************************************
 */
const uint8_t *diss_value_get(uint8_t val_idx, uint16_t *p_len);


#endif /* PRF_DISS_H_ */
