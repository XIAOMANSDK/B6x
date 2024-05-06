/**
 ****************************************************************************************
 *
 * @file mm_genc.h
 *
 * @brief Header file for Mesh Model Generic Client Module
 *
 ****************************************************************************************
 */

#ifndef _MM_GENC_H_
#define _MM_GENC_H_

/**
 ****************************************************************************************
 * @defgroup MM_GENC Mesh Model Generic Client Module
 * @ingroup MESH_MDL
 * @brief Mesh Model Generic Client Module
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "mesh_def.h"
#include "mm_gen.h"


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Register Generic OnOff Client model. Model is registered on first element.
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_genc_oo_register(void);

/**
 ****************************************************************************************
 * @brief Register Generic Level Client model. Model is registered on first element.
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_genc_lvl_register(void);

/**
 ****************************************************************************************
 * @brief Register Generic Default Transition Time Client model. Model is registered on first element.
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_genc_dtt_register(void);

/**
 ****************************************************************************************
 * @brief Register Generic Power OnOff Client model. Model is registered on first element.
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_genc_poo_register(void);

/**
 ****************************************************************************************
 * @brief Register Generic Power Loevel Client model. Model is registered on first element.
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_genc_plvl_register(void);

/**
 ****************************************************************************************
 * @brief Register Generic Battery Client model. Model is registered on first element.
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_genc_bat_register(void);

/**
 ****************************************************************************************
 * @brief Register Generic Location Client model. Model is registered on first element.
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_genc_loc_register(void);

/**
 ****************************************************************************************
 * @brief Set global part of Generic Location state of a given node's element.
 *
 * @param[in] mdl_lid       Local index for the client model used to set the needed state value
 * @param[in] dst           Address of node's element to which message will be sent
 * @param[in] set_info      Set information
 * @param[in] latitude      Global Latitude
 * @param[in] longitude     Global Longitude
 * @param[in] altitude      Global Altitude
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_genc_locg_set(m_lid_t mdl_lid, m_lid_t app_key_lid, uint16_t dst,
                                uint8_t set_info, int32_t latitude, int32_t longitude,
                                int16_t altitude);

/**
 ****************************************************************************************
 * @brief Set local part of Generic Location state of a given node's element.
 *
 * @param[in] mdl_lid       Local index for the client model used to set the needed state value
 * @param[in] dst           Address of node's element to which message will be sent
 * @param[in] set_info      Set information
 * @param[in] north         Local North
 * @param[in] east          Local East
 * @param[in] altitude      Local Altitude
 * @param[in] floor         Floor Number
 * @param[in] uncertainty   Uncertainty
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_genc_locl_set(m_lid_t mdl_lid, m_lid_t app_key_lid, uint16_t dst,
                               uint8_t set_info, int16_t north, int16_t east, int16_t altitude,
                               uint8_t floor, uint16_t uncertainty);

/**
 ****************************************************************************************
 * @brief Register Generic Property Client model. Model is registered on first element.
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_genc_prop_register(void);

/**
 ****************************************************************************************
 * @brief Request to send a Generic User/Admin/Manufacturer/Client Property(ies) Get message
 * to an element
 *
 * @param[in] mdl_lid       Model Local Index of Generic Property Client model
 * @param[in] dst           Address of element to which the message must be sent
 * @param[in] prop_id       Property ID
 *
 * @return An handling status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_genc_prop_get(m_lid_t mdl_lid, m_lid_t app_key_lid, uint16_t dst,
                          uint8_t get_type, uint16_t prop_id);

/**
 ****************************************************************************************
 * @brief Request to send a Generic User/Admin/Manufacturer Property Set message to an
 * element
 *
 * @param[in] mdl_lid       Model local Index of Generic Property Client model
 * @param[in] dst           Address of element to which the message must be sent
 * @param[in] set_info      Set information
 * @param[in] prop_id       Property ID
 * @param[in] user_access   User access
 * @param[in] length        Property value length
 * @param[in] p_val         Pointer to the property value
 *
 * @return An handling status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_genc_prop_set(m_lid_t mdl_lid, m_lid_t app_key_lid, uint16_t dst, uint8_t set_info,
                          uint16_t prop_id, uint8_t user_access, uint16_t length, const uint8_t *p_val);


/// @} end of group

#endif // _MM_GENC_
