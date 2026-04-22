/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

/* Modified for embedded systems: Static allocation, no free needed */

/* $Header: /tmp_amd/presto/export/kbs/jutta/src/gsm/RCS/gsm_destroy.c,v 1.3 1994/11/28 19:52:25 jutta Exp $ */

#include "gsm.h"
#include "config.h"
#include "proto.h"

#ifdef	HAS_STRING_H
#	include	<string.h>
#endif

void gsm_destroy P1((S), gsm S)
{
	/* For static allocation, just clear the state */
	/* This allows reinitialization of the decoder */
	if (S) {
		memset((char *)S, 0, sizeof(struct gsm_state));
		S->nrp = 40;  /* Reset to default value */
	}
}
