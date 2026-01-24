/***********************************************************************************
* @file     : drv_dac.c
* @brief    : DAC driver implementation
* @details  : DAC voltage output control functions implementation
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drv_dac.h"
#include "dac.h"
#include "stm32f1xx_hal.h"

/* DAC reference voltage in millivolts (mV) */
#define DAC_REF_VOLTAGE_MV    3300    ///< 3.3V reference voltage
#define DAC_RESOLUTION        4096    ///< 12-bit DAC resolution (2^12 = 4096)

static uint16_t s_currentVoltage = 0;  ///< Current DAC output voltage in mV

/**
 * @brief Initialize DAC driver
 */
void Drv_DAC_Init(void)
{
    // DAC is already initialized by MX_DAC_Init()
    // Start DAC channel 1
    if(HAL_DAC_Start(&hdac, DAC_CHANNEL_1) != HAL_OK)
    {
        // Error handling
    }
    s_currentVoltage = 0;
}

/**
 * @brief Set DAC output voltage
 * @param voltage_mv Voltage in millivolts (mV), range: 0-3300mV
 * @retval true if success, false if failed
 */
bool Drv_DAC_SetVoltage(uint16_t voltage_mv)
{
    uint32_t dacValue;
    HAL_StatusTypeDef status;
    
    // Limit voltage to valid range
    if(voltage_mv > DAC_REF_VOLTAGE_MV)
    {
        voltage_mv = DAC_REF_VOLTAGE_MV;
    }
    
    // Convert voltage to DAC value
    // DAC_OUT = VREF+ * DOR / 4095
    // DOR = (DAC_OUT * 4095) / VREF+
    dacValue = ((uint32_t)voltage_mv * DAC_RESOLUTION) / DAC_REF_VOLTAGE_MV;
    
    // Set DAC value (12-bit right alignment)
    status = HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dacValue);
    
    if(status == HAL_OK)
    {
        s_currentVoltage = voltage_mv;
        return true;
    }
    
    return false;
}

/**
 * @brief Get current DAC output voltage
 * @retval Current voltage in millivolts (mV)
 */
uint16_t Drv_DAC_GetVoltage(void)
{
    return s_currentVoltage;
}

/**************************End of file********************************/
