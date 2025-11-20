/**
 ****************************************************************************************
 *
 * @file sadc.c
 *
 * @brief 逐次逼近型模数转换器(SADC)驱动程序
 *
 * 该文件包含逐次逼近型模数转换器(SADC)的初始化、校准、数据读取、DMA传输、
 * RSSI测量、PCM音频采集、定时器触发转换和随机数生成等功能的实现。
 *
 ****************************************************************************************
 */

#include "reg_sadc.h"
#include "sadc.h"
#include "rcc.h"
#include "gpio.h"
#include "iopad.h"
#include "reg_rf.h"
#include "reg_timer.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// 清除上次标志，启动转换
#define SADC_START()                           \
    dowl(                                      \
        SADC->STCTRL.SADC_AUX_FLG_CLR = 1;     /* 清除辅助标志位 */ \
        SADC->CTRL.SADC_SOC = 1;               /* 启动ADC转换 */ \
    )

// 等待辅助标志位为1(转换完成)，清除标志
#define SADC_AFLG_CLR()    dowl( SADC->STCTRL.SADC_AUX_FLG_CLR = 1; )  /* 清除辅助标志位 */
#define SADC_AFLG_WAIT()   dowl( while (!(SADC->STCTRL.SADC_AUX_FLG)); )  /* 等待转换完成 */

#define SADC_CALIB_CNT     (8)                 /* 校准计数次数 */
#define SADC_CTRL_MSK      (SADC_CR_CALIB_BIT | SADC_CR_CONV_MODE_BIT | SADC_CR_DMAEN_BIT \
                            | SADC_CR_SAMP_MODE_MSK | SADC_CR_DBGEN_BIT)  /* 控制寄存器掩码 */

#define RF_RSV_ADC_TEMP_BIT   (0x01UL)         /* RF温度测量位 */
#define RF_RSV_ADC_1P2V_BIT   (0x02UL)         /* 1.2V参考电压位 */
#define RF_RSV_ADC_RSSI_BIT   (0x04UL)         /* RSSI测量位 */
#define RF_RSV_ADC_VDD_IF_BIT (0x08UL)         /* VDD接口位 */
#define RF_RSV_ADC_MSK        (0x0FUL)         /* RF ADC掩码 */

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * @brief SADC校准函数
 * 
 * @details
 * - 配置麦克风控制寄存器
 * - 设置校准模式
 * - 启动校准转换
 * - 等待校准完成
 * - 清除校准标志
 * - 禁用校准模式
 */
static void sadc_calib(void)
{
    // .MIC_PD=1, .PGA_EN=0
    SADC->MIC_CTRL.Word = SADC_MIC_PWD_BIT;    /* 关闭麦克风，禁用PGA */

    // .Calib_mode=1, .samp_mod=0 .Dbg_ctrl=0
    SADC->CTRL.Word = (SADC->CTRL.Word & ~SADC_CTRL_MSK) | (SADC_CR_CALIB_BIT | SADC_CR_SAMP_SOFT); /* 设置校准模式和软件采样模式 */

    // SADC_START();
    SADC->CTRL.SADC_SOC = 1;                   /* 启动校准转换 */
    SADC_AFLG_WAIT();                          /* 等待校准完成 */

    SADC_AFLG_CLR();                           /* 清除辅助标志位 */

    // disable calibration
    SADC->CTRL.Word &= ~SADC_CR_CALIB_BIT;     /* 禁用校准模式 */
}

/**
 * @brief 初始化SADC模块
 * 
 * @param[in] ana_ctrl 模拟控制寄存器配置值
 * 
 * @details
 * - 重置ADC时钟
 * - 配置模拟控制寄存器
 * - 执行ADC校准
 */
void sadc_init(uint32_t ana_ctrl)
{
    RCC_AHBCLK_DIS(AHB_ADC_BIT);               /* 禁用ADC AHB时钟 */
    RCC_AHBRST_REQ(AHB_ADC_BIT);               /* 请求ADC AHB复位 */
    RCC_AHBCLK_EN(AHB_ADC_BIT);                /* 使能ADC AHB时钟 */

    SADC->SADC_ANA_CTRL.Word = ana_ctrl;       /* 配置模拟控制寄存器 */

    sadc_calib();                              /* 执行ADC校准 */
}

/**
 * @brief 反初始化SADC模块
 * 
 * @details
 * - 禁用ADC时钟
 * - 请求ADC复位
 */
void sadc_deinit(void)
{
    RCC_AHBCLK_DIS(AHB_ADC_BIT);               /* 禁用ADC AHB时钟 */
    RCC_AHBRST_REQ(AHB_ADC_BIT);               /* 请求ADC AHB复位 */
}

/**
 * @brief 配置SADC控制寄存器
 * 
 * @param[in] ctrl 控制寄存器配置值
 * 
 * @details
 * - 清除辅助标志位
 * - 配置控制寄存器
 */
void sadc_conf(uint32_t ctrl)
{
    SADC->STCTRL.SADC_AUX_FLG_CLR = 1;         /* 清除辅助标志位 */
    SADC->CTRL.Word = ctrl;                    /* 配置控制寄存器 */
}

/**
 * @brief 读取SADC转换数据
 * 
 * @param[in] chset 通道选择
 * @param[in] times 采样次数(0表示单次采样，>0表示多次采样取平均)
 * 
 * @return uint16_t ADC转换结果
 * 
 * @details
 * - 配置麦克风控制
 * - 设置转换模式(单次或连续)
 * - 配置通道选择
 * - 启动转换并读取数据
 * - 多次采样时计算平均值
 */
uint16_t sadc_read(uint8_t chset, uint16_t times)
{
    uint16_t dout;

    // .MIC_PD=1, .PGA_EN=0
    SADC->MIC_CTRL.Word = SADC_MIC_PWD_BIT;    /* 关闭麦克风，禁用PGA */

    // .Calib_mode=0, .conv_mode=1 or 0, .sadc_dmac_en=0, .samp_mod=0 .Dbg_ctrl=0
    uint16_t cr_val = (times > 0) ? (SADC_CR_CONV_CONTINUE | SADC_CR_SAMP_SOFT) : (SADC_CR_CONV_SINGLE | SADC_CR_SAMP_SOFT); /* 根据采样次数选择转换模式 */
    SADC->CTRL.Word = (SADC->CTRL.Word & ~SADC_CTRL_MSK) | cr_val; /* 配置控制寄存器 */
    // .auto_sw_ch=0, Set sadc_ch_set0
    SADC->AUTO_SW_CTRL.Word = 0;               /* 禁用自动通道切换 */
    SADC->CH_CTRL.SADC_CH_SET0 = chset;        /* 设置通道选择 */

    //Clear
    SADC_AFLG_CLR();                           /* 清除辅助标志位 */

    // SADC_START();
    SADC->CTRL.SADC_SOC = 1;                   /* 启动ADC转换 */
    SADC_AFLG_WAIT();                          /* 等待转换完成 */
    dout = SADC->AUX_ST.SADC_AUX_DOUT;         /* 读取转换结果 */
    SADC_AFLG_CLR();                           /* 清除辅助标志位 */

    if (times > 0)
    {
        for (uint16_t i = 1; i < times; i++)
        {
            dout += SADC->AUX_ST.SADC_AUX_DOUT; /* 累加多次采样结果 */
        }

        dout /= times;                         /* 计算平均值 */
        // Stop continuous mode .conv_mode=0 .sadc_soc=1
        SADC->CTRL.SADC_CONV_MODE = 0;         /* 停止连续转换模式 */
        SADC->CTRL.SADC_SOC = 1;               /* 启动单次转换以停止连续模式 */
        SADC_AFLG_CLR();                       /* 清除辅助标志位 */
    }

    return dout;
}

/**
 * @brief 停止SADC转换
 * 
 * @details
 * - 停止连续转换模式
 * - 启动单次转换以停止连续模式
 * - 清除辅助标志位
 */
void sadc_stop(void)
{
    // Stop continuous mode .conv_mode=0 .sadc_soc=1
    SADC->CTRL.SADC_CONV_MODE = 0;             /* 停止连续转换模式 */
    SADC->CTRL.SADC_SOC = 1;                   /* 启动单次转换以停止连续模式 */
    SADC_AFLG_CLR();                           /* 清除辅助标志位 */
}

/**
 * @brief 配置SADC DMA传输
 * 
 * @param[in] sw_auto 自动切换控制(0x10表示停止DMA，其他值配置自动切换)
 * @param[in] ch_ctrl 通道控制配置
 * 
 * @details
 * - 配置DMA连续转换模式
 * - 设置自动通道切换
 * - 启动DMA转换
 * - 或停止DMA转换
 */
void sadc_dma(uint8_t sw_auto, uint32_t ch_ctrl)
{
    if (sw_auto < 0x10)
    {
        // .MIC_PD=1, .PGA_EN=0
        SADC->MIC_CTRL.Word = SADC_MIC_PWD_BIT; /* 关闭麦克风，禁用PGA */

        // .Calib_mode=0, .conv_mode=1, .sadc_dmac_en=1, .samp_mod=0 .Dbg_ctrl=0
        SADC->CTRL.Word = (SADC->CTRL.Word & ~SADC_CTRL_MSK) | (SADC_CR_CONV_CONTINUE | SADC_CR_DMAEN_BIT | SADC_CR_SAMP_SOFT); /* 配置DMA连续转换模式 */
        // .auto_sw_ch=0 or 1, Set sadc_ch_set0
        SADC->AUTO_SW_CTRL.Word = sw_auto;     /* 设置自动通道切换 */
        SADC->CH_CTRL.SADC_CH_SET0 = ch_ctrl;  /* 设置通道控制 */

        // SADC_START();
        SADC->CTRL.SADC_SOC = 1;               /* 启动DMA转换 */
    }
    else
    {
        // Stop continuous mode .conv_mode=0 .sadc_soc=1
        SADC->CTRL.SADC_CONV_MODE = 0;         /* 停止连续转换模式 */
        SADC->CTRL.SADC_SOC = 1;               /* 启动单次转换以停止连续模式 */
    }
}

/**
 * @brief 配置SADC进行RSSI测量
 * 
 * @param[in] rf_rsv RF保留寄存器配置
 * 
 * @details
 * - 配置RSSI采样模式
 * - 设置RF通道
 * - 配置RF保留寄存器
 */
void sadc_rssi(uint8_t rf_rsv)
{
    // .MIC_PD=1, .PGA_EN=0
    SADC->MIC_CTRL.Word = SADC_MIC_PWD_BIT;    /* 关闭麦克风，禁用PGA */

    // .Calib_mode=0, .conv_mode=0, .sadc_dmac_en=0, .samp_mod=1 .Dbg_ctrl=0
    SADC->CTRL.Word = (SADC->CTRL.Word & ~SADC_CTRL_MSK) | (SADC_CR_SAMP_RSSI); /* 配置RSSI采样模式 */
    // .auto_sw_ch=0, .sadc_ch_set0=15
    SADC->AUTO_SW_CTRL.Word = 0;               /* 禁用自动通道切换 */
    SADC->CH_CTRL.SADC_CH_SET0 = SADC_CH_RFRSV; /* 设置RF保留通道 */
    RF->RF_RSV = rf_rsv;                       /* 配置RF保留寄存器 */

    // Set sadc_rssi_samp_dly sadc_aux_clk_div=16M
    // no need software set sadc_soc to start
}

/**
 * @brief 配置SADC进行PCM音频采集
 * 
 * @param[in] mic_sel 麦克风控制配置(0表示停止PCM，非0表示启动PCM)
 * 
 * @details
 * - 配置麦克风输入IO
 * - 设置PGA和直流偏移
 * - 配置PCM采样模式
 * - 启动或停止PCM采集
 */
void sadc_pcm(uint32_t mic_sel)
{
    if (mic_sel)
    {
        // micbias and micin io configure
        GPIO_DIR_CLR(GPIO02 | GPIO03);         /* 清除GPIO方向 */
        iom_ctrl(PA02, IOM_HIZ);               /* 配置PA02为高阻态 */
        iom_ctrl(PA03, IOM_ANALOG);            /* 配置PA03为模拟模式 */

        // .MIC_PD=0, .PGA_EN=1; Set PGA_VOL
        SADC->MIC_CTRL.Word = mic_sel;         /* 配置麦克风控制和PGA */
        SADC->DC_OFFSET = 0x200;               /* 设置直流偏移 */

        // .Calib_mode=0, .conv_mode=0, .sadc_dmac_en=1, .samp_mod=2 .Dbg_ctrl=0
        SADC->CTRL.Word = (SADC->CTRL.Word & ~SADC_CTRL_MSK) | (SADC_CR_DMAEN_BIT | SADC_CR_SAMP_PCM); /* 配置PCM采样模式 */

        // .auto_sw_ch=0, .sadc_ch_set0=4
        SADC->AUTO_SW_CTRL.Word = 0;           /* 禁用自动通道切换 */
        SADC->CH_CTRL.SADC_CH_SET0 = SADC_CH_PGAOUT; /* 设置PGA输出通道 */

        // SADC_START();
        SADC->CTRL.SADC_SOC = 1;               /* 启动PCM采集 */
    }
    else
    {
        // Stop decimation filter mode
        SADC->CTRL.SADC_DECIM_END = 1;         /* 停止抽取滤波器模式 */
    }
}

/**
 * @brief 配置高级定时器触发SADC转换
 * 
 * @param[in] sw_auto 自动切换控制
 * @param[in] ch_ctrl 通道控制配置
 * 
 * @details
 * - 配置高级定时器主模式选择为更新事件
 * - 设置定时器触发采样模式
 * - 配置自动通道切换和通道选择
 * - 启动定时器触发转换
 */
void sadc_atmr(uint8_t sw_auto, uint32_t ch_ctrl)
{
    // Set ADTIM work as basic timer and trigger source is uevent (mms = 3'b010)
    ATMR->CR2.MMS = 2;                         /* 配置主模式选择为更新事件 */

    // .MIC_PD=1, .PGA_EN=0
    SADC->MIC_CTRL.Word = SADC_MIC_PWD_BIT;    /* 关闭麦克风，禁用PGA */

    // .Calib_mode=0, .conv_mode=0, .sadc_dmac_en=1, .samp_mod=3 .Dbg_ctrl=0 .sadc_aux_clk_div=0
    SADC->CTRL.Word = (SADC->CTRL.Word & ~(SADC_CTRL_MSK | SADC_CR_CLK_DIV_MSK)) | (SADC_CR_DMAEN_BIT | SADC_CR_SAMP_ADTMR); /* 配置定时器触发采样模式 */
    // .auto_sw_ch=0 or 1, Set sadc_ch_set0
    SADC->AUTO_SW_CTRL.Word = sw_auto;         /* 设置自动通道切换 */
    SADC->CH_CTRL.SADC_CH_SET0 = ch_ctrl;      /* 设置通道控制 */

    // SADC_START();
    SADC->CTRL.SADC_SOC = 1;                   /* 启动定时器触发转换 */
}

/**
 * @brief 生成随机数
 * 
 * @return uint32_t 生成的32位随机数
 * 
 * @details
 * - 保存当前SADC配置
 * - 配置RF和SADC用于随机数生成
 * - 通过多次采样ADC最低位生成随机数
 * - 恢复原始SADC配置
 */
uint32_t sadc_rand_num(void)
{
    uint32_t random_num = 0;
    uint32_t sadc_bak[5] = {0};

    if (SADC->SADC_ANA_CTRL.Word & SADC_EN_BIT)
    {
        sadc_bak[0]  = SADC->SADC_ANA_CTRL.Word; /* 保存模拟控制寄存器 */
        sadc_bak[1]  = SADC->CTRL.Word;        /* 保存控制寄存器 */
        sadc_bak[2]  = SADC->MIC_CTRL.Word;    /* 保存麦克风控制寄存器 */
        sadc_bak[3]  = SADC->AUTO_SW_CTRL.Word; /* 保存自动切换控制寄存器 */
        sadc_bak[4]  = SADC->CH_CTRL.Word;     /* 保存通道控制寄存器 */
    }

    // .EN_BG
    RF->ANA_EN_CTRL.Word     = 0x00070000;     /* 配置RF模拟使能控制 */
    // bit0:rf_temp
    RF->RF_RSV               = 0x0000B801;     /* 配置RF保留寄存器 */

    // SADC_EN_BIT setup about 4us.
    SADC->SADC_ANA_CTRL.Word = 0x0011B6D9;     /* 配置模拟控制寄存器 */
    SADC->CTRL.Word          = 0x09FB0010;     /* 配置控制寄存器 */
    SADC->CH_CTRL.Word       = SADC_CH_RFRSV;  /* 设置RF保留通道 */

    for (uint8_t cnt = 0; cnt < 32; cnt++)
    {
        // clk must <= 16M.
        SADC->CTRL.SADC_AUX_CLK_DIV = 3 + cnt; /* 设置辅助时钟分频 */

        SADC->SADC_ANA_CTRL.Word    = ((cnt & 0x07) << SADC_IBSEL_CMP_LSB) | ((cnt & 0x07) << SADC_IBSEL_VCM_LSB)
                                      | ((cnt & 0x07) << SADC_IBSEL_VREF_LSB) | ((cnt & 0x07) << SADC_IBSEL_BUF_LSB)
                                      | ((cnt & 0x03) << SADC_CALCAP_LSB) | SADC_INBUF_BYPSS_BIT | SADC_VREF_VBG | SADC_EN_BIT; /* 动态配置模拟参数 */
        // start conversion
        SADC->CTRL.SADC_SOC         = 1;       /* 启动ADC转换 */

        SADC_AFLG_WAIT();                      /* 等待转换完成 */

        // clear flag
        SADC->STCTRL.SADC_AUX_FLG_CLR = 1;     /* 清除辅助标志位 */

        random_num |= ((((SADC->AUX_ST.Word) & 0x01)) << (cnt)); /* 采集最低位构建随机数 */
    }

    RF->RF_RSV             = 0x0000B800;       /* 恢复RF保留寄存器 */

    if (sadc_bak[0] & SADC_EN_BIT)
    {
        SADC->MIC_CTRL.Word      = sadc_bak[2]; /* 恢复麦克风控制寄存器 */
        SADC->AUTO_SW_CTRL.Word  = sadc_bak[3]; /* 恢复自动切换控制寄存器 */
        SADC->CH_CTRL.Word       = sadc_bak[4]; /* 恢复通道控制寄存器 */
        SADC->SADC_ANA_CTRL.Word = sadc_bak[0]; /* 恢复模拟控制寄存器 */
        SADC->CTRL.Word          = sadc_bak[1]; /* 恢复控制寄存器 */

        sadc_calib();                          /* 重新校准ADC */
    }

    return random_num;
}

#ifdef SADC_ANA_VREF_1V2
#undef SADC_ANA_VREF_1V2
#endif
#define SADC_ANA_VREF_1V2     (SADC_VREF_VBG | SADC_CALCAP_PRECISION | SADC_IBSEL_BUF(3) | SADC_IBSEL_VREF(3)  \
                               | SADC_IBSEL_VCM(3) | SADC_IBSEL_CMP(3) | SADC_VREF_TRIM_1V2 | SADC_EN_BIT | SADC_INBUF_BYPSS_BIT) /* 1.2V参考电压配置 */

#define SADC_CR_TEMP                (SADC_CR_HPF(11) | SADC_CR_CLKPH_POSEDGE | SADC_CR_SAMP_SOFT | SADC_CR_CONV_SINGLE) /* 温度测量控制配置 */
#define SADC_CLK_DIV_TEMP(_sys_clk) (((_sys_clk + 1) * 160) - 1) // div_100K  /* 温度测量时钟分频计算 */

/**
 * @brief 测量温度
 * 
 * @param[in] times 采样次数
 * 
 * @return uint16_t 温度测量结果
 * 
 * @details
 * - 保存当前配置
 * - 配置RF温度测量
 * - 初始化SADC进行温度测量
 * - 多次采样取平均
 * - 恢复原始配置
 */
uint16_t sadc_temperature(uint16_t times)
{
    if (times == 0)
        return 0;
    
    uint32_t sadc_bak[5] = {0};
    uint32_t rf_rsv_bak;
    uint32_t sadc_val = 0;;
    uint8_t sys_clk = rcc_sysclk_get();

    if ((SADC->SADC_ANA_CTRL.Word & SADC_EN_BIT) && (SADC->CH_CTRL.SADC_CH_SET0 != SADC_CH_RFRSV))
    {
        sadc_bak[0]  = SADC->SADC_ANA_CTRL.Word; /* 保存模拟控制寄存器 */
        sadc_bak[1]  = SADC->CTRL.Word;        /* 保存控制寄存器 */
        sadc_bak[2]  = SADC->MIC_CTRL.Word;    /* 保存麦克风控制寄存器 */
        sadc_bak[3]  = SADC->AUTO_SW_CTRL.Word; /* 保存自动切换控制寄存器 */
        sadc_bak[4]  = SADC->CH_CTRL.Word;     /* 保存通道控制寄存器 */
    }
    
    // RF_RSV<0> 温度
    rf_rsv_bak = RF->RF_RSV;                   /* 保存RF保留寄存器 */
    if ((RF->RF_RSV & RF_RSV_ADC_TEMP_BIT) != RF_RSV_ADC_TEMP_BIT)
    {
        RF->RF_RSV &= ~RF_RSV_ADC_MSK;         /* 清除RF ADC掩码 */
        RF->RF_RSV |= RF_RSV_ADC_TEMP_BIT;     /* 设置温度测量位 */
    }
    
    sadc_init(SADC_ANA_VREF_1V2);              /* 初始化SADC使用1.2V参考电压 */
    SADC->CH_CTRL.Word      = SADC_CH_RFRSV;   /* 设置RF保留通道 */
    SADC->AUTO_SW_CTRL.Word = 0;               /* 禁用自动通道切换 */
    SADC->CTRL.Word = SADC_CR_TEMP | (SADC_CLK_DIV_TEMP(sys_clk) << SADC_CR_CLK_DIV_LSB); /* 配置温度测量控制 */
    
    SADC->STCTRL.SADC_AUX_FLG_CLR = 1;         /* 清除辅助标志位 */
    
    for (uint8_t cnt = 0; cnt < times; cnt++)
    {
        // start conversion
        SADC->CTRL.SADC_SOC           = 1;     /* 启动温度转换 */
        
        // wait conversion done is 1
        while (! (SADC->STCTRL.SADC_AUX_FLG)); /* 等待转换完成 */
        
        // clear flag
        SADC->STCTRL.SADC_AUX_FLG_CLR = 1;     /* 清除辅助标志位 */
        
        sadc_val += ((SADC->AUX_ST.Word) & 0x03FF); /* 累加转换结果 */
    }
    
    // average
    sadc_val /= times;                         /* 计算平均值 */
    
    RF->RF_RSV = rf_rsv_bak;                   /* 恢复RF保留寄存器 */
    
    if (sadc_bak[0] & SADC_EN_BIT)
    {
        SADC->MIC_CTRL.Word      = sadc_bak[2]; /* 恢复麦克风控制寄存器 */
        SADC->AUTO_SW_CTRL.Word  = sadc_bak[3]; /* 恢复自动切换控制寄存器 */
        SADC->CH_CTRL.Word       = sadc_bak[4]; /* 恢复通道控制寄存器 */
        SADC->SADC_ANA_CTRL.Word = sadc_bak[0]; /* 恢复模拟控制寄存器 */
        SADC->CTRL.Word          = sadc_bak[1]; /* 恢复控制寄存器 */
        
        sadc_calib();                          /* 重新校准ADC */
    }
    
    return (uint16_t)sadc_val;
}
