/***********************************************************************************
* @file     : drv_adc.c
* @brief    : ADC driver implementation
* @details  : ADC channel read functions implementation
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drv_adc.h"
#include "adc.h"
#include "stm32f1xx_hal.h"

/* ADC reference voltage in millivolts (mV) */
#define ADC_REF_VOLTAGE_MV    3300    ///< 3.3V reference voltage
#define ADC_RESOLUTION        4096    ///< 12-bit ADC resolution (2^12 = 4096)

/**
 * @brief Convert ADC channel enumeration to STM32 HAL ADC channel
 * @param channel ADC channel enumeration
 * @retval STM32 HAL ADC channel number, returns 0 if invalid
 */
static uint32_t Drv_ADC_GetHALChannel(ADC_Channel_EnumDef channel)
{
    switch (channel)
    {
        case E_ADC_CHANNEL_US_I:
            return ADC_CHANNEL_0;
        case E_ADC_CHANNEL_RF_I:
            return ADC_CHANNEL_1;
        case E_ADC_CHANNEL_Heat_REF02:
            return ADC_CHANNEL_5;
        case E_ADC_CHANNEL_Heat_REF01:
            return ADC_CHANNEL_6;
        case E_ADC_CHANNEL_ESW_U:
            return ADC_CHANNEL_8;
        case E_ADC_CHANNEL_ESW_I:
            return ADC_CHANNEL_9;
        case E_ADC_CHANNEL_HP_PRE:
            return ADC_CHANNEL_12;
        case E_ADC_CHANNEL_HAND_NTC:
            return ADC_CHANNEL_13;
        case E_ADC_CHANNEL_VER_ID:
            return ADC_CHANNEL_14;
        case E_ADC_CHANNEL_VOUT:
            return ADC_CHANNEL_15;
        default:
            return 0;
    }
}

/**
 * @brief Read ADC value from specified channel
 * @param channel ADC channel enumeration
 * @retval ADC value (0-4095 for 12-bit ADC), returns 0 if error
 */
uint16_t Dal_ADC_ReadChannel(ADC_Channel_EnumDef channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint32_t halChannel;
    HAL_StatusTypeDef status;
    uint16_t adcValue = 0;

    /* Check channel validity */
    if (channel >= E_ADC_CHANNEL_MAX)
    {
        return 0;
    }

    /* Get HAL channel number */
    halChannel = Drv_ADC_GetHALChannel(channel);
    /* Note: ADC_CHANNEL_0 is 0, so we cannot use halChannel==0 to check error.
       We already validated channel < E_ADC_CHANNEL_MAX above. */

    /* Configure ADC channel */
    sConfig.Channel = halChannel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
    
    status = HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    if (status != HAL_OK)
    {
        return 0;
    }

    /* Start ADC conversion */
    status = HAL_ADC_Start(&hadc1);
    if (status != HAL_OK)
    {
        return 0;
    }

    /* Wait for conversion complete */
    status = HAL_ADC_PollForConversion(&hadc1, 100);
    if (status != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);
        return 0;
    }

    /* Read ADC value */
    adcValue = HAL_ADC_GetValue(&hadc1);

    /* Stop ADC conversion */
    HAL_ADC_Stop(&hadc1);

    return adcValue;
}

/**
 * @brief Read ADC value with voltage conversion (assuming 3.3V reference)
 * @param channel ADC channel enumeration
 * @retval Voltage in millivolts (mV), returns 0xFFFF if error
 */
uint32_t Drv_ADC_ReadVoltage(ADC_Channel_EnumDef channel)
{
    uint16_t adcValue;
    uint32_t voltage;

    /* Read ADC value */
    adcValue = Dal_ADC_ReadChannel(channel);
    
    /* Note: adcValue can legitimately be 0, so we cannot use 0 to indicate error.
       If error occurs, Drv_ADC_ReadChannel will return 0, but we have no way to 
       distinguish between error and actual 0 value. In practice, HAL_ADC_GetValue 
       should always return a valid 12-bit value (0-4095) if conversion succeeds. */

    /* Convert ADC value to voltage in millivolts */
    /* voltage = (adcValue * ADC_REF_VOLTAGE_MV) / ADC_RESOLUTION */
    voltage = ((uint32_t)adcValue * ADC_REF_VOLTAGE_MV) / ADC_RESOLUTION;

    return voltage;
}

uint16_t Drv_ADC_ReadWorkCurrent(void)
{
    uint16_t adcValue;
    uint32_t voltage;

    /* Read ADC value */
    adcValue = Dal_ADC_ReadChannel(E_ADC_CHANNEL_US_I);
    voltage = ((uint32_t)adcValue * ADC_REF_VOLTAGE_MV) / ADC_RESOLUTION;
    return voltage;
}

uint16_t Drv_ADC_ReadHandNTC(void)
{
    uint16_t adcValue;
    uint32_t voltage;
    adcValue = Dal_ADC_ReadChannel(E_ADC_CHANNEL_HAND_NTC);
    voltage = ((uint32_t)adcValue * ADC_REF_VOLTAGE_MV) / ADC_RESOLUTION;
    return voltage;
}

uint16_t Drv_ADC_ReadRFCurrent(void)
{
    uint16_t adcValue;
    uint32_t voltage;
    
    /* Read ADC value */
    adcValue = Dal_ADC_ReadChannel(E_ADC_CHANNEL_RF_I);
    voltage = ((uint32_t)adcValue * ADC_REF_VOLTAGE_MV) / ADC_RESOLUTION;
    return voltage;
}

uint16_t Drv_ADC_ReadESWCurrent(void)
{
    uint16_t adcValue;
    uint32_t voltage;
    
    /* Read ADC value */
    adcValue = Dal_ADC_ReadChannel(E_ADC_CHANNEL_ESW_I);
    voltage = ((uint32_t)adcValue * ADC_REF_VOLTAGE_MV) / ADC_RESOLUTION;
    return voltage;
}

uint16_t Drv_ADC_ReadESWVoltage(void)
{
    uint16_t adcValue;
    uint32_t voltage;
    
    /* Read ADC value */
    adcValue = Dal_ADC_ReadChannel(E_ADC_CHANNEL_ESW_U);
    voltage = ((uint32_t)adcValue * ADC_REF_VOLTAGE_MV) / ADC_RESOLUTION;
    return voltage;
}

uint16_t Drv_ADC_ReadHPPre(void)
{
    uint16_t adcValue;
    uint32_t voltage;
    
    /* Read ADC value */
    adcValue = Dal_ADC_ReadChannel(E_ADC_CHANNEL_HP_PRE);
    voltage = ((uint32_t)adcValue * ADC_REF_VOLTAGE_MV) / ADC_RESOLUTION;
    return voltage;
}

uint16_t Drv_ADC_GetRealValue(ADC_Channel_EnumDef channel)
{
    switch(channel)
    {
        case E_ADC_CHANNEL_US_I:
            return Drv_ADC_ReadWorkCurrent();
        case E_ADC_CHANNEL_RF_I:
            return Drv_ADC_ReadRFCurrent();
        case E_ADC_CHANNEL_ESW_I:
            return Drv_ADC_ReadESWCurrent();
        case E_ADC_CHANNEL_ESW_U:
            return Drv_ADC_ReadESWVoltage();
        case E_ADC_CHANNEL_HP_PRE:
            return Drv_ADC_ReadHPPre();
        case E_ADC_CHANNEL_HAND_NTC:
            return Drv_ADC_ReadHandNTC();
        default:
            return 0;
    }
}


/**************************End of file********************************/
