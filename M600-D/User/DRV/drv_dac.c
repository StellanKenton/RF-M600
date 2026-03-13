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

/* DC-DC: centivolt 600~2000 (6.00V~20.00V) maps to DAC code; divider span 1400 */
#define DCDC_CENTIVOLT_SPAN 1400u
#define DCDC_CENTIVOLT_OFFSET 600u

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

void Drv_DAC_SetVoltage(uint16_t centivolt)
{
    if (centivolt < DRV_DAC_DCDC_CENTIVOLT_MIN || centivolt > DRV_DAC_DCDC_CENTIVOLT_MAX)
        centivolt = DRV_DAC_DCDC_CENTIVOLT_DEFAULT;
    float voltage = centivolt / 100.0f;
    float dacVoltage = (45.6975f - voltage) / 12.1084f;  /* derived from actual voltage measurements */
    uint32_t code = (uint32_t)(dacVoltage/3.3f * (DAC_RESOLUTION - 1u) + 0.5f);  /* round to nearest */
    if (code > DAC_RESOLUTION - 1u)
        code = DAC_RESOLUTION - 1u;
    BSP_DAC_SetValue((uint16_t)code);
    s_currentVoltage = (uint16_t)((45.6975f - 12.1084f * dacVoltage)*100);
}

uint16_t Drv_DAC_GetVoltage(void)
{
    uint16_t raw = Drv_ADC_ReadVoutRaw();
    /* actual_voltage * 100 = (11 * 3.3V_ref * raw) / 4096 => (11*330*raw)/4096, 11:1 divider */
    return (uint16_t)((uint32_t)11u * 330u * raw / DAC_RESOLUTION);
}

uint16_t Drv_DAC_GetDCDCVoltageCentivolt(void)
{
    return s_currentVoltage;
}
