/**
 ****************************************************************************************
 *
 * @file mesh_cfg.h
 *
 * @brief Header file for Mesh Stack Configuration
 *
 ****************************************************************************************
 */

#ifndef MESH_CFG_H_
#define MESH_CFG_H_

/*
 * LIB FLAGS
 ****************************************************************************************
 */

#if (MESH_LIB_FULL)
/// Support of GATT Proxy feature
#define BLE_MESH_GATT_PROXY             (1)

/// Support of GATT Provisioning feature
#define BLE_MESH_GATT_PROV              (1)

/// Support of Relay feature
#define BLE_MESH_RELAY                  (1)

/// Support of Friend feature
#define BLE_MESH_FRIEND                 (1)

/// Support of Low Power Node feature
#define BLE_MESH_LPN                    (1)

#else //(MESH_LIB_LITE)
/// Support of GATT Proxy feature
#define BLE_MESH_GATT_PROXY             (1)

/// Support of GATT Provisioning feature
#define BLE_MESH_GATT_PROV              (1)

/// Support of Relay feature
#define BLE_MESH_RELAY                  (0)

/// Support of Friend feature
#define BLE_MESH_FRIEND                 (0)

/// Support of Low Power Node feature
#define BLE_MESH_LPN                    (0)

#endif //(MESH_LIB_FULL)


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Mesh Supported Features
enum mesh_feat
{
    /// Relay Node
    MESH_FEAT_RELAY_NODE_SUP            = (1 << 0),
    /// Proxy Node
    MESH_FEAT_PROXY_NODE_SUP            = (1 << 1),
    /// Friend Node
    MESH_FEAT_FRIEND_NODE_SUP           = (1 << 2),
    /// Low Power Node
    MESH_FEAT_LOW_POWER_NODE_SUP        = (1 << 3),

    /// Provisioning over GATT
    MESH_FEAT_PB_GATT_SUP               = (1 << 14),
    /// Dynamic beacon interval supported
    MESH_FEAT_DYN_BCN_INTV_SUP          = (1 << 15),
};

/// Used to know if GATT Bearer is present
#define BLE_MESH_GATT_BEARER            (BLE_MESH_GATT_PROXY || BLE_MESH_GATT_PROV)

/// Supported feature mask
#define BLE_MESH_FEAT_MASK      (  (BLE_MESH_RELAY      * MESH_FEAT_RELAY_NODE_SUP     )  \
                                 | (BLE_MESH_GATT_PROXY * MESH_FEAT_PROXY_NODE_SUP     )  \
                                 | (BLE_MESH_FRIEND     * MESH_FEAT_FRIEND_NODE_SUP    )  \
                                 | (BLE_MESH_LPN        * MESH_FEAT_LOW_POWER_NODE_SUP )  \
                                 | (BLE_MESH_GATT_PROV  * MESH_FEAT_PB_GATT_SUP        )  )


#endif /* MESH_CFG_H_ */
