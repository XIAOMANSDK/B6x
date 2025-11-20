/**
 ****************************************************************************************
 *
 * @file rco_test.c
 *
 * @brief RC32K/RC16M clkout and calib. RC振荡器校准与监控示例
 *
 * @details
 * 本示例演示RC振荡器（RC32K和RC16M）的校准和监控功能：
 * - 配置RC32K使用DPLL参考时钟进行校准
 * - 使用二分法校准RC16M振荡器
 * - 实时监控振荡器频率稳定性
 * - 通过RTC记录校准和漂移事件时间
 *
 * 工作原理：
 * 1. 校准过程：使用高精度参考时钟测量RC振荡器频率，调整trim值使其接近目标频率
 * 2. 监控过程：定期重新校准并与历史值比较，检测频率漂移
 * 3. 状态指示：通过GPIO引脚显示校准和监控状态
 * 4. 时间记录：使用RTC提供精确的时间戳
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

/// GPIO引脚定义 - 用于状态指示和时钟输出
#define GPIO_32K            GPIO11  ///< RC32K校准状态指示引脚
#define GPIO_16M            GPIO12  ///< RC16M校准状态指示引脚  
#define GPIO_RUN            GPIO13  ///< 主循环运行状态指示引脚

/// RC32K校准参数配置
#define RC32K_REF_CLK       RCLK_DPLL      ///< 参考时钟选择：DPLL（高精度）
#define RC32K_CAL_CTL       RCAL_CYCLES(4) ///< 校准控制：4个校准周期
#define RC32K_CAL_NB        8              ///< 校准次数：执行8次校准取稳定值
#define RC32K_CAL_DIFF      2              ///< 校准差异阈值：LSB变化超过2视为漂移

/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (RC32K_TEST)

uint16_t rc32kCal;  ///< RC32K校准值存储 - 用于比较频率漂移

/**
 ****************************************************************************************
 * @brief 绝对值计算函数
 *
 * @param[in] val 输入数值
 * @return int 绝对值
 *
 * @details
 * 用于计算校准值的绝对差异，判断频率漂移是否超过阈值
 ****************************************************************************************
 */
static __forceinline int co_abs(int val)
{
    return val < 0 ? val*(-1) : val;
}

/**
 ****************************************************************************************
 * @brief RC32K初始化函数
 *
 * @details
 * 配置和校准RC32K振荡器的完整流程：
 * 1. 配置时钟输出引脚，便于观察RC32K时钟
 * 2. 初始化RC32K校准参数（参考时钟和校准控制）
 * 3. 执行多次校准获取稳定的trim值
 * 4. 通过GPIO指示校准过程
 *
 * 校准原理：
 * - 使用DPLL等高精度时钟作为参考
 * - 通过校准算法调整RC32K的trim值
 * - trim值包含MSB(4bit)和LSB(10bit)两部分
 ****************************************************************************************
 */
static void rc32kInit(void)
{
    // 配置时钟输出引脚，观察RC32K时钟信号
    iospc_clkout(CLK_OUT_LSI);  ///< 配置LSI(32K)时钟输出，便于频率测量
    
    /**
     * @brief 配置RC32K校准参数
     * 
     * rc32k_conf(RC32K_REF_CLK, RC32K_CAL_CTL):
     * - RC32K_REF_CLK: 参考时钟选择（RCLK_DPLL = 1）
     * - RC32K_CAL_CTL: 校准控制（4个校准周期）
     * 
     * 校准控制位说明：
     * - RCAL_CYCLES(4): 校准周期数 = 4
     * - RCAL_SCALE_EN: 比例因子使能（未使用）
     * - RCAL_DELAY_EN: 延迟使能（未使用）
     */
    rc32k_conf(RC32K_REF_CLK, RC32K_CAL_CTL);
    
    /**
     * @brief 执行多次校准获取稳定值
     * 
     * 循环执行RC32K_CAL_NB(8)次校准：
     * - 每次校准时设置GPIO指示
     * - 调用rc32k_calib()获取trim值
     * - 清除GPIO指示
     * - 输出每次校准的MSB和LSB值
     */
    for (uint8_t i = 0; i < RC32K_CAL_NB; i++)
    {
        GPIO_DAT_SET(GPIO_32K);        ///< 设置校准状态指示
        rc32kCal = rc32k_calib();      ///< 执行RC32K校准，返回trim值
        GPIO_DAT_CLR(GPIO_32K);        ///< 清除校准状态指示
        
        /**
         * @brief 输出校准结果
         * 
         * trim值结构：
         * - MSB: 高4位 (rc32kCal & 0xF)
         * - LSB: 低10位 (rc32kCal >> 4)
         * 
         * 调试输出格式：RC32K CalX(msb:0xX,lsb:0xX)
         */
        debug("RC32K Cal%d(msb:0x%X,lsb:0x%X)\\r\\n", i, rc32kCal&0xF, rc32kCal>>4);
    }
}

/**
 ****************************************************************************************
 * @brief RC32K频率漂移检测函数
 *
 * @return bool true=检测到频率漂移, false=频率稳定
 *
 * @details
 * 检测RC32K频率是否发生显著漂移：
 * 1. 执行单次校准获取当前trim值
 * 2. 比较当前值与历史值的差异
 * 3. MSB变化或LSB变化超过阈值视为漂移
 * 4. 通过GPIO指示检测过程
 ****************************************************************************************
 */
static bool rc32kChgd(void)
{
    uint16_t curcal;  ///< 当前校准值
    
    GPIO_DAT_SET(GPIO_32K);        ///< 设置检测状态指示
    curcal = rc32k_calib();        ///< 执行单次校准获取当前值
    GPIO_DAT_CLR(GPIO_32K);        ///< 清除检测状态指示
    
    /**
     * @brief 频率漂移判断条件
     * 
     * 满足以下任一条件即认为发生频率漂移：
     * 1. MSB发生变化：((curcal&0xF) != (rc32kCal&0xF))
     * 2. LSB变化超过阈值：(co_abs((curcal>>4)-(rc32kCal>>4)) > RC32K_CAL_DIFF)
     * 
     * 其中RC32K_CAL_DIFF=2，即LSB变化超过2视为显著漂移
     */
    if (((curcal&0xF) != (rc32kCal&0xF))/*MSB变化*/ 
        || (co_abs((curcal>>4)-(rc32kCal>>4)) > RC32K_CAL_DIFF)/*LSB超阈值*/)
    {
        /**
         * @brief 频率漂移处理
         * 
         * 输出漂移信息并更新历史值：
         * - 记录漂移发生时的校准值
         * - 输出调试信息便于分析
         */
        debug("RC32K Changed(msb:0x%X,lsb:0x%X)\\r\\n", rc32kCal&0xF, rc32kCal>>4);
        return true;  ///< 返回检测到漂移
    }
    return false;  ///< 返回频率稳定
}
#endif

#if (RC16M_TEST)
uint8_t rc16mCal;  ///< RC16M校准值存储 - 用于比较频率漂移

/**
 ****************************************************************************************
 * @brief RC16M初始化函数
 *
 * @details
 * 配置和校准RC16M振荡器的完整流程：
 * 1. 配置时钟输出引脚，便于观察RC16M时钟
 * 2. 获取当前trim值作为参考
 * 3. 执行二分法校准获取最优trim值
 * 4. 通过GPIO指示校准过程
 * 5. 输出校准前后的trim值对比
 *
 * 校准原理：
 * - 使用二分查找算法在0~63范围内寻找最优trim值
 * - 通过测量实际频率与目标频率的偏差进行调整
 * - 最终得到使频率最接近16MHz的trim值
 ****************************************************************************************
 */
static void rc16mInit(void)
{
    uint8_t curtrim;  ///< 当前trim值
    
    iospc_clkout(CLK_OUT_HSI);  ///< 配置HSI(16M)时钟输出，便于频率测量
    
    curtrim = rc16m_trim_get();  ///< 获取当前RC16M trim值
    
    GPIO_DAT_SET(GPIO_16M);      ///< 设置校准状态指示
    rc16mCal = rc16m_calib();    ///< 执行RC16M二分法校准
    GPIO_DAT_CLR(GPIO_16M);      ///< 清除校准状态指示
    
    /**
     * @brief 输出校准结果对比
     * 
     * 调试输出格式：RC16M Cal(X->Y)
     * - X: 校准前的trim值
     * - Y: 校准后的trim值
     * 
     * trim值范围：0~63，共64个可调级别
     */
    debug("RC16M Cal(%d->%d)\\r\\n", curtrim, rc16mCal);
}

/**
 ****************************************************************************************
 * @brief RC16M频率漂移检测函数
 *
 * @return bool true=检测到频率漂移, false=频率稳定
 *
 * @details
 * 检测RC16M频率是否发生显著漂移：
 * 1. 执行单次校准获取当前trim值
 * 2. 比较当前值与历史值的差异
 * 3. trim值变化即视为漂移（RC16M灵敏度较高）
 * 4. 通过GPIO指示检测过程
 * 5. 更新历史值供下次比较
 ****************************************************************************************
 */
static bool rc16mChgd(void)
{
    uint8_t curcal;  ///< 当前校准值
    
    GPIO_DAT_SET(GPIO_16M);      ///< 设置检测状态指示
    curcal = rc16m_calib();      ///< 执行单次校准获取当前值
    GPIO_DAT_CLR(GPIO_16M);      ///< 清除检测状态指示
    
    /**
     * @brief 频率漂移判断
     * 
     * RC16M对trim值变化更敏感，任何变化都视为漂移：
     * - 比较当前校准值与历史值
     * - 如有变化则输出漂移信息并更新历史值
     */
    if (curcal != rc16mCal)
    {
        /**
         * @brief 频率漂移处理
         * 
         * 输出漂移信息并更新历史值：
         * - 记录漂移前后的trim值变化
         * - 更新rc16mCal为当前值供下次比较
         * - 输出调试信息便于分析
         */
        debug("RC16M Changed(%d->%d)\\r\\n", rc16mCal, curcal);
        rc16mCal = curcal;  ///< 更新历史值为当前值
        return true;        ///< 返回检测到漂移
    }
    return false;  ///< 返回频率稳定
}
#endif

/**
 ****************************************************************************************
 * @brief RCO测试主函数
 *
 * @details
 * 执行RC振荡器校准和监控的完整测试流程：
 * 1. GPIO初始化：配置状态指示引脚
 * 2. RC16M初始化校准（如启用）
 * 3. RC32K初始化校准（如启用）  
 * 4. RTC配置：用于记录校准和漂移时间
 * 5. 主循环监控：持续检测频率漂移并记录时间
 *
 * 监控策略：
 * - 循环检测两个RC振荡器的频率稳定性
 * - 发现漂移时记录RTC时间戳
 * - 通过GPIO指示监控状态
 ****************************************************************************************
 */
void rcoTest(void)
{
    bool chgd = false;     ///< 频率漂移标志
    rtc_time_t time;       ///< RTC时间结构体
    
    /**
     * @brief GPIO方向配置
     * 
     * 配置所有状态指示引脚为输出模式：
     * - GPIO_32K: RC32K校准状态
     * - GPIO_16M: RC16M校准状态
     * - GPIO_RUN: 主循环运行状态
     */
    GPIO_DIR_SET_LO(GPIO_32K | GPIO_16M | GPIO_RUN);
    
    /**
     * @brief RC16M初始化校准（如启用）
     * 
     * 条件编译：仅在RC16M_TEST定义时执行
     * 执行RC16M的初始校准并记录基准值
     */
    #if (RC16M_TEST)
    rc16mInit();
    #endif

    /**
     * @brief RC32K初始化校准（如启用）
     * 
     * 条件编译：仅在RC32K_TEST定义时执行
     * 执行RC32K的多次校准并记录基准值
     */
    #if (RC32K_TEST)
    rc32kInit();
    #endif
    
    /**
     * @brief RTC配置和时间获取
     * 
     * rtc_conf(true): 配置并使能RTC
     * rtc_time_get(): 获取当前RTC时间
     * 输出初始时间戳作为参考
     */
    rtc_conf(true);                    ///< 配置并使能RTC
    time = rtc_time_get();             ///< 获取当前RTC时间
    debug("RTC Time:%d.%03d\\r\\n", time.sec, time.ms);  ///< 输出初始时间

    /**
     * @brief 主循环监控
     * 
     * 持续监控RC振荡器频率稳定性：
     * - 设置运行状态指示
     * - 检测RC16M频率漂移（如启用）
     * - 检测RC32K频率漂移（如启用）
     * - 发现漂移时记录RTC时间
     * - 清除运行状态指示
     */
    while (1)
    {
        GPIO_DAT_SET(GPIO_RUN);  ///< 设置运行状态指示
        
        /**
         * @brief RC16M频率监控（如启用）
         * 
         * 调用rc16mChgd()检测频率漂移
         * 如检测到漂移则设置chgd标志
         */
        #if (RC16M_TEST)
        if (rc16mChgd()) chgd = true;
        #endif
        
        /**
         * @brief RC32K频率监控（如启用）
         * 
         * 调用rc32kChgd()检测频率漂移
         * 如检测到漂移则设置chgd标志
         */
        #if (RC32K_TEST)
        if (rc32kChgd()) chgd = true;
        #endif
    
        /**
         * @brief 漂移事件处理
         * 
         * 当检测到频率漂移时：
         * - 重置chgd标志
         * - 获取当前RTC时间
         * - 输出漂移发生的时间戳
         */
        if (chgd)
        {
            chgd = false;  ///< 清除漂移标志
            
            time = rtc_time_get();  ///< 获取当前RTC时间
            debug("at RTC Time:%d.%03d\\r\\n", time.sec, time.ms);  ///< 输出漂移时间
        }
        
        GPIO_DAT_CLR(GPIO_RUN);  ///< 清除运行状态指示
    }
}

