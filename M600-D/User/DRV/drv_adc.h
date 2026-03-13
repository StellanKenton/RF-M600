/************************************************************************************
 * @file     : drv_adc.h
 * @brief    : ADC driver - DRV API, DAL calls BSP (Std lib)
 ***********************************************************************************/
#ifndef DRV_ADC_H
#define DRV_ADC_H

#include "bsp_adc.h"
#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    E_NTC_HAND = 0,   /* 换能�?/手柄NTC (transducer IGBT temp) - ADC13 */
    E_NTC_MAIN,       /* 电路板NTC (circuit board temp) - ADC6 Heat_REF01 */
    E_NTC_MAX
} NTC_Type_EnumDef;

typedef struct {
    uint16_t usCurrent;
    uint16_t rfCurrent;
    uint16_t heatRef02;
    uint16_t heatRef01;
    uint16_t eswVoltage;
    uint16_t eswCurrent;
    uint16_t hpPressure;
    uint16_t handNTC;
    uint16_t verId;
    uint16_t vout;
    uint32_t updateTickMs;
    bool  isContactSkin;
} Drv_ADC_PhysicalValues_t;

void Drv_ADC_Init(void);
void Drv_ADC_Process(void);
uint16_t Drv_ADC_ReadChannel(BSP_ADC_Channel_t channel);
uint16_t Drv_ADC_GetRealValue(BSP_ADC_Channel_t channel);
uint16_t Drv_ADC_ReadVoutRaw(void);
uint16_t Drv_ADC_GetVoutRealValue(void);
const Drv_ADC_PhysicalValues_t *Drv_ADC_GetPhysicalValues(void);
void Drv_ADC_SetTempOverride(char *data);

#ifdef __cplusplus
}
#endif

#endif /* DRV_ADC_H */
