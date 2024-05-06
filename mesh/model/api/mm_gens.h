/**
 ****************************************************************************************
 * @file mm_gens.h
 *
 * @brief Header file for Mesh Model Generic Server Module
 *
 ****************************************************************************************
 */

#ifndef _MM_GENS_H_
#define _MM_GENS_H_

/**
 ****************************************************************************************
 * @defgroup MM_GENS Mesh Model Generic Server Module
 * @ingroup MESH_MDL
 * @brief Mesh Model Generic Server Module
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
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Structure for Generic Property state information
typedef struct mm_prop
{
    /// Property ID
    uint16_t prop_id;
    /// User Access
    uint8_t user_access;
} mm_prop_t;


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Register Generic OnOff Server model for a given local element
 *
 * @param[in] elmt_idx      Element Index
 * @param[in] main          True if model is a main model, else false
 *
 * @return An error status (@see mesh_err)
 ****************************************************************************************
 */
m_lid_t mm_gens_oo_register(uint8_t elmt_idx, bool main);

/**
 ****************************************************************************************
 * @brief Bind a Generic OnOff Server model to a group
 *
 * @param[in] grp_lid       Group local index
 * @param[in] oo_lid        Model local index
 ****************************************************************************************
 */
uint8_t mm_gens_oo_bind_group(m_lid_t grp_lid, m_lid_t oo_lid);

/**
 ****************************************************************************************
 * @brief Register Generic Level Server model for a given local element
 *
 * @param[in] elmt_idx      Element Index
 * @param[in] p_mdl_lid     Pointer to the variable in which allocated model local index
 * will be written
 *
 * @return An error status (@see mesh_err)
 ****************************************************************************************
 */
m_lid_t mm_gens_lvl_register(uint8_t elmt_idx, bool main);

/**
 ****************************************************************************************
 * @brief Bind a Generic Level Server model to a group
 *
 * @param[in] grp_lid       Group local index
 * @param[in] lvl_lid       Model local index
 ****************************************************************************************
 */
uint8_t mm_gens_lvl_bind_group(m_lid_t grp_lid, m_lid_t lvl_lid);

/**
 ****************************************************************************************
 * @brief Register Generic Power Level Server and Generic Power Level Setup Setup models
 * for a given local element
 *
 * @param[in] elmt_idx      Element Index
 * @param[in] p_mdl_lid     Pointer to the variable in which allocated model local index
 * for the Generic Power Level Server model will be written
 *
 * @return An error status (@see mesh_err)
 ****************************************************************************************
 */
uint8_t mm_gens_plvl_register(uint8_t elmt_idx, bool main);

/**
 ****************************************************************************************
 * @brief Register Generic Default Transition Time Server model for a given local element
 *
 * @param[in] elmt_idx      Element Index
 *
 * @return An error status (@see mesh_err)
 ****************************************************************************************
 */
uint8_t mm_gens_dtt_register(uint8_t elmt_idx);

/**
 ****************************************************************************************
 * @brief Register Generic Power OnOff Server and Generic Power OnOff Setup Server model
 * for a given local element
 *
 * @param[in] elmt_idx      Element Index
 *
 * @return An error status (@see mesh_err)
 ****************************************************************************************
 */
uint8_t mm_gens_poo_register(uint8_t elmt_idx, bool main);

/**
 ****************************************************************************************
 * @brief Register Generic Battery Server model for a given element
 *
 * @param[in] elmt_idx    Element ID
 *
 * @return An error status (@see mesh_err)
 ****************************************************************************************
 */
uint8_t mm_gens_bat_register(uint8_t elmt_idx);

/**
 ****************************************************************************************
 * @brief Function called upon reception of Generic Battery state value for a local element.
 *
 * @param[in] elmt_idx          Index of element for which Battery information are provided
 * @param[in] status            Confirmation status
 * @param[in] bat_lvl           Battery Level
 * @param[in] time_charge       Time to charge in minutes
 * @param[in] time_discharge    Time to discharge in minutes
 * @param[in] flags             Flags
 ****************************************************************************************
 */
void mm_gens_bat_cfm(uint8_t elmt_idx, uint8_t status, uint8_t bat_lvl,
                     uint32_t time_discharge, uint32_t time_charge, uint8_t flags);

/**
 ****************************************************************************************
 * @brief Register Generic Location Server and Generic Location Setup Server models for
 * a given local element
 *
 * @param[in] elmt_idx      Element Index
 *
 * @return An error status (@see mesh_err)
 ****************************************************************************************
 */
uint8_t mm_gens_loc_register(uint8_t elmt_idx);

/**
 ****************************************************************************************
 * @brief Function called upon reception of global part of Generic Location state value
 * for a local element.
 *
 * @param[in] elmt_idx          Index of element for which Location information are provided
 * @param[in] status            Confirmation status
 * @param[in] latitude          Global Latitude
 * @param[in] longitude         Global Longitude
 * @param[in] altitude          Global Altitude
 ****************************************************************************************
 */
void mm_gens_locg_cfm(uint8_t elmt_idx, uint8_t status, int32_t latitude,
                            int32_t longitude, int16_t altitude);

/**
 ****************************************************************************************
 * @brief Function called upon reception of local part of Generic Location state value
 * for a local element.
 *
 * @param[in] elmt_idx          Index of element for which Location information are provided
 * @param[in] status            Confirmation status
 * @param[in] north             Local North
 * @param[in] east              Local East
 * @param[in] altitude          Local Altitude
 * @param[in] floor             Floor Number
 * @param[in] uncertainty       Uncertainty
 ****************************************************************************************
 */
void mm_gens_locl_cfm(uint8_t elmt_idx, uint8_t status, int16_t north,
                           int16_t east, int16_t altitude, uint8_t floor, uint16_t uncertainty);

/**
 ****************************************************************************************
 * @brief Register Generic Property models on an element
 *
 * @param[in] elmt_idx          Index of element on which models must be registered
 * @param[in] req_queue_len     Number of received messages that can be queued when model
 * is waiting for application confirmation
 * @param[in] nb_prop_user      Number of Generic User Properties
 * @param[in] nb_prop_admin     Number of Generic Admin Properties
 * @param[in] nb_prop_manuf     Number of Generic Manufacturer Properties
 * @param[in] nb_prop_cli       Number of Generic Client Properties
 * @param[in] p_props           Pointer to list of Generic Property information
 *
 * @return An handling error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_gens_prop_register(uint8_t elmt_idx, uint8_t req_queue_len,
                               uint8_t nb_prop_user, uint8_t nb_prop_admin,
                               uint8_t nb_prop_manuf, uint8_t nb_prop_cli, const mm_prop_t *p_props);

/**
 ****************************************************************************************
 * @brief Confirmation functions for get and set request indication received by the application
 *
 * @param[in] status        Confirmation status
 * @param[in] elmt_idx      Element index
 * @param[in] prop_type     Property Type (@see enum mm_prop_type)
 * @param[in] prop_id       Property ID
 * @param[in] length        Property Length
 * @param[in] p_val         Pointer to the property value
 ****************************************************************************************
 */
void mm_gens_prop_cfm(uint8_t elmt_idx, uint8_t status, uint8_t prop_type, uint16_t prop_id,
                      uint16_t length, const uint8_t *p_val);

/// @} end of group

#endif // _MM_GENS_
