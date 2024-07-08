/*
** adpcm.h - include file for adpcm coder.
**
** Version 1.0, 7-Jul-92.
*/
 
#ifndef ADPCM_H
#define ADPCM_H
 
#ifdef __cplusplus
extern "C" {
#endif

struct adpcm_state {
	short    valprev;    /* Previous output value */
	char    index;        /* Index into stepsize table */
};

//len 是采样点的个数，不是字节大小
int adpcm_coder(short* indata, char* outdata, int len, struct adpcm_state* state);
int adpcm_decoder(char* indata, short* outdata, int len, struct adpcm_state* state);
int adpcm_decoder2(char* indata, short* outdata, int len, struct adpcm_state* state);

#ifdef __cplusplus
}  /* extern "C" */
#endif
 
#endif /* ADPCM_H*/
