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

void Drv_SI5351_Init(void)
{
    // Initialize the SI5351
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
void Drv_SI5351_SetComplementaryPWM(uint16_t frequency_khz, uint16_t dead_time_ns)
{
    // TODO: Implement I2C communication with SI5351 to configure:
    // 1. Set output frequency to frequency_khz (for RF: 1000kHz = 1MHz)
    // 2. Configure complementary outputs (CLK0 and CLK1 or CLK2)
    // 3. Set dead time between complementary signals
    // 
    // Example I2C register configuration:
    // - SI5351_CLK0_CTRL: Configure CLK0 output
    // - SI5351_CLK1_CTRL: Configure CLK1 output (complementary)
    // - SI5351_CLK2_CTRL: Configure CLK2 output if needed
    // - Calculate PLL and divider values for desired frequency
    // - Configure phase offset for dead time
    
    // Placeholder: Set frequency (this may need to be called separately)
    Drv_SI5351_SetFrequency(frequency_khz);
    
    /* TODO: DAL -> BSP_I2C1_Transmit(addr, buf, len) for SI5351 I2C config */
}

/**************************End of file********************************/


