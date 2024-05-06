/**
 ****************************************************************************************
 *
 * @file genie_mesh.h
 *
 * @brief Tmall Genie mesh
 *
 ****************************************************************************************
 */

#ifndef __GENIE_MESH_H__
#define __GENIE_MESH_H__

#include <stdint.h>


/*
 * DEFINITIONS
 ****************************************************************************************
 */

/// Genie's IDs
#define GENIE_MESH_CID                      (0x01A8) // taobao.inc

/// Triple Tuple size
#define TRI_TUPLE_SIZE                      (28) // 4+16+6+2
#define TRI_TUPLE_VAL_SIZE                  (26) // 4+16+6
#define TRI_TUPLE_PID_SIZE                  (4)
#define TRI_TUPLE_KEY_SIZE                  (16)
#define TRI_TUPLE_MAC_SIZE                  (6)
#define TRI_TUPLE_CRC_SIZE                  (2)

/// Size of Device UUID
#define MESH_DEV_UUID_LEN                   (16)

/// Unprovison Feature Flag
//fixed broadcast
#define UNPROV_ADV_FEAT0_BROADCAST          0x00
//sig mesh profile, This version is deprecated
#define UNPROV_ADV_FEAT0_UUID_VER0          0x00
//Optimized the distribution network process and needs to support Vendor Model
#define UNPROV_ADV_FEAT0_UUID_VER1          0x02
//This version adds some new Mesh features, see the definition of FeatureFlag2 flag bit for details
#define UNPROV_ADV_FEAT0_UUID_VER2          0x04
//Tiny Mesh flag
#define UNPROV_ADV_FEAT1_TINY_MESH_FLAG     0x01
//expand Auth value flag
#define UNPROV_ADV_FEAT1_EXPAND_AUTH_FLAG   0x02
//BLE Advertising pair flag
#define UNPROV_ADV_FEAT1_BLE_ADV_PAIR_FLAG  0x04
//RFU flag
#define UNPROV_ADV_FEAT1_RFU_FLAG           0x08
//Genie BT Mesh Stack flag bit7~4
#define UNPROV_ADV_FEAT1_GENIE_MESH_STACK   0x00


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Storage Index
enum genie_store_index
{
    GENIE_STORE_INDEX_START     = 0,
    GFI_MESH_PARA,
    GFI_MESH_TRITUPLE,
    GFI_MESH_SADDR,
    GFI_MESH_SUB,
    GFI_MESH_SEQ,
    GFI_MESH_RESET_CNT,
    GFI_MESH_POWERUP,
    GFI_MESH_HB,
    GFI_MESH_DEVKEY,
    GFI_MESH_NETKEY,
    GFI_MESH_APPKEY,
    GFI_OTA_INDICAT,
    GFI_OTA_IMAGE_ID,
    GFI_USERDATA_START,
    GFI_MESH_CTRL_RELAY,
    GFI_MESH_APPKEY1,
    GFI_MESH_SCENE_DATA,
    GFI_MESH_PANEL_DATA,
};

/// Storage Status
enum genie_store_status
{
    GENIE_STORE_SUCCESS         = 0,
    GENIE_STORE_INIT_FAIL,
    GENIE_STORE_MALLOC_FAIL,
    GENIE_STORE_EARSE_FAIL,
    GENIE_STORE_DATA_INVALID,
    GENIE_STORE_ERASE_FAIL,
    GENIE_STORE_READ_FAIL,
    GENIE_STORE_WRITE_FAIL,
    GENIE_STORE_DELETE_FAIL,
    GENIE_STORE_INVALID_INDEX,
    GENIE_STORE_SIZE_ERROR,
    GENIE_STORE_CHECK_CRC_FAIL
};


/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Triple Tuple struct
typedef struct tri_tuple
{
    uint32_t pid;
    uint8_t  key[TRI_TUPLE_KEY_SIZE];
    uint8_t  mac[TRI_TUPLE_MAC_SIZE];
    uint16_t crc;
} tri_tuple_t;


/*
* FUNCTIONS DECLARATION
****************************************************************************************
*/

/**
 * @brief generate the trituple info formatted in uuid
 * @return the pointer refering to uuid
 */
void genie_gen_uuid(uint8_t *p_uuid, const tri_tuple_t *p_triple);

/**
 * @brief get the authentication info
 * @param[in] random: the random sequence used for calculating.
 * @return the authenticated sequence
 */
void genie_calc_auth(uint8_t *p_auth, const tri_tuple_t *p_triple);

void genie_calc_auth_with_rand(uint8_t *p_auth, const tri_tuple_t *p_triple, const uint8_t *p_rand);


#endif //__GENIE_MESH_H__
