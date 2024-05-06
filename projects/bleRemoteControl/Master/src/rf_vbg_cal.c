uint16_t ft_rfbandgap_test(void)
{
    // coreldo replace aonldo
    APBMISC->AON_PMU_CTRL.AON_PWR_SEL_RUN = 1;   
    
    uint16_t adc_data = 0, trim_val = 0x10;
    uint16_t max_rfbandgap_voleage = 0, min_rfbandgap_voleage = 0, rfbandgap_voleage = 0, coreldo_voltage = 0;
    
#if 1//(DBG_MODE)
    RF->ANA_TRIM.LDO_RX_TRIM        = 0;
    RF->ANAMISC_CTRL1.AT0_SEL       = 0;
    RF->ANA_PWR_CTRL.TEST_EN_LDO_IF = 1;
    RF->DIG_CTRL.LDO_TEST_EN        = 1;
    RF->ANA_EN_CTRL.EN_BG           = 1;
    
    GPIO->DIR_CLR = 1 << 18;
    iom_ctrl(18, IOM_ANALOG);    
#endif
    
//    coreldo_voltage = RD_32(CORELDO_VOLTAGE_ADDR);
    coreldo_voltage = 1220;//1194;//get_trim_vdd12_voltage();//1200;
  
    max_rfbandgap_voleage = 1170 * 1024 / coreldo_voltage;
    min_rfbandgap_voleage = 1130 * 1024 / coreldo_voltage;
    rfbandgap_voleage = (max_rfbandgap_voleage + min_rfbandgap_voleage) >> 1;
    
    debug("0---max:%d, min:%d, rf_vbg:%d\r\n\r\n",max_rfbandgap_voleage, min_rfbandgap_voleage, rfbandgap_voleage);
    sadc_init(SADC_ANA_VREF_1V2);
    sadc_conf(SADC_CR_DFLT);
    bootDelayMs(2);

    RF->RF_RSV |= 1 << 1; 
    
    for (uint8_t step = 0x10; step > 0; step >>= 1)
    {
        RF->ANA_TRIM.BG_RES_TRIM = trim_val;
        bootDelayMs(2);
        
        adc_data = sadc_read(15, 10);
        debug("0---step:%02x, trim_val:%02X,adc:%d\r\n",step, trim_val,adc_data);
        
        if ((adc_data > min_rfbandgap_voleage) && (adc_data < max_rfbandgap_voleage))
            break;

        trim_val = trim_val + (step >> 1) - ((adc_data > rfbandgap_voleage) ? step : 0);
    }
    
    RF->RF_RSV &= ~(1UL << 1);
   
    debug("0---trim_val:0x%02X\r\n\r\n",trim_val);
    
    return trim_val;
}