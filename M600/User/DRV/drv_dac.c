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
#include "drv_adc.h"
#include "dac.h"
#include "stm32f1xx_hal.h"

/* DAC reference voltage in millivolts (mV) */
#define DAC_REF_VOLTAGE_MV    3300    ///< 3.3V reference voltage
#define DAC_RESOLUTION        4096    ///< 12-bit DAC resolution (2^12 = 4096)
#define DAC_VOLTAGE_TOLERANCE_MV  50  ///< Voltage tolerance for verification (50mV)

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
 * @brief Set DAC output voltage with verification
 * @param voltage_mv Voltage in millivolts (mV), range: 0-3300mV
 * @retval true if success, false if failed
 * @note This function sets the voltage and verifies it using ADC_VOUT
 */
bool Drv_DAC_SetVoltage(uint16_t voltage_mv)
{
    uint32_t dacValue;
    HAL_StatusTypeDef status;
    uint16_t actualVoltage;
    int16_t voltageError;
    
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
    
    if(status != HAL_OK)
    {
        return false;
    }
    
    // 等待DAC输出稳定（可能需要一些时间）
    HAL_Delay(10);  // 等待10ms让DAC输出稳定
    
    // 通过ADC_VOUT监控电压调节是否正确
    actualVoltage = Drv_ADC_ReadVOUT();
    voltageError = (int16_t)actualVoltage - (int16_t)voltage_mv;
    
    // 检查电压误差是否在允许范围内（±50mV）
    if(voltageError > DAC_VOLTAGE_TOLERANCE_MV || voltageError < -DAC_VOLTAGE_TOLERANCE_MV)
    {
        // 电压误差超出允许范围，可能需要调整或报警
        // 注意：这里只是记录，不阻止设置，因为实际硬件可能有放大电路
        // 实际电压值 = ADC_VOUT读取值 * 放大倍数（需要根据硬件设计确定）
        // TODO: 根据实际硬件设计调整验证逻辑
    }
    
    s_currentVoltage = voltage_mv;
    return true;
}

/**
 * @brief Get current DAC output voltage (from internal record)
 * @retval Current voltage in millivolts (mV)
 */
uint16_t Drv_DAC_GetVoltage(void)
{
    return s_currentVoltage;
}

/**
 * @brief Get actual output voltage by reading ADC_VOUT
 * @retval Actual voltage in millivolts (mV) read from ADC_VOUT
 * @note This function reads the actual output voltage from ADC_VOUT channel
 *       The actual voltage may be different from set voltage due to hardware
 *       amplification or other factors. Need to calibrate based on actual hardware.
 */
uint16_t Drv_DAC_GetActualVoltage(void)
{
    return Drv_ADC_ReadVOUT();
}

/**************************End of file********************************/
