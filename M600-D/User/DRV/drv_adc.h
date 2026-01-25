/************************************************************************************
 * @file     : drv_adc.h
 * @brief    : ADC driver - DRV API, DAL calls BSP (Std lib)
 ***********************************************************************************/
#ifndef DRV_ADC_H
#define DRV_ADC_H

#include "stm32f10x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    E_ADC_CHANNEL_US_I = 0,
    E_ADC_CHANNEL_RF_I,
    E_ADC_CHANNEL_Heat_REF02,
    E_ADC_CHANNEL_Heat_REF01,
    E_ADC_CHANNEL_ESW_U,
    E_ADC_CHANNEL_ESW_I,
    E_ADC_CHANNEL_HP_PRE,
    E_ADC_CHANNEL_HAND_NTC,
    E_ADC_CHANNEL_VER_ID,   /* BSP has no; DAL returns 0 */
    E_ADC_CHANNEL_VOUT,     /* BSP has no; DAL returns 0 */
    E_ADC_CHANNEL_MAX
} ADC_Channel_EnumDef;

uint16_t Drv_ADC_ReadChannel(ADC_Channel_EnumDef channel);
uint32_t Drv_ADC_ReadVoltage(ADC_Channel_EnumDef channel);
uint16_t Drv_ADC_ReadVOUT(void);
uint16_t Drv_ADC_GetRealValue(ADC_Channel_EnumDef channel);

#ifdef __cplusplus
}
#endif

#endif /* DRV_ADC_H */
