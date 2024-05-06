/**
 ****************************************************************************************
 *
 * @file genie_mesh.c
 *
 * @brief Tmall Genie mesh
 *
 ****************************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include "sha256.h"
#include "genie_mesh.h"


/*
 * DEFINITIONS
 ****************************************************************************************
 */

// pid + ',' + mac + ',' + secret = 8+1+12+1+32 = 54
#define GENIE_OOB_STR_LEN          (54)
// pid + ',' + mac + ',' + secret + ',' + rand = 8+1+12+1+32+1+32 = 87
#define GENIE_OOB_RND_LEN          (87)


/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Swap bytes of an array of bytes
 * .
 * The swap is done in every case. Should not be called directly.
 *
 * @param[in] dst    The output value.
 * @param[in] src    The input value.
 * @param[in] len    number of bytes to swap
 ****************************************************************************************
 */
static __inline void co_bswap(uint8_t *dst, const uint8_t *src, uint16_t len)
{
    while (len--)
    {
        *dst++ = src[len];
    }
}

/**
 ****************************************************************************************
 * @brief Function to return the hex char of a 4bit.
 * @return The hex char.
 ****************************************************************************************
 */
static __inline uint8_t co_hex(uint8_t b4)
{
    return b4 < 10 ? b4 + '0' : b4 - 10 + 'a';
}

static void hex2str(uint8_t *dest, const uint8_t *src, uint16_t len)
{
    while (len--)
    {
        *dest++ = co_hex(*src >> 4);
        *dest++ = co_hex(*src & 0x0F);
        src++;
    }
}


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * @brief generate the trituple info formatted in uuid
 * @return the pointer refering to uuid
 */
void genie_gen_uuid(uint8_t *p_uuid, const tri_tuple_t *p_triple)
{
    // all fields in uuid should be in little-endian

    // B[0:1] - CID, Taobao
    *p_uuid++ = (GENIE_MESH_CID >> 0) & 0xFF;
    *p_uuid++ = (GENIE_MESH_CID >> 8) & 0xFF;

    // B[2] - Flags Bit0~Bit3: 0001 (broadcast version) Bit4: (one secret pre device)
    //              Bit5: 1 (OTA support) Bit6~Bit7: 01 (00:4.0 01:4.2 10:5.0 11:>5.0)
    *p_uuid++ = 0x71;

    // B[3:6] - Product ID
    *p_uuid++ = (uint8_t)( p_triple->pid & 0xff);
    *p_uuid++ = (uint8_t)((p_triple->pid & 0xff00) >> 8);
    *p_uuid++ = (uint8_t)((p_triple->pid & 0xff0000) >> 16);
    *p_uuid++ = (uint8_t)((p_triple->pid & 0xff000000) >> 24);

    // B[7:12] - MAC addr (device name)
    for (uint8_t i = 0; i < 6; i++)
    {
        *p_uuid++ = p_triple->mac[TRI_TUPLE_MAC_SIZE - 1 - i];
    }

    // B[13:15] - Feature
    *p_uuid++ = UNPROV_ADV_FEAT0_BROADCAST | UNPROV_ADV_FEAT0_UUID_VER0;
    *p_uuid++ = 0;
    *p_uuid   = 0;
}

/**
 * @brief get the authentication info
 * @param[in] random: the random sequence used for calculating.
 * @return the authenticated sequence
 */
void genie_calc_auth(uint8_t *p_auth, const tri_tuple_t *p_triple)
{
    struct tc_sha256_state_struct sha256_ctx;
    uint8_t tmp_auth[TC_SHA256_DIGEST_SIZE];

    // Compose oob information
    uint8_t oob_str[GENIE_OOB_STR_LEN];

    hex2str(&oob_str[0],  (uint8_t *)&p_triple->pid, 4);
    oob_str[8]  = ',';
    hex2str(&oob_str[9],  (uint8_t *)&p_triple->mac, 6);
    oob_str[21] = ',';
    hex2str(&oob_str[22], (uint8_t *)&p_triple->key, 16);

    /* calculate the sha256 of oob info */
    tc_sha256_init(&sha256_ctx);
    tc_sha256_update(&sha256_ctx, oob_str, GENIE_OOB_STR_LEN);
    tc_sha256_final(tmp_auth, &sha256_ctx);

    /* fetch the top 16 bytes as static value */
    co_bswap(p_auth, tmp_auth, 16);
}

void genie_calc_auth_with_rand(uint8_t *p_auth, const tri_tuple_t *p_triple, const uint8_t *p_rand)
{
    struct tc_sha256_state_struct sha256_ctx;
    uint8_t tmp_auth[TC_SHA256_DIGEST_SIZE];

    // Compose oob information
    uint8_t oob_str[GENIE_OOB_RND_LEN];

    hex2str(&oob_str[0],  (uint8_t *)&p_triple->pid, 4);
    oob_str[8]  = ',';
    hex2str(&oob_str[9],  (uint8_t *)&p_triple->mac, 6);
    oob_str[21] = ',';
    hex2str(&oob_str[22], (uint8_t *)&p_triple->key, 16);
    oob_str[54] = ',';
    hex2str(&oob_str[55], (uint8_t *)&p_rand, 16);

    /* calculate the sha256 of oob info */
    tc_sha256_init(&sha256_ctx);
    tc_sha256_update(&sha256_ctx, oob_str, GENIE_OOB_RND_LEN);
    tc_sha256_final(tmp_auth, &sha256_ctx);

    /* fetch the top 16 bytes as static value */
    co_bswap(p_auth, tmp_auth, 16);
}

