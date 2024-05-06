/**
 ****************************************************************************************
 *
 * @file mm_lightc.h
 *
 * @brief Header file for Mesh Model Lighting Client Module
 *
 ****************************************************************************************
 */

#ifndef _MM_LIGHTC_H_
#define _MM_LIGHTC_H_

/**
 ****************************************************************************************
 * @defgroup MM_LIGHTC Mesh Model Lighting Client Module
 * @ingroup MESH_MDL
 * @brief Mesh Model Lighting Client Module
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
 * @brief Register Light Lightness Client model. Model is registered on first element.
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_lightc_ln_register(void);

/**
 ****************************************************************************************
 * @brief Register Light CTL Client model. Model is registered on first element.
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_lightc_ctl_register(void);

/**
 ****************************************************************************************
 * @brief Register Light HSL Client model. Model is registered on first element.
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_lightc_hsl_register(void);

/**
 ****************************************************************************************
 * @brief Register Light XYL Client model. Model is registered on first element.
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_lightc_xyl_register(void);

/// @} end of group

#endif // _MM_LIGHTC_
