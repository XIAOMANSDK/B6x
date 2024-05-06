/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */



/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void sysInit(void)
{
    // Todo config, if need
    
}

static void devInit(void)
{
    iwdt_disable();
    
    // Init debug
    dbgInit();
}

extern void spisTest(void);

int main(void)
{
    sysInit();
    
    devInit();

    spisTest();
}
