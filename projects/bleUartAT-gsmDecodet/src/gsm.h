/* gsm.h */
#ifndef GSM_H
#define GSM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GSM 6.10 state structure - MUST use name 'gsm_state' for GSM library compatibility */
#ifndef GSM_STATE_DEFINED
#define GSM_STATE_DEFINED
struct gsm_state {
    int16_t  dp0[280];      /* History data buffer */
    int16_t  e[50];         /* For code.c */
    int16_t  z1;            /* Preprocessing state */
    int32_t  L_z2;
    int      mp;
    int16_t  u[8];          /* LPC coefficients */
    int16_t  LARpp[2][8];   /* LAR parameters */
    int16_t  j;             /* Frame counter */
    int16_t  ltp_cut;       /* long_term.c, LTP crosscorr. */
    int16_t  nrp;           /* Long term predictor */
    int16_t  v[9];
    int16_t  msr;           /* decoder.c, Postprocessing */
    int8_t   verbose;       /* only used if !NDEBUG */
    int8_t   fast;          /* only used if FAST */
    int8_t   wav_fmt;       /* only used if WAV49 defined */
    uint8_t  frame_index;   /* odd/even chaining */
    uint8_t  frame_chain;   /* half-byte to carry forward */
};
#endif

/* Convenience typedef for user code */
typedef struct gsm_state gsm_state_t;

/* GSM type definitions (compatible with GSM library) */
typedef struct gsm_state *gsm;
typedef int16_t gsm_signal;
typedef uint8_t gsm_byte;
typedef gsm_byte gsm_frame[33];

/* GSM constants */
#define GSM_MAGIC 0xD  /* 13 kbit/s RPE-LTP */

/* GSM option constants */
#define GSM_OPT_VERBOSE     1
#define GSM_OPT_FAST        2
#define GSM_OPT_LTP_CUT     3
#define GSM_OPT_WAV49       4
#define GSM_OPT_FRAME_INDEX 5
#define GSM_OPT_FRAME_CHAIN 6

/* GSM library interface functions */
gsm gsm_create(void);
void gsm_destroy(gsm S);
int gsm_decode(gsm S, gsm_byte *c, gsm_signal *target);

#ifdef __cplusplus
}
#endif

#endif /* GSM_H */
