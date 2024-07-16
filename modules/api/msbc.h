/******************************************************************************
 *
 * Modified SubBand Coding (mSBC)
 *
 * This implementation conforms to :
 *   Advanced Audio Distribution v1.3.2 - Appendix B
 *   Hands-Free Profile v1.8 - Appendix A
 *   Bluetooth Profile Specification
 *
 ******************************************************************************/

#ifndef _MSBC_H
#define _MSBC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>


/*
 * DEFINES
 ****************************************************************************************
 */

#define ALIGN_PCM                   __attribute__((aligned(sizeof(int16_t))))

#define MSBC_SYNCWORD               (0xad)

#define MSBC_NB_SAMPLES             (120)   // 8*15

#define MSBC_NB_PCM16K              (MSBC_NB_SAMPLES)
#define MSBC_NB_PCM8K               (MSBC_NB_SAMPLES / 2)

#define MSBC_PKT_SIZE               (57)    // 4Head + (4*8+15*26+7)>>3

typedef struct msbc_packet
{
    /* Frame Header -  4B */
    uint8_t sync;
    uint8_t rsvd[2];
    uint8_t crc8;
    /* Scale Factor -  4B */
    uint8_t scfs[4];
    /* Audio Sample - 49B */
    uint8_t bits[49];
} msbc_pkt_t;


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 * Reset of the context to start Decoding
 *
 */
void msbc_dstart(void);

/**
 * Decode a mSBC frame to pcm - Standard (16bit, 16K)
 *
 * pkt             Input Frame data, size = MSBC_PKT_SIZE 
 * pcm             Output PCM buffer, size = MSBC_NB_PCM16K
 * pitchx          Number of samples between two consecutives
 *
 * return          0 on success, -1 on invalid header
 */
bool msbc_decode(const void *pkt, int16_t pcm[MSBC_NB_PCM16K]);

/**
 * Reset of the context to start Encoding
 *
 */
void msbc_estart(void);

/**
 * Encode a mSBC frame from pcm - Standard (16bit, 16K)
 *
 * pcm             Input PCM buffer, size = MSBC_NB_PCM16K
 * pkt             Output Frame data, size = MSBC_PKT_SIZE
 *
 */
void msbc_encode(const int16_t pcm[MSBC_NB_PCM16K], void *pkt);

/**
 * Encode a mSBC frame from pcm - Special (16bit, 8K)
 *
 * pcm             Input PCM buffer, size = MSBC_NB_PCM8K
 * pkt             Output Frame data, size = MSBC_PKT_SIZE
 *
 */
void msbc_encode_8k(const int16_t pcm[MSBC_NB_PCM8K], void *pkt);

#ifdef __cplusplus
}
#endif

#endif /* _MSBC_H */
