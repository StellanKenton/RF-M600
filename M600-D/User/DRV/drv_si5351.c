/***********************************************************************************
* @file     : drv_si5351.c
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drv_si5351.h"
#include "bsp_i2c.h"
#include "bsp_SI5351.h"
#include "bsp_tim.h"
#include "drv_soft_i2c.h"

#define SI5351_I2C_INSTANCE    DRV_SOFT_I2C_INSTANCE_1  /* Use I2C instance 1 */

Drv_SoftI2C_Config_t i2c_config = {
    .SCL_Port = GPIOB,
    .SCL_Pin = GPIO_Pin_6,
    .SDA_Port = GPIOB,
    .SDA_Pin = GPIO_Pin_7
};

void Drv_SI5351_Init(void)
{
    // Initialize the SI5351
    Drv_SoftI2C_Init(SI5351_I2C_INSTANCE, &i2c_config);
    PWM_Generate_Config();
}

uint16_t Drv_SI5351_SetFrequency(uint16_t frequency)
{
    // Set the frequency of the SI5351
    if(frequency <= 700)
    {
        frequency = 0;
    }
    else if(frequency > 1400)
    {
        frequency = 1400;
    }
    // Set the frequency of the SI5351
    PWM_Generate(frequency);
    return frequency;
}

uint16_t Drv_SI5351_SetPulseWidthus(uint16_t pulse_width_us)
{
    // Set the pulse width of the SI5351 
    // input 0.5ms value to 5, 20ms value to 200
    if(pulse_width_us < 500)
    {
        pulse_width_us = 0;
    }
    else if(pulse_width_us > 20000)
    {
        pulse_width_us = 20000;
    }
    // Set the pulse width of the SI5351
    pulse_width_us = (pulse_width_us/500)*500;

    // BSP Control Pulse Width Register
    return pulse_width_us;
}

/**
 * @brief Set SI5351 to output complementary PWM signals with dead time
 * @param frequency_khz Frequency in kHz (for RF: 1000kHz = 1MHz)
 * @param dead_time_ns Dead time in nanoseconds
 * @note This function configures SI5351 to output complementary PWM signals
 *       with dead time for RF module. The actual implementation depends on
 *       hardware design - SI5351 may generate clock signals that are then
 *       used by MCU timers to generate complementary PWM, or SI5351 may
 *       have additional logic to generate PWM directly.
 */
void Drv_SI5351_SetComplementaryPWM(bool enable)
{
    if(enable)
    {
        BSP_TIM1_ComplementaryPWM_Enable();
    }
    else
    {
        BSP_TIM1_ComplementaryPWM_Disable();
    }
}

/**************************End of file********************************/


