/************************************************************************************
* @file     : drv_si5351.h
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRV_SI5351_H
#define DRV_SI5351_H

#include <string.h>
#include <stdbool.h>
#include "stdint.h"

#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif


void Drv_SI5351_Init(void);
uint16_t Drv_SI5351_SetFrequency(uint16_t frequency);
uint16_t Drv_SI5351_SetPulseWidthus(uint16_t pulse_width_us);


#ifdef __cplusplus
}
#endif
#endif  // DRV_SI5351_H
/**************************End of file********************************/

