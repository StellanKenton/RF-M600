/************************************************************************************
 * @file     : drv_dac.c
 * @brief    : DAC driver - DRV calls DAL, DAL calls BSP (Std lib)
 ***********************************************************************************/
#include "drv_dac.h"
#include "drv_adc.h"
#include "bsp_dac.h"

#define DAC_REF_MV      3300u
#define DAC_RESOLUTION  4096u
#define DAC_VOLTAGE_TOLERANCE_MV  50u

static uint16_t s_currentVoltage = 0;

/* DAL_DAC_Init: only called from DRV; calls BSP */
static void Dal_DAC_Init(void)
{
    BSP_DAC_Init();
}

void Drv_DAC_Init(void)
{
    Dal_DAC_Init();
    s_currentVoltage = 0;
}

bool Drv_DAC_SetVoltage(uint16_t voltage_mv)
{
    if (voltage_mv > DAC_REF_MV)
        voltage_mv = DAC_REF_MV;
    BSP_DAC_SetVoltage(voltage_mv);
    s_currentVoltage = voltage_mv;
    return true;
}

uint16_t Drv_DAC_GetVoltage(void)
{
    return s_currentVoltage;
}

uint16_t Drv_DAC_GetActualVoltage(void)
{
    return Drv_ADC_ReadVOUT();
}
