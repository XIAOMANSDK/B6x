/**
 ****************************************************************************************
 *
 * @file mm_lights.h
 *
 * @brief Header file for Mesh Model Lighting Server Module
 *
 ****************************************************************************************
 */

#ifndef _MM_LIGHTS_H_
#define _MM_LIGHTS_H_

/**
 ****************************************************************************************
 * @defgroup MM_LIGHTS Mesh Model Lighting Server Module
 * @ingroup MESH_MDL
 * @brief Mesh Model Lighting Server Module
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "mesh_def.h"
#include "mm_light.h"


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Register Light Lightness Server and Light Lightness Setup Server models
 * needed for management of Light Lightness state. Also register the Generic Power
 * OnOff Server and the Generic Level Server models and the Generic OnOff Server model.
 *
 * @param[in] elmt_idx      Index of element on which required models must be registered
 * @param[in] main          True if Generic Power Level model is a main model, else false
 *
 * @return An error status
 ****************************************************************************************
 */
uint8_t mm_lights_ln_register(uint8_t elmt_idx, bool main);

/**
 ****************************************************************************************
 * @brief Bind a Light Lightness Server model to a group
 *
 * @param[in] grp_lid       Group local index
 * @param[in] oo_lid        Model local index
 ****************************************************************************************
 */
uint8_t mm_lights_ln_bind_group(m_lid_t grp_lid, m_lid_t ln_lid);

/**
 ****************************************************************************************
 * @brief Get Light Lightness Range or Light Lightness Default value of an element
 *
 * @param[in] elmt_idx      Element index
 * @param[in] state_id      State Identifier
 ****************************************************************************************
 */
uint16_t mm_lights_ln_get(uint8_t elmt_idx, uint32_t state_id);

/**
 ****************************************************************************************
 * @brief Set Light Lightness Default value of an element
 *
 * @param[in] elmt_idx      Element index
 * @param[in] ln_dflt       Light Lightness Default value
 ****************************************************************************************
 */
void mm_lights_ln_set_dflt(uint8_t elmt_idx, uint16_t ln_dflt);

/**
 ****************************************************************************************
 * @brief Register Light CTL Server, Light CTL Setup Server, Light Lightness Temperature
 * Server models and associated models
 *
 * @param[in] elmt_idx      Index of element on which required models must be registered
 * @param[in] main          True if Generic Power Level model is a main model, else false
 *
 * @return An error status
 ****************************************************************************************
 */
uint8_t mm_lights_ctl_register(uint8_t elmt_idx, bool main);

/**
 ****************************************************************************************
 * @brief Register Light HSL Server, Light HSL Setup Server, Light Lightness Hue Server,
 * Light Lightness Saturation Server models and associated models
 *
 * @param[in] elmt_idx      Index of element on which required models must be registered
 * @param[in] main          True if Generic Power Level model is a main model, else false
 *
 * @return An error status
 ****************************************************************************************
 */
uint8_t mm_lights_hsl_register(uint8_t elmt_idx, bool main);


/// @} end of group

#endif // _MM_LIGHTS_
