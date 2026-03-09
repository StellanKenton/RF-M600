/************************************************************************************
 * @file     : drv_adc.c
 * @brief    : ADC驱动层实现 - 提供电压值和处理后的物理量
 ***********************************************************************************/
#include "drv_adc.h"

/* ADC参考电压和分辨率 */
#define ADC_REF_MV      3300u
#define ADC_RESOLUTION  4096u

/* NTC相关参数 */
#define NTC_SERIES_R    10000u  /* NTC串联电阻，单位：欧姆 */
#define NTC_RAW_OPEN    3900u   /* ADC原始值 > 此值：NTC开路 */
#define NTC_RAW_SHORT   50u     /* ADC原始值 < 此值：NTC短路 */

/* VOUT电压分压系数 */
#define VOUT_DIVIDER_RATIO  15353u  /* Vout = 15.3529 * Vsample，定点数：15.3529 * 1000 */

/* NTC 10K温度表: -40~105°C，每1°C一个点，来自规格书 */
static const uint32_t s_ntc_temp_table[] = {
    30488, 28599, 26838, 25195, 23663, 22233, 20897, 19651, 18486, 17397,
    16380, 15428, 14538, 13705, 12925, 12195, 11511, 10869, 10267, 9703,
    9173, 8675, 8208, 7768, 7355, 6967, 6601, 6256, 5932, 5626,
    5338, 5066, 4810, 4568, 4339, 4123, 3919, 3726, 3543, 3371,
    3204, 3053, 2906, 2768, 2636, 2512, 2394, 2282, 2175, 2074,
    1979, 1888, 1802, 1720, 1642, 1568, 1498, 1431, 1367, 1307,
    1249, 1195, 1143, 1093, 1046, 1000, 958, 917, 878, 841,
    806, 772, 740, 710, 681, 653, 626, 601, 577, 553,
    531, 510, 490, 471, 452, 435, 418, 402, 387, 372,
    358, 344, 331, 319, 307, 296, 285, 275, 265, 255,
    246, 237, 229, 221, 213, 205, 198, 191, 185, 178,
    172, 166, 161, 155, 150, 145, 140, 136, 131, 127,
    123, 119, 115, 111, 108, 105, 101, 98, 95, 92,
    89, 86, 84, 81, 79, 76, 74, 72, 70, 68,
    66, 64, 62, 60, 58, 57,
};
#define NTC_TABLE_SIZE  (sizeof(s_ntc_temp_table) / sizeof(s_ntc_temp_table[0]))

/**
 * @brief  将NTC ADC原始值转换为温度
 * @param  adcRaw: ADC原始值
 * @retval 温度值：(温度+40)*10，-40°C=0，0°C=400，105°C=1450
 */
static uint16_t NTC_ADCToTemp(uint16_t adcRaw)
{
    /* 计算NTC两端电压 */
    uint32_t tempMv = ((uint32_t)ADC_REF_MV * adcRaw) / ADC_RESOLUTION;
    uint32_t vDiff = ADC_REF_MV - tempMv;
    
    if (vDiff == 0u)
        return (uint16_t)((NTC_TABLE_SIZE - 1) * 10u);
    
    /* 计算NTC电阻值 */
    uint32_t rOhm = (tempMv * NTC_SERIES_R) / vDiff;
    uint32_t r10 = rOhm / 10u;  /* 转换为10欧姆单位 */

    /* 在温度表中查找对应温度 */
    uint16_t numN = 0, numP;
    for (uint16_t i = 0; i < NTC_TABLE_SIZE; i++) {
        if (r10 > s_ntc_temp_table[i]) {
            numP = i;
            if (numP == 0)
                return 0u;
            
            /* 线性插值 */
            uint32_t d = s_ntc_temp_table[numN] - s_ntc_temp_table[numP];
            if (d == 0u)
                return (uint16_t)(numN * 10u);
            
            uint32_t m = s_ntc_temp_table[numN] - r10;
            uint16_t frac = (uint16_t)((10u * m) / d);
            return (uint16_t)(frac + numN * 10u);
        }
        numN = i;
    }
    
    return (uint16_t)((NTC_TABLE_SIZE - 1) * 10u);
}

/**
 * @brief  处理NTC通道，返回温度值
 * @param  ch: NTC通道
 * @retval 温度值或故障码
 */
static uint16_t ProcessNTC(BSP_ADC_Channel_t ch)
{
    uint16_t raw = BSP_ADC_ReadChannel(ch);
    
    /* 检测开路 */
    if (raw > NTC_RAW_OPEN)
        return DRV_ADC_NTC_FAULT_OPEN;
    
    /* 检测短路 */
    if (raw < NTC_RAW_SHORT)
        return DRV_ADC_NTC_FAULT_SHORT;
    
    /* 转换为温度 */
    return NTC_ADCToTemp(raw);
}

/**
 * @brief  处理电流通道，返回电流值(mA)
 * @param  ch: 电流通道
 * @retval 电流值(mA)
 */
static uint16_t ProcessCurrent(BSP_ADC_Channel_t ch)
{
    uint16_t voltageMv = DRV_ADC_ReadVoltage(ch);
    /* 这里根据实际硬件电路进行电流转换 */
    /* 示例：假设1mV对应1mA，实际需要根据硬件调整 */
    return voltageMv;
}

/**
 * @brief  处理VOUT通道，返回补偿后的输出电压(mV)
 * @param  ch: VOUT通道
 * @retval 补偿后的输出电压(mV)
 */
static uint16_t ProcessVOUT(BSP_ADC_Channel_t ch)
{
    uint16_t sampleMv = DRV_ADC_ReadVoltage(ch);
    /* Vout = 15.3529 * Vsample，使用定点数计算 */
    uint32_t voutMv = ((uint32_t)sampleMv * VOUT_DIVIDER_RATIO) / 1000u;
    return (uint16_t)voutMv;
}

/* ==================== 公共接口实现 ==================== */

uint16_t DRV_ADC_ReadRaw(BSP_ADC_Channel_t ch)
{
    return BSP_ADC_ReadChannel(ch);
}

uint16_t DRV_ADC_ReadVoltage(BSP_ADC_Channel_t ch)
{
    uint16_t raw = BSP_ADC_ReadChannel(ch);
    return (uint16_t)(((uint32_t)raw * ADC_REF_MV) / ADC_RESOLUTION);
}

uint16_t DRV_ADC_GetProcessedValue(BSP_ADC_Channel_t ch)
{
    if (ch >= BSP_ADC_CH_MAX)
        return 0;
    
    switch (ch) {
        /* NTC温度通道 */
        case BSP_ADC_CH_HAND_NTC:
        case BSP_ADC_CH_Heat_REF01:
            return ProcessNTC(ch);
        
        /* 电流通道 */
        case BSP_ADC_CH_US_I:
        case BSP_ADC_CH_RF_I:
        case BSP_ADC_CH_ESW_I:
            return ProcessCurrent(ch);
        
        /* VOUT特殊处理 */
        case BSP_ADC_CH_VOUT:
            return ProcessVOUT(ch);
        
        /* 其他电压通道直接返回电压值 */
        case BSP_ADC_CH_ESW_U:
        case BSP_ADC_CH_HP_PRE:
        case BSP_ADC_CH_Heat_REF02:
        default:
            return DRV_ADC_ReadVoltage(ch);
    }
}

/* ==================== 旧版兼容接口实现 ==================== */

/**
 * @brief  旧枚举到新枚举的映射
 */
static BSP_ADC_Channel_t MapOldToNewChannel(ADC_Channel_EnumDef oldCh)
{
    switch (oldCh) {
        case E_ADC_CHANNEL_US_I:       return BSP_ADC_CH_US_I;
        case E_ADC_CHANNEL_RF_I:       return BSP_ADC_CH_RF_I;
        case E_ADC_CHANNEL_Heat_REF02: return BSP_ADC_CH_Heat_REF02;
        case E_ADC_CHANNEL_Heat_REF01: return BSP_ADC_CH_Heat_REF01;
        case E_ADC_CHANNEL_ESW_U:      return BSP_ADC_CH_ESW_U;
        case E_ADC_CHANNEL_ESW_I:      return BSP_ADC_CH_ESW_I;
        case E_ADC_CHANNEL_HP_PRE:     return BSP_ADC_CH_HP_PRE;
        case E_ADC_CHANNEL_HAND_NTC:   return BSP_ADC_CH_HAND_NTC;
        case E_ADC_CHANNEL_VOUT:       return BSP_ADC_CH_VOUT;
        case E_ADC_CHANNEL_VER_ID:     return BSP_ADC_CH_MAX; /* 无效通道 */
        default:                       return BSP_ADC_CH_MAX;
    }
}

uint16_t Drv_ADC_ReadChannel(ADC_Channel_EnumDef channel)
{
    if (channel == E_ADC_CHANNEL_VER_ID)
        return 0;
    
    BSP_ADC_Channel_t newCh = MapOldToNewChannel(channel);
    return DRV_ADC_ReadRaw(newCh);
}

uint32_t Drv_ADC_ReadVoltage(ADC_Channel_EnumDef channel)
{
    if (channel == E_ADC_CHANNEL_VER_ID)
        return 0;
    
    BSP_ADC_Channel_t newCh = MapOldToNewChannel(channel);
    return (uint32_t)DRV_ADC_ReadVoltage(newCh);
}

uint16_t Drv_ADC_ReadVOUT(void)
{
    return DRV_ADC_ReadVoltage(BSP_ADC_CH_VOUT);
}

uint16_t Drv_GetADCVout(void)
{
    return ProcessVOUT(BSP_ADC_CH_VOUT);
}

uint16_t Drv_ADC_GetRealValue(ADC_Channel_EnumDef channel)
{
    if (channel == E_ADC_CHANNEL_VER_ID)
        return 0;
    
    BSP_ADC_Channel_t newCh = MapOldToNewChannel(channel);
    return DRV_ADC_GetProcessedValue(newCh);
}

uint16_t Drv_ADC_GetNTCValue(NTC_Type_EnumDef ntcType)
{
    BSP_ADC_Channel_t ch;
    
    switch (ntcType) {
        case E_NTC_HAND:
            ch = BSP_ADC_CH_HAND_NTC;
            break;
        case E_NTC_MAIN:
            ch = BSP_ADC_CH_Heat_REF01;
            break;
        default:
            return 0;
    }
    
    return ProcessNTC(ch);
}
