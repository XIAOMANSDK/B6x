/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

/* Modified for embedded systems: Use static allocation instead of malloc */

#include "config.h"

#ifdef	HAS_STRING_H
#include <string.h>
#else
#	include "proto.h"
	extern char	* memset P((char *, int, int));
#endif

#include <stdio.h>

#include "gsm.h"
#include "private.h"
#include "proto.h"

/* Static GSM state structure for embedded systems */
static struct gsm_state gsm_state_static;

gsm gsm_create P0()
{
	gsm  r = &gsm_state_static;

	/* Initialize the state structure to zero */
	memset((char *)r, 0, sizeof(*r));
	r->nrp = 40;

	return r;
}
