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




/**************************End of file********************************/


