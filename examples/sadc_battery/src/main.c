/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

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
#define SADC_READ_CNT          (6)
#if (SADC_READ_CNT < 3)
    #error "Error SADC Read Count"
#endif

#if ((LI_BATT_MODE + DCDC_MODE) > 1)
    #error "Error CFG"
#endif

static void sysInit(void)
{
    // Todo config, if need
    SYS_CLK_ALTER();
}

void batt_init(void)
{
    #if (DCDC_MODE)
    sadc_init(SADC_ANA_VREF_1V2);
    iom_ctrl(SADC_BATTERY_IO, IOM_ANALOG);
    #elif (LI_BATT_MODE)
    sadc_init(SADC_ANA_VREF_2V4);
    iom_ctrl(SADC_BATTERY_IO, IOM_ANALOG);
    #else
    sadc_init(SADC_ANA_VREF_VDD);
    #endif

    sadc_conf(SADC_CR_DFLT);
}

uint32_t batt_vol(void)
{
    uint32_t vdd12    = get_trim_vdd12_voltage();
    uint32_t sadc_val;
    uint16_t adc_data[SADC_READ_CNT];
    uint16_t max, min, i;

    for (i = 0; i < SADC_READ_CNT; ++i)
    {
        #if (EXT_ADC_IO)
        adc_data[i] = sadc_read(SADC_CH_AIN2, 0) & 0xFFFEU;
        #else
        adc_data[i] = sadc_read(SADC_CH_VDD12, 0) & 0xFFFEU;
        #endif
    }

    max = min = adc_data[0];
    sadc_val = adc_data[0];
    for (i = 1; i < SADC_READ_CNT; ++i)
    {
        if (adc_data[i] > max)
            max = adc_data[i];
        if (adc_data[i] < min)
            min = adc_data[i];

        sadc_val += adc_data[i];
    }

    sadc_val -= (max + min);
    sadc_val /= (SADC_READ_CNT-2);
    // (3v3 / 1024) = (1v2 / sadc_val)
    #if (DCDC_MODE)
    uint32_t v_bat = (vdd12 *  sadc_val * 3) / 1024;
    #elif (LI_BATT_MODE)
    uint32_t v_bat = (vdd12 * 2 *  sadc_val * 2) / 1024;
    #else
    uint32_t v_bat = (vdd12 * 1024) / sadc_val;
    #endif

    debug("val(avge:%" PRIu32 ",min:%d,max:%d), v12:%" PRIu32 ", v_bat:%" PRIu32 "\r\n",
          sadc_val, min, max, vdd12, v_bat);

    return v_bat;
}


void adc_battery(void)
{
    while (1)
    {
        batt_vol();
        bootDelayMs(1000);
    }
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    iwdt_disable();

    dbgInit();
    batt_init();

    debug("RST:%X, SADC Battery...\r\n", rsn);
}

int main(void)
{
    sysInit();
    devInit();

    adc_battery();
}
