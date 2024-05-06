/**
 ****************************************************************************************
 *
 * @file mesh_err.h
 *
 * @brief Header file for Mesh Error Defines
 *
 ****************************************************************************************
 */

#ifndef _MESH_ERR_H_
#define _MESH_ERR_H_

/**
 ****************************************************************************************
 * @defgroup MESH_DEFINES Mesh Defines
 * @ingroup MESH
 * @brief  Mesh Defines
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDES FILES
 ****************************************************************************************
 */


/*
 * DEFINES
 ****************************************************************************************
 */

/// Mesh Error Protocol Group Code - bit[7:5]
#define MESH_ERR_PROTOCOL_CODE          (0x01)
/// Mesh Error Provisioning Group Code
#define MESH_ERR_PROVISIONING_CODE      (0x02)
/// Mesh Error Internal Group Code
#define MESH_ERR_INTERNAL_CODE          (0x03)
/// Mesh Error Low Power Node Group Code
#define MESH_ERR_LPN_CODE               (0x04)
/// Mesh Error Model Group Code
#define MESH_ERR_MDL_CODE               (0x05)

/*
 * MACROS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Macros returning the mesh error code for a given mesh sub-error code.
 *
 * @param[in] grp        Mesh group code
 * @param[in] suberror   Mesh sub-error code
 *
 * @return Mesh error code - bit[7:5] grp code, bit[4:0] sub code
 ****************************************************************************************
 */
#define MESH_ERR_(grp, suberror)        (((MESH_ERR_##grp##_CODE) << 5) | (suberror & 0x1F))

/**
 ****************************************************************************************
 * @brief Macros returning the mesh sub-error code for a given mesh error code.
 *
 * @param[in] error   Mesh error code
 *
 * @return Mesh sub-error code
 ****************************************************************************************
 */
#define MESH_SUBERR(error)              ((error) & 0x1F)

/**
 ****************************************************************************************
 * @brief Macros returning the mesh group code for a given mesh error code.
 *
 * @param[in] error   Mesh error code
 *
 * @return Mesh group error code
 ****************************************************************************************
 */
#define MESH_ERR_GRP(error)             ((error) >> 5)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Mesh Error Codes
enum mesh_error
{
    /// No Error
    MESH_ERR_NO_ERROR                   = 0x0000,

    /* **************************************************************** */
    /* *                     PROTOCOL ERROR CODES                     * */
    /* **************************************************************** */

    /// Invalid Address
    MESH_ERR_INVALID_ADDR               = MESH_ERR_(PROTOCOL, 0x01),
    /// Invalid_Model
    MESH_ERR_INVALID_MODEL              = MESH_ERR_(PROTOCOL, 0x02),
    /// Invalid AppKey Index
    MESH_ERR_INVALID_APPKEY_ID          = MESH_ERR_(PROTOCOL, 0x03),
    /// Invalid NetKey Index
    MESH_ERR_INVALID_NETKEY_ID          = MESH_ERR_(PROTOCOL, 0x04),
    /// Insufficient_Resources
    MESH_ERR_INSUFFICIENT_RESOURCES     = MESH_ERR_(PROTOCOL, 0x05),
    /// Key Index Already Stored
    MESH_ERR_KEY_ID_ALREADY_STORED      = MESH_ERR_(PROTOCOL, 0x06),
    /// Invalid Publish Parameters
    MESH_ERR_INVALID_PUBLISH_PARAMS     = MESH_ERR_(PROTOCOL, 0x07),
    /// Not a Subscribe Model
    MESH_ERR_NOT_A_SUBSCRIBE_MODEL      = MESH_ERR_(PROTOCOL, 0x08),
    /// Storage Failure
    MESH_ERR_STORAGE_FAILURE            = MESH_ERR_(PROTOCOL, 0x09),
    /// Feature Not Supported
    MESH_ERR_NOT_SUPPORTED              = MESH_ERR_(PROTOCOL, 0x0A),
    /// Cannot Update
    MESH_ERR_CANNOT_UPDATE              = MESH_ERR_(PROTOCOL, 0x0B),
    /// Cannot Remove
    MESH_ERR_CANNOT_REMOVE              = MESH_ERR_(PROTOCOL, 0x0C),
    /// Cannot Bind
    MESH_ERR_CANNOT_BIND                = MESH_ERR_(PROTOCOL, 0x0D),
    /// Temporarily Unable to Change State
    MESH_ERR_UNABLE_TO_CHANGE_STATE     = MESH_ERR_(PROTOCOL, 0x0E),
    /// Cannot Set
    MESH_ERR_CANNOT_SET                 = MESH_ERR_(PROTOCOL, 0x0F),
    /// Unspecified Error
    MESH_ERR_UNSPECIFIED_ERROR          = MESH_ERR_(PROTOCOL, 0x10),
    /// Invalid Binding
    MESH_ERR_INVALID_BINDING            = MESH_ERR_(PROTOCOL, 0x11),

    /* **************************************************************** */
    /* *                     PROVISIONING ERROR CODES                 * */
    /* **************************************************************** */

    /// Prohibited
    MESH_ERR_PROV_PROHIBITED            = MESH_ERR_(PROVISIONING, 0x00),
    /// The provisioning protocol PDU is not recognized by the device
    MESH_ERR_PROV_INVALID_PDU           = MESH_ERR_(PROVISIONING, 0x01),
    /// The arguments of the protocol PDUs are outside expected values or the length of the PDU is
    /// different than expected
    MESH_ERR_PROV_INVALID_FORMAT        = MESH_ERR_(PROVISIONING, 0x02),
    /// The PDU received was not expected at this moment of the procedure
    MESH_ERR_PROV_UNEXPECTED_PDU        = MESH_ERR_(PROVISIONING, 0x03),
    /// The computed confirmation value was not successfully verified
    MESH_ERR_PROV_CONFIRMATION_FAILED   = MESH_ERR_(PROVISIONING, 0x04),
    /// The provisioning protocol cannot be continued due to insufficient resources in the device
    MESH_ERR_PROV_OUT_OF_RESOURCES      = MESH_ERR_(PROVISIONING, 0x05),
    /// The Data block was not successfully decrypted
    MESH_ERR_PROV_DECRYPTION_FAILED     = MESH_ERR_(PROVISIONING, 0x06),
    /// An unexpected error occurred that may not be recoverable
    MESH_ERR_PROV_UNEXPECTED            = MESH_ERR_(PROVISIONING, 0x07),
    /// The device cannot assign consecutive unicast addresses to all elements
    MESH_ERR_PROV_CANNOT_ASSIGN_ADDR    = MESH_ERR_(PROVISIONING, 0x08),

    /* **************************************************************** */
    /* *                     INTERNAL ERROR CODES                     * */
    /* **************************************************************** */
    /// Invalid Parameter
    MESH_ERR_INVALID_PARAM              = MESH_ERR_(INTERNAL, 0x01),
    /// Command Disallowed
    MESH_ERR_COMMAND_DISALLOWED         = MESH_ERR_(INTERNAL, 0x02),
    /// MIC Error
    MESH_ERR_MIC_ERROR                  = MESH_ERR_(INTERNAL, 0x03),
    /// Resource requested is busy
    MESH_ERR_BUSY                       = MESH_ERR_(INTERNAL, 0x04),
    /// Request time value is past
    MESH_ERR_TIME_PAST                  = MESH_ERR_(INTERNAL, 0x05),
    /// Resource requested not found
    MESH_ERR_NOT_FOUND                  = MESH_ERR_(INTERNAL, 0x06),
    /// Sequence number error
    MESH_ERR_SEQ_ERROR                  = MESH_ERR_(INTERNAL, 0x07),
    /// Bearer instance has been closed
    MESH_ERR_BEARER_CLOSED              = MESH_ERR_(INTERNAL, 0x08),
    /// Provisioning Failed
    MESH_ERR_PROVISIONING_FAILED        = MESH_ERR_(INTERNAL, 0x09),
    /// Provisioning timeout - Transaction or Link timeout
    MESH_ERR_PROVISIONING_TIMEOUT       = MESH_ERR_(INTERNAL, 0x0A),
    /// Failed to access ECDH - Critical error
    MESH_ERR_ECDH_FAILED                = MESH_ERR_(INTERNAL, 0x0B),
    /// Request has no effect
    MESH_ERR_NO_EFFECT                  = MESH_ERR_(INTERNAL, 0x0C),
    /// Cannot fragment message due to lack of ressources
    MESH_ERR_CANNOT_FRAGMENT            = MESH_ERR_(INTERNAL, 0x0D),

    /* **************************************************************** */
    /* *                  LOW POWER NODE ERROR CODES                  * */
    /* **************************************************************** */
    /// Establishment failed after several attempts
    MESH_ERR_LPN_ESTAB_FAILED           = MESH_ERR_(LPN, 0x01),
    /// Establishment failed due to failure during generation of friend keys
    MESH_ERR_LPN_ESTAB_FAILED_KEY       = MESH_ERR_(LPN, 0x02),
    /// Establishment failed because Friend Update message not received after transmission of Friend Poll
    MESH_ERR_LPN_ESTAB_FAILED_UPD       = MESH_ERR_(LPN, 0x03),
    /// Friendship stopped due to local request
    MESH_ERR_LPN_FRIEND_LOST_LOCAL      = MESH_ERR_(LPN, 0x04),
    /// Friendship lost due to request timeout
    MESH_ERR_LPN_FRIEND_LOST_TIMEOUT    = MESH_ERR_(LPN, 0x05),

    /* **************************************************************** */
    /* *                      MODEL ERROR CODES                       * */
    /* **************************************************************** */
    /// Invalid Model Configuration
    MESH_ERR_MDL_INVALID_CFG            = MESH_ERR_(MDL, 0x01),
    /// Invalid Model Identifier
    MESH_ERR_MDL_INVALID_MDL_ID         = MESH_ERR_(MDL, 0x02),
    /// Invalid Opcode
    MESH_ERR_MDL_INVALID_OPCODE         = MESH_ERR_(MDL, 0x03),
    /// Invalid model group local index
    MESH_ERR_MDL_INVALID_GROUP          = MESH_ERR_(MDL, 0x04),
    /// Unknown model group
    MESH_ERR_MDL_UNKNOWN_GROUP          = MESH_ERR_(MDL, 0x05),
    /// Model already part of the group
    MESH_ERR_MDL_MDL_ALREADY_IN_GROUP   = MESH_ERR_(MDL, 0x06),
    /// Group of model is full
    MESH_ERR_MDL_GROUP_FULL             = MESH_ERR_(MDL, 0x07),
    /// One of the models has already been registered
    MESH_ERR_MDL_ALREADY_REGISTERED     = MESH_ERR_(MDL, 0x08),
    /// Provided Model Local Index is not valid
    MESH_ERR_MDL_INVALID_MDL_LID        = MESH_ERR_(MDL, 0x09),
    /// Invalid role
    MESH_ERR_MDL_INVALID_ROLE           = MESH_ERR_(MDL, 0x0A),
    /// Command is not available for the model
    MESH_ERR_MDL_COMMAND_NOT_AVAILABLE  = MESH_ERR_(MDL, 0x0B),
};

/// @} MESH_DEFINES

#endif /* _MESH_ERR_H_ */
