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

enum test_pad
{
    // en output
    PA_OUT00            = PA02,
    PA_OUT01            = PA03,
    PA_OUT10            = PA04,
    PA_OUT11            = PA05,
    // en input
    PA_IN00             = PA08,
    PA_IN01             = PA09,
    PA_IN10             = PA10,
    PA_IN11             = PA11,
};

#define GPIO_OUT0        (BIT(PA_OUT00) | BIT(PA_OUT01))
#define GPIO_OUT1        (BIT(PA_OUT10) | BIT(PA_OUT11))
#define GPIO_IN0         (BIT(PA_IN00)  | BIT(PA_IN01))
#define GPIO_IN1         (BIT(PA_IN10)  | BIT(PA_IN11))


/*
 * FUNCTIONS
 ****************************************************************************************
 */

static uint32_t pin_val; // io value
static uint32_t pin_chg; // io change
static bool io_rst; // 0-nReset, 1-GPIO

static uint32_t padIn2Out(uint32_t val)
{
    uint32_t out = 0;
    
    // map gpio input to output
    if (val & BIT(PA_IN00)) out |= BIT(PA_OUT00);
    if (val & BIT(PA_IN01)) out |= BIT(PA_OUT01);
    if (val & BIT(PA_IN10)) out |= BIT(PA_OUT10);
    if (val & BIT(PA_IN11)) out |= BIT(PA_OUT11);
    
    return out;
}

static void gpioTest(void)
{
    uint32_t tmp_val;
    
    // recover nRESET
    io_rst = 0;
    iospc_rstpin(0);
    
    // Init GPIO
    debug("0-gpio(dir:0x%X,dat:0x%X,pin:0x%X)\r\n", 
                GPIO_DIR_GET(), GPIO_DAT_GET(), GPIO_PIN_GET());
    GPIO_DAT_CLR(GPIO_OUT0);             // Low level
    GPIO_DAT_SET(GPIO_OUT1);             // High level
    GPIO_DIR_SET(GPIO_OUT0 | GPIO_OUT1); // Output enable
    
    GPIO_DIR_CLR(GPIO_IN0 | GPIO_IN1);   // OE disable
    gpio_dir_input(PA_IN00, IE_DOWN);    // Input enable, pull-down
    gpio_dir_input(PA_IN01, IE_DOWN);
    gpio_dir_input(PA_IN10, IE_UP);      // Input enable, pull-up
    gpio_dir_input(PA_IN11, IE_UP);
    debug("1-gpio(dir:0x%X,dat:0x%X,pin:0x%X)\r\n", 
                GPIO_DIR_GET(), GPIO_DAT_GET(), GPIO_PIN_GET());
    
    // Init value(pull-up)
    pin_val = GPIO_IN1;
    
    while (1)
    {
        // pin scan
        tmp_val = gpio_get_all() & (GPIO_IN0 | GPIO_IN1); // get input value
        pin_chg = tmp_val ^ pin_val;
        pin_val = tmp_val;
        
        // pin func
        if (pin_chg)
        {
            uint32_t posedge = pin_val & pin_chg;
            uint32_t negedge = ~pin_val & pin_chg;
            
            if (posedge)
            {
                GPIO_DAT_SET(padIn2Out(posedge)); // relate output
            }
            
            if (negedge)
            {
                GPIO_DAT_CLR(padIn2Out(negedge)); // relate output
                
                if (negedge & BIT(PA_IN11))
                {
                    // toggle rst_pin mode
                    io_rst = !io_rst;
                    iospc_rstpin(io_rst);
                    
                    if (io_rst)
                    {
                        GPIO_DIR_SET_LO(BIT(PA_RSTPIN));
                    }
                }
            }
            
            if (io_rst)
            {
                GPIO_DAT_TOG(BIT(PA_RSTPIN)); // toggle dat
            }
            
            debug("pin trg(val:0x%X,chg:0x%X,pos:0x%X,neg:0x%X)\r\n", 
                        pin_val, pin_chg, posedge, negedge);
        }
    }
}

static void sysInit(void)
{
    // Todo config, if need
    
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
}

int main(void)
{
    sysInit();
    devInit();
    
    gpioTest();
}
