/************************************************************************************
 * @file     : drv_adc.c
 * @brief    : ADC driver - DRV calls DAL, DAL calls BSP (Std lib)
 ***********************************************************************************/
#include "drv_adc.h"
#include "bsp_adc.h"

#define ADC_REF_MV      3300u
#define ADC_RESOLUTION  4096u

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
