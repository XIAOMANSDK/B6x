/**
 ****************************************************************************************
 *
 * @file gpio.h
 *
 * @brief GPIO驱动头文件
 *
 * @details 该文件提供了GPIO控制的相关宏定义和函数声明
 *
 ****************************************************************************************
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdint.h>
#include "iopad.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#if !defined(BIT)
    #define BIT(n)             (1UL << (n))
#endif

/**
 * @brief GPIO引脚位定义
 */
enum gpio_bits
{
    GPIO00                   = (1UL << PA00),  /**< GPIO00引脚 */
    GPIO01                   = (1UL << PA01),  /**< GPIO01引脚 */
    GPIO02                   = (1UL << PA02),  /**< GPIO02引脚 */
    GPIO03                   = (1UL << PA03),  /**< GPIO03引脚 */
    GPIO04                   = (1UL << PA04),  /**< GPIO04引脚 */
    GPIO05                   = (1UL << PA05),  /**< GPIO05引脚 */
    GPIO06                   = (1UL << PA06),  /**< GPIO06引脚 */
    GPIO07                   = (1UL << PA07),  /**< GPIO07引脚 */
    GPIO08                   = (1UL << PA08),  /**< GPIO08引脚 */
    GPIO09                   = (1UL << PA09),  /**< GPIO09引脚 */
    GPIO10                   = (1UL << PA10),  /**< GPIO10引脚 */
    GPIO11                   = (1UL << PA11),  /**< GPIO11引脚 */
    GPIO12                   = (1UL << PA12),  /**< GPIO12引脚 */
    GPIO13                   = (1UL << PA13),  /**< GPIO13引脚 */
    GPIO14                   = (1UL << PA14),  /**< GPIO14引脚 */
    GPIO15                   = (1UL << PA15),  /**< GPIO15引脚 */
    GPIO16                   = (1UL << PA16),  /**< GPIO16引脚 */
    GPIO17                   = (1UL << PA17),  /**< GPIO17引脚 */
    GPIO18                   = (1UL << PA18),  /**< GPIO18引脚 */
    GPIO19                   = (1UL << PA19),  /**< GPIO19引脚 */
};

/**
 * @brief GPIO输出类型定义
 */
enum gpio_out_typ
{
    OE_LOW                   = 0,  /**< 输出低电平 */
    OE_HIGH                  = 1,  /**< 输出高电平 */
};

/**
 * @brief GPIO输入类型定义
 */
enum gpio_in_typ
{
    IE_AIR                   = IOM_INPUT,           /**< 浮空输入 */
    IE_DOWN                  = IOM_INPUT | IOM_PULLDOWN, /**< 下拉输入 */
    IE_UP                    = IOM_INPUT | IOM_PULLUP,   /**< 上拉输入 */
};

/*
 * MACROS DECLARATION
 ****************************************************************************************
 */

#include "reg_gpio.h"

/**
 * @brief 设置GPIO数据输出高电平
 * @param bits 要设置的GPIO位掩码
 * @note 写1有效，写0无效。对应DAT_SET寄存器操作
 */
#define GPIO_DAT_SET(bits)     ( GPIO->DAT_SET = (uint32_t)(bits) )

/**
 * @brief 清除GPIO数据输出低电平
 * @param bits 要清除的GPIO位掩码
 * @note 写1有效，写0无效。对应DAT_CLR寄存器操作
 */
#define GPIO_DAT_CLR(bits)     ( GPIO->DAT_CLR = (uint32_t)(bits) )

/**
 * @brief 翻转GPIO数据输出状态
 * @param bits 要翻转的GPIO位掩码
 * @note 写1有效，写0无效。对应DAT_TOG寄存器操作
 */
#define GPIO_DAT_TOG(bits)     ( GPIO->DAT_TOG = (uint32_t)(bits) )

/**
 * @brief 设置GPIO数据操作屏蔽
 * @param bits 要屏蔽的GPIO位掩码
 * @note 写1：阻止set/clear/toggle操作，写0：正常操作。对应DAT_MSK寄存器操作
 */
#define GPIO_DAT_MSK(bits)     ( GPIO->DAT_MSK = (uint32_t)(bits) )

/**
 * @brief 获取GPIO数据屏蔽状态
 * @return 当前的数据屏蔽寄存器值
 * @note 读1：屏蔽，读0：不屏蔽
 */
#define GPIO_DAT_MSK_GET()     ( GPIO->DAT_MSK )

/**
 * @brief 配置GPIO数据输出状态
 * @param bits GPIO数据配置位掩码
 * @note 写1：输出高电平，写0：输出低电平。对应DAT寄存器操作
 */
#define GPIO_DAT_CFG(bits)     ( GPIO->DAT = (uint32_t)(bits) )

/**
 * @brief 获取GPIO数据输出状态
 * @return 当前的数据寄存器值
 * @note 读1：高电平，读0：低电平
 */
#define GPIO_DAT_GET()         ( GPIO->DAT )

/**
 * @brief 设置GPIO方向为输出使能
 * @param bits 要设置的GPIO位掩码
 * @note 写1有效，写0无效。对应DIR_SET寄存器操作
 */
#define GPIO_DIR_SET(bits)     ( GPIO->DIR_SET = (uint32_t)(bits) )

/**
 * @brief 清除GPIO方向为输出禁用
 * @param bits 要清除的GPIO位掩码
 * @note 写1有效，写0无效。对应DIR_CLR寄存器操作
 */
#define GPIO_DIR_CLR(bits)     ( GPIO->DIR_CLR = (uint32_t)(bits) )

/**
 * @brief 翻转GPIO方向状态
 * @param bits 要翻转的GPIO位掩码
 * @note 写1有效，写0无效。对应DIR_TOG寄存器操作
 */
#define GPIO_DIR_TOG(bits)     ( GPIO->DIR_TOG = (uint32_t)(bits) )

/**
 * @brief 设置GPIO方向操作屏蔽
 * @param bits 要屏蔽的GPIO位掩码
 * @note 写1：阻止set/clear/toggle操作，写0：正常操作。对应DIR_MSK寄存器操作
 */
#define GPIO_DIR_MSK(bits)     ( GPIO->DIR_MSK = (uint32_t)(bits) )

/**
 * @brief 获取GPIO方向屏蔽状态
 * @return 当前的方向屏蔽寄存器值
 * @note 读1：屏蔽，读0：不屏蔽
 */
#define GPIO_DIR_MSK_GET()     ( GPIO->DIR_MSK )

/**
 * @brief 配置GPIO方向状态
 * @param bits GPIO方向配置位掩码
 * @note 写1：输出使能，写0：输出禁用。对应DIR寄存器操作
 */
#define GPIO_DIR_CFG(bits)     ( GPIO->DIR = (uint32_t)(bits) )

/**
 * @brief 获取GPIO方向状态
 * @return 当前的方向寄存器值
 * @note 读1：输出使能，读0：输出禁用
 */
#define GPIO_DIR_GET()         ( GPIO->DIR )

/**
 * @brief 获取GPIO输入电平状态
 * @return 当前的引脚电平状态
 * @note 读1：高电平，读0：低电平。对应PIN寄存器操作，不受MASK寄存器影响
 */
#define GPIO_PIN_GET()         ( GPIO->PIN )

/**
 * @brief 设置GPIO输出使能并输出低电平
 * @param bits 要设置的GPIO位掩码
 */
#define GPIO_DIR_SET_LO(bits)  dowl( GPIO_DAT_CLR(bits); GPIO_DIR_SET(bits); )

/**
 * @brief 设置GPIO输出使能并输出高电平
 * @param bits 要设置的GPIO位掩码
 */
#define GPIO_DIR_SET_HI(bits)  dowl( GPIO_DAT_SET(bits); GPIO_DIR_SET(bits); )

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 * @brief 设置GPIO为高阻态
 * @param pad GPIO引脚编号
 * @note 禁用输出和输入功能，对应DIR_CLR寄存器操作和IOM_HIZ配置
 */
static inline void gpio_set_hiz(uint8_t pad)
{
    // 禁用输出 & 输入
    GPIO_DIR_CLR(1UL << pad);  /* DIR_CLR: 输出禁用 */
    iom_ctrl(pad, IOM_HIZ);
}

/**
 * @brief 配置GPIO为输出模式
 * @param pad GPIO引脚编号
 * @param oe 输出电平类型
 * @note 根据oe参数设置输出电平后使能输出方向
 */
static inline void gpio_dir_output(uint8_t pad, uint8_t oe)
{
    uint32_t bits = 1UL << pad;
    
    if (oe == OE_HIGH)
    {
        GPIO_DAT_SET(bits);  /* DAT_SET: IO输出高电平 */
    }
    else
    {
        GPIO_DAT_CLR(bits);  /* DAT_CLR: IO输出低电平 */
    }
    
    GPIO_DIR_SET(bits);  /* DIR_SET: 输出使能 */
}

/**
 * @brief 设置GPIO输出电平
 * @param pad GPIO引脚编号
 * @param val 输出电平值
 * @note 设置指定引脚的输出电平，对应DAT_SET/DAT_CLR寄存器操作
 */
static inline void gpio_put(uint8_t pad, uint8_t val)
{
    if (val == OE_HIGH)
    {
        GPIO_DAT_SET(1UL << pad);  /* DAT_SET: IO输出高电平 */
    }
    else
    {
        GPIO_DAT_CLR(1UL << pad);  /* DAT_CLR: IO输出低电平 */
    }
}

/**
 * @brief 设置GPIO输出高电平
 * @param pad GPIO引脚编号
 * @note 对应DAT_SET寄存器操作
 */
static inline void gpio_put_hi(uint8_t pad)
{
    GPIO_DAT_SET(1UL << pad);  /* DAT_SET: IO输出高电平 */
}

/**
 * @brief 设置GPIO输出低电平
 * @param pad GPIO引脚编号
 * @note 对应DAT_CLR寄存器操作
 */
static inline void gpio_put_lo(uint8_t pad)
{
    GPIO_DAT_CLR(1UL << pad);  /* DAT_CLR: IO输出低电平 */
}

/**
 * @brief 配置GPIO为输入模式
 * @param pad GPIO引脚编号
 * @param ie 输入类型配置
 * @note 配置GPIO的输入特性（浮空、上拉、下拉）
 */
static inline void gpio_dir_input(uint8_t pad, uint16_t ie)
{
    iom_ctrl(pad, ie);
}

/**
 * @brief 获取GPIO输入电平
 * @param pad GPIO引脚编号
 * @return true-高电平, false-低电平
 * @note 读取PIN寄存器获取当前IO电平状态
 */
static inline bool gpio_get(uint8_t pad)
{
    return ((GPIO_PIN_GET() >> pad) & 0x01);  /* PIN: 反映当前IO电平状态 */
}

/**
 * @brief 获取所有GPIO输入电平状态
 * @return 所有GPIO的输入状态位图
 * @note 读取PIN寄存器获取所有IO电平状态
 */
static inline uint32_t gpio_get_all(void)
{
    return GPIO_PIN_GET();  /* PIN: 反映当前所有IO电平状态 */
}

#endif
