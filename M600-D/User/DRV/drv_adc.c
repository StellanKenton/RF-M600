/************************************************************************************
 * @file     : drv_adc.c
 * @brief    : ADC driver - DRV calls DAL, DAL calls BSP (Std lib)
 ***********************************************************************************/
#include "drv_adc.h"
#include "bsp_adc.h"

#define ADC_REF_MV      3300u
#define ADC_RESOLUTION  4096u
#define NTC_SERIES_R    10000u  /* series R with NTC, ohm */
#define NTC_RAW_OPEN    3900u   /* ADC raw > this: NTC open */
#define NTC_RAW_SHORT   50u     /* ADC raw < this: NTC short */

/* NTC 10R table: -40~105C, per 1C, from spec (10R NTC) */
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

static ADC_Channel_EnumDef Dal_NTC_MapChannel(NTC_Type_EnumDef ntcType)
{
    switch (ntcType) {
        case E_NTC_HAND: return E_ADC_CHANNEL_HAND_NTC;
        case E_NTC_MAIN: return E_ADC_CHANNEL_Heat_REF01;
        default:         return E_ADC_CHANNEL_MAX;
    }
}

/* Convert NTC ADC raw to temp: return (temp+40)*10, -40C=0, 0C=400, 105C=1450 */
static uint16_t Dal_NTC_ADCToTemp(uint32_t adcRaw)
{
    uint32_t tempMv = ((uint32_t)ADC_REF_MV * adcRaw) / ADC_RESOLUTION;
    uint32_t vDiff = ADC_REF_MV - tempMv;
    if (vDiff == 0u)
        return (uint16_t)((NTC_TABLE_SIZE - 1) * 10u);
    uint32_t rOhm = (tempMv * NTC_SERIES_R) / vDiff;
    uint32_t r10 = rOhm / 10u;

    uint16_t numN = 0, numP;
    for (uint16_t i = 0; i < NTC_TABLE_SIZE; i++) {
        if (r10 > s_ntc_temp_table[i]) {
            numP = i;
            if (numP == 0)
                return 0u;
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

static BSP_ADC_Channel_t Dal_ADC_MapChannel(ADC_Channel_EnumDef ch)
{
    switch (ch) {
        case E_ADC_CHANNEL_US_I:      return BSP_ADC_CH_US_I;
        case E_ADC_CHANNEL_RF_I:      return BSP_ADC_CH_RF_I;
        case E_ADC_CHANNEL_Heat_REF02: return BSP_ADC_CH_Heat_REF02;
        case E_ADC_CHANNEL_Heat_REF01: return BSP_ADC_CH_Heat_REF01;
        case E_ADC_CHANNEL_ESW_U:     return BSP_ADC_CH_ESW_U;
        case E_ADC_CHANNEL_ESW_I:     return BSP_ADC_CH_ESW_I;
        case E_ADC_CHANNEL_HP_PRE:    return BSP_ADC_CH_HP_PRE;
        case E_ADC_CHANNEL_HAND_NTC:  return BSP_ADC_CH_HAND_NTC;
        default:                      return BSP_ADC_CH_MAX;
    }
}

/* DAL: only called from DRV; calls BSP */
static uint16_t Dal_ADC_ReadChannel(ADC_Channel_EnumDef channel)
{
    if (channel >= E_ADC_CHANNEL_MAX)
        return 0;
    if (channel == E_ADC_CHANNEL_VER_ID || channel == E_ADC_CHANNEL_VOUT)
        return 0;
    BSP_ADC_Channel_t bch = Dal_ADC_MapChannel(channel);
    return BSP_ADC_ReadChannel(bch);
}

uint16_t Drv_ADC_ReadChannel(ADC_Channel_EnumDef channel)
{
    return Dal_ADC_ReadChannel(channel);
}

uint32_t Drv_ADC_ReadVoltage(ADC_Channel_EnumDef channel)
{
    uint16_t raw = Dal_ADC_ReadChannel(channel);
    return ((uint32_t)raw * ADC_REF_MV) / ADC_RESOLUTION;
}

uint16_t Drv_ADC_ReadVOUT(void)
{
    uint16_t raw = Dal_ADC_ReadChannel(E_ADC_CHANNEL_VOUT);
    return (uint16_t)(((uint32_t)raw * ADC_REF_MV) / ADC_RESOLUTION);
}

uint16_t Drv_ADC_ReadWorkCurrent(void)
{
    uint16_t raw = Dal_ADC_ReadChannel(E_ADC_CHANNEL_US_I);
    return (uint16_t)(((uint32_t)raw * ADC_REF_MV) / ADC_RESOLUTION);
}

uint16_t Drv_ADC_ReadHandNTC(void)
{
    uint16_t raw = Dal_ADC_ReadChannel(E_ADC_CHANNEL_HAND_NTC);
    return (uint16_t)(((uint32_t)raw * ADC_REF_MV) / ADC_RESOLUTION);
}

uint16_t Drv_ADC_ReadRFCurrent(void)
{
    uint16_t raw = Dal_ADC_ReadChannel(E_ADC_CHANNEL_RF_I);
    return (uint16_t)(((uint32_t)raw * ADC_REF_MV) / ADC_RESOLUTION);
}

uint16_t Drv_ADC_ReadESWCurrent(void)
{
    uint16_t raw = Dal_ADC_ReadChannel(E_ADC_CHANNEL_ESW_I);
    return (uint16_t)(((uint32_t)raw * ADC_REF_MV) / ADC_RESOLUTION);
}

uint16_t Drv_ADC_ReadESWVoltage(void)
{
    uint16_t raw = Dal_ADC_ReadChannel(E_ADC_CHANNEL_ESW_U);
    return (uint16_t)(((uint32_t)raw * ADC_REF_MV) / ADC_RESOLUTION);
}

uint16_t Drv_ADC_ReadHPPre(void)
{
    uint16_t raw = Dal_ADC_ReadChannel(E_ADC_CHANNEL_HP_PRE);
    return (uint16_t)(((uint32_t)raw * ADC_REF_MV) / ADC_RESOLUTION);
}

uint16_t Drv_ADC_GetRealValue(ADC_Channel_EnumDef channel)
{
    switch (channel) {
        case E_ADC_CHANNEL_US_I:     return Drv_ADC_ReadWorkCurrent();
        case E_ADC_CHANNEL_RF_I:     return Drv_ADC_ReadRFCurrent();
        case E_ADC_CHANNEL_ESW_I:    return Drv_ADC_ReadESWCurrent();
        case E_ADC_CHANNEL_ESW_U:    return Drv_ADC_ReadESWVoltage();
        case E_ADC_CHANNEL_HP_PRE:   return Drv_ADC_ReadHPPre();
        case E_ADC_CHANNEL_VOUT:     return Drv_ADC_ReadVOUT();
        case E_ADC_CHANNEL_HAND_NTC: return Drv_ADC_ReadHandNTC();
        default:                     return 0;
    }
}

uint16_t Drv_ADC_GetNTCValue(NTC_Type_EnumDef ntcType)
{
    if (ntcType >= E_NTC_MAX)
        return 0;
    ADC_Channel_EnumDef ch = Dal_NTC_MapChannel(ntcType);
    uint16_t raw = Dal_ADC_ReadChannel(ch);
    if (raw > NTC_RAW_OPEN)
        return NTC_FAULT_OPEN;
    if (raw < NTC_RAW_SHORT)
        return NTC_FAULT_SHORT;
    return Dal_NTC_ADCToTemp(raw);
}
