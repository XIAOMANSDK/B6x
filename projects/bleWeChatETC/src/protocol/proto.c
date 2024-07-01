#include <string.h>
#include "proto.h"
#include "pt_env.h"
#include "uartRb.h"

/// Protocol API
extern void pt_sch_init(proto_t *pt, uint8_t *buf, read_fnct read, parse_fnct parse);
extern void pt_sch_proc(proto_t *pt);

/// Proto: Host <--> Local -- UART1

static proto_t gPT2Host;
static uint8_t gHostBuf[PKT_MAX_SIZE];

void proto_init(parse_fnct parse)
{
    uart1Rb_Init(); //dbgInit();    
    pt_sch_init(&gPT2Host, gHostBuf, uart1Rb_Read, parse);
    
    pt_rsp_standby_rep();
}

void proto_schedule(void)
{
    pt_sch_proc(&gPT2Host);
}
