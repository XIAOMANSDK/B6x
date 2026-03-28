/* gsm.c - GSM 6.10 Decoder wrapper implementation */
#include "gsm.h"

/* Include GSM library headers for access to internal functions */
#include "gsm-1.0-pl23/inc/proto.h"
#include "gsm-1.0-pl23/inc/private.h"

#include <string.h>
#include <stdlib.h>

/* Gsm_Decoder is declared in private.h, no need to redeclare it here */

/**
 * @brief Unpack bits from 33-byte GSM frame into parameters
 */
void gsm_unpack_bits(const uint8_t *c, int16_t *LARc, int16_t *Nc,
                     int16_t *bc, int16_t *Mc, int16_t *xmaxc, int16_t *xmc)
{
    /* Check GSM magic number */
    if (((*c >> 4) & 0x0F) != GSM_MAGIC) {
        return; /* Invalid frame */
    }

    /* Unpack LAR parameters */
    LARc[0] = (*c++ & 0xF) << 2;        /* 1 */
    LARc[0] |= (*c >> 6) & 0x3;
    LARc[1] = *c++ & 0x3F;
    LARc[2] = (*c >> 3) & 0x1F;
    LARc[3] = (*c++ & 0x7) << 2;
    LARc[3] |= (*c >> 6) & 0x3;
    LARc[4] = (*c >> 2) & 0xF;
    LARc[5] = (*c++ & 0x3) << 2;
    LARc[5] |= (*c >> 6) & 0x3;
    LARc[6] = (*c >> 3) & 0x7;
    LARc[7] = *c++ & 0x7;

    /* Unpack subframe 0 parameters */
    Nc[0] = (*c >> 1) & 0x7F;
    bc[0] = (*c++ & 0x1) << 1;
    bc[0] |= (*c >> 7) & 0x1;
    Mc[0] = (*c >> 5) & 0x3;
    xmaxc[0] = (*c++ & 0x1F) << 1;
    xmaxc[0] |= (*c >> 7) & 0x1;

    /* Unpack xmc for subframe 0 */
    xmc[0] = (*c >> 4) & 0x7;
    xmc[1] = (*c >> 1) & 0x7;
    xmc[2] = (*c++ & 0x1) << 2;
    xmc[2] |= (*c >> 6) & 0x3;
    xmc[3] = (*c >> 3) & 0x7;
    xmc[4] = *c++ & 0x7;
    xmc[5] = (*c >> 5) & 0x7;
    xmc[6] = (*c >> 2) & 0x7;
    xmc[7] = (*c++ & 0x3) << 1;        /* 10 */
    xmc[7] |= (*c >> 7) & 0x1;
    xmc[8] = (*c >> 4) & 0x7;
    xmc[9] = (*c >> 1) & 0x7;
    xmc[10] = (*c++ & 0x1) << 2;
    xmc[10] |= (*c >> 6) & 0x3;
    xmc[11] = (*c >> 3) & 0x7;
    xmc[12] = *c++ & 0x7;

    /* Unpack subframe 1 parameters */
    Nc[1] = (*c >> 1) & 0x7F;
    bc[1] = (*c++ & 0x1) << 1;
    bc[1] |= (*c >> 7) & 0x1;
    Mc[1] = (*c >> 5) & 0x3;
    xmaxc[1] = (*c++ & 0x1F) << 1;
    xmaxc[1] |= (*c >> 7) & 0x1;

    /* Unpack xmc for subframe 1 */
    xmc[13] = (*c >> 4) & 0x7;
    xmc[14] = (*c >> 1) & 0x7;
    xmc[15] = (*c++ & 0x1) << 2;
    xmc[15] |= (*c >> 6) & 0x3;
    xmc[16] = (*c >> 3) & 0x7;
    xmc[17] = *c++ & 0x7;
    xmc[18] = (*c >> 5) & 0x7;
    xmc[19] = (*c >> 2) & 0x7;
    xmc[20] = (*c++ & 0x3) << 1;
    xmc[20] |= (*c >> 7) & 0x1;
    xmc[21] = (*c >> 4) & 0x7;
    xmc[22] = (*c >> 1) & 0x7;
    xmc[23] = (*c++ & 0x1) << 2;
    xmc[23] |= (*c >> 6) & 0x3;
    xmc[24] = (*c >> 3) & 0x7;
    xmc[25] = *c++ & 0x7;

    /* Unpack subframe 2 parameters */
    Nc[2] = (*c >> 1) & 0x7F;          /* 20 */
    bc[2] = (*c++ & 0x1) << 1;
    bc[2] |= (*c >> 7) & 0x1;
    Mc[2] = (*c >> 5) & 0x3;
    xmaxc[2] = (*c++ & 0x1F) << 1;
    xmaxc[2] |= (*c >> 7) & 0x1;

    /* Unpack xmc for subframe 2 */
    xmc[26] = (*c >> 4) & 0x7;
    xmc[27] = (*c >> 1) & 0x7;
    xmc[28] = (*c++ & 0x1) << 2;
    xmc[28] |= (*c >> 6) & 0x3;
    xmc[29] = (*c >> 3) & 0x7;
    xmc[30] = *c++ & 0x7;
    xmc[31] = (*c >> 5) & 0x7;
    xmc[32] = (*c >> 2) & 0x7;
    xmc[33] = (*c++ & 0x3) << 1;
    xmc[33] |= (*c >> 7) & 0x1;
    xmc[34] = (*c >> 4) & 0x7;
    xmc[35] = (*c >> 1) & 0x7;
    xmc[36] = (*c++ & 0x1) << 2;
    xmc[36] |= (*c >> 6) & 0x3;
    xmc[37] = (*c >> 3) & 0x7;
    xmc[38] = *c++ & 0x7;

    /* Unpack subframe 3 parameters */
    Nc[3] = (*c >> 1) & 0x7F;
    bc[3] = (*c++ & 0x1) << 1;
    bc[3] |= (*c >> 7) & 0x1;
    Mc[3] = (*c >> 5) & 0x3;
    xmaxc[3] = (*c++ & 0x1F) << 1;
    xmaxc[3] |= (*c >> 7) & 0x1;

    /* Unpack xmc for subframe 3 */
    xmc[39] = (*c >> 4) & 0x7;
    xmc[40] = (*c >> 1) & 0x7;
    xmc[41] = (*c++ & 0x1) << 2;
    xmc[41] |= (*c >> 6) & 0x3;
    xmc[42] = (*c >> 3) & 0x7;
    xmc[43] = *c++ & 0x7;              /* 30 */
    xmc[44] = (*c >> 5) & 0x7;
    xmc[45] = (*c >> 2) & 0x7;
    xmc[46] = (*c++ & 0x3) << 1;
    xmc[46] |= (*c >> 7) & 0x1;
    xmc[47] = (*c >> 4) & 0x7;
    xmc[48] = (*c >> 1) & 0x7;
    xmc[49] = (*c++ & 0x1) << 2;
    xmc[49] |= (*c >> 6) & 0x3;
    xmc[50] = (*c >> 3) & 0x7;
    xmc[51] = *c & 0x7;                /* 33 */
}

/**
 * @brief Convert LAR coefficients to reflection coefficients
 * This is a simplified version - the actual implementation is in short_term.c
 */
void gsm_lar_to_coefficients(gsm_state_t *S, const int16_t *LARc, int16_t *rp)
{
    /* The GSM library's Gsm_Decoder will handle LAR conversion internally */
    /* This is just a placeholder for compatibility */
    (void)S;
    (void)LARc;
    (void)rp;
}

/**
 * @brief RPE decoding - placeholder, actual implementation in GSM library
 */
void gsm_rpe_decode(gsm_state_t *S, int16_t xmaxc, int16_t Mc,
                    const int16_t *xmc, int16_t *erp)
{
    (void)S;
    (void)xmaxc;
    (void)Mc;
    (void)xmc;
    (void)erp;
    /* Actual implementation is in Gsm_RPE_Decoding from rpe.c */
}

/**
 * @brief LTP synthesis - placeholder, actual implementation in GSM library
 */
void gsm_ltp_synthesis(gsm_state_t *S, int16_t Nc, int16_t bc, int16_t *drp)
{
    (void)S;
    (void)Nc;
    (void)bc;
    (void)drp;
    /* Actual implementation is in Gsm_Long_Term_Synthesis_Filtering from long_term.c */
}

/**
 * @brief Short term synthesis filter - placeholder, actual implementation in GSM library
 */
void gsm_short_term_synthesis_filter(gsm_state_t *S, const int16_t *rp,
                                      int16_t *s)
{
    (void)S;
    (void)rp;
    (void)s;
    /* Actual implementation is in Gsm_Short_Term_Synthesis_Filter from short_term.c */
}

/**
 * @brief Post-processing - placeholder, actual implementation in GSM library
 */
void gsm_post_processing(gsm_state_t *S, int16_t *s)
{
    (void)S;
    (void)s;
    /* Actual implementation is in Postprocessing from decode.c */
}

/**
 * @brief GSM 6.10 decode one frame
 * @param S GSM state structure pointer
 * @param c 33-byte compressed GSM frame
 * @param target 160 samples (320 bytes) PCM output
 */
void gsm_decode_frame(gsm_state_t *S, uint8_t *c, int16_t *target)
{
    int16_t LARc[8], Nc[4], Mc[4], bc[4], xmaxc[4], xmc[52];

    /* Step 1: Unpack 33 bytes into parameters */
    gsm_unpack_bits(c, LARc, Nc, bc, Mc, xmaxc, xmc);

    /* Step 2-5: Use GSM library's decoder which handles all the decoding steps */
    /* This includes RPE decoding, LTP synthesis, short-term synthesis filter,
       and post-processing */
    Gsm_Decoder((struct gsm_state *)S, LARc, Nc, bc, Mc, xmaxc, xmc, target);
}
