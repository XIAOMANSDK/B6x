/**
 ****************************************************************************************
 *
 * @file gsm.c
 *
 * @brief GSM 6.10 Decoder wrapper - delegates to standard gsm_decode() API
 *
 * @details This file provides a thin wrapper around the GSM library's gsm_decode()
 *          function. The actual bit unpacking and DSP decoding are handled entirely
 *          by gsm-1.0-pl23/src/gsm_decode.c and its dependent modules.
 *
 ****************************************************************************************
 */

#include "gsm.h"

/**
 ****************************************************************************************
 * @brief Decode one GSM 6.10 frame to PCM
 *
 * @param S      GSM state structure pointer
 * @param c      33-byte compressed GSM frame
 * @param target 160 samples (320 bytes) of 16-bit PCM output
 ****************************************************************************************
 */
void gsm_decode_frame(gsm_state_t *S, uint8_t *c, int16_t *target)
{
    gsm_decode((gsm)S, (gsm_byte *)c, (gsm_signal *)target);
}
