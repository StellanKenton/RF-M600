/************************************************************************************
 * @file     : drv_temp_module.h
 * @brief    : GY-MCU90614 temperature module driver on USART2 (PA2/PA3)
 ***********************************************************************************/
#ifndef DRV_TEMP_MODULE_H
#define DRV_TEMP_MODULE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t object_temp_centi_c;
    int16_t ambient_temp_centi_c;
} Drv_TempModule_Data_t;

void Drv_TempModule_Init(void);
void Drv_TempModule_Process(void);

bool Drv_TempModule_GetLatest(Drv_TempModule_Data_t *pOut);

#ifdef __cplusplus
}
#endif

#endif /* DRV_TEMP_MODULE_H */
