#include "drvs.h"
#include "uartRb.h"
#include "pt_env.h"
#include "proto.h"

/// Proto: Host <--> Local -- UART1

__ATTR_SRAM static proto_t gPT2Host;

__ATTR_SRAM static uint8_t gHostBuf[PKT_MAX_SIZE];

void proto_init(parse_fnct parse)
{
    uart1Rb_Init();
    pt_sch_init(&gPT2Host, gHostBuf, uart1Rb_Read, parse);
    
//    uint8_t init_data[4] = {0x55, 0xAA, 0x5A, 0xA5};
//    uart_send(UART1_PORT, sizeof(init_data), init_data);
}

void proto_schedule(void)
{
    pt_sch_proc(&gPT2Host);
}
