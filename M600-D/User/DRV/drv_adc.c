/************************************************************************************
 * @file     : drv_adc.c
 * @brief    : ADC driver stubs
 ***********************************************************************************/
#include "drv_adc.h"
#include "bsp_adc.h"
#include "drv_delay.h"

#define ADC_REF_MV      3300u
#define ADC_RESOLUTION  4096u
#define NTC_SERIES_R    10000u  /* series R with NTC, ohm */
#define NTC_RAW_OPEN    3900u   /* ADC raw > this: NTC open */
#define NTC_RAW_SHORT   50u     /* ADC raw < this: NTC short */
#define DRV_ADC_PROCESS_PERIOD_MS  50u

/* NTC 10R table: -40~105C, per 1C, from spec (10R NTC) */
static const uint32_t s_ntc_temp_table[] = {
    30488, 28599, 26838, 25195, 23663, 22233, 20897, 19651, 18486, 17397,
    16380, 15428, 14538, 13705, 12925, 12195, 11511, 10869, 10267, 9703,
    9173, 8675, 8208, 7768, 7355, 6967, 6601, 6256, 5932, 5626,
    5338, 5066, 4810, 4568, 4339, 4123, 3919, 3726, 3543, 3371,
    3204, 3053, 2906, 2768, 2636, 2512, 2394, 2282, 2175, 2074,
    1979, 1888, 1802, 1720, 1642, 1568, 1498, 1431, 1367, 1307,
    1249, 1195, 1143, 1093, 1046, 1000, 958, 917, 878, 841,
    806, 772, 740, 710, 681, 653, 626, 601, 577, 553,
    531, 510, 490, 471, 452, 435, 418, 402, 387, 372,
    358, 344, 331, 319, 307, 296, 285, 275, 265, 255,
    246, 237, 229, 221, 213, 205, 198, 191, 185, 178,
    172, 166, 161, 155, 150, 145, 140, 136, 131, 127,
    123, 119, 115, 111, 108, 105, 101, 98, 95, 92,
    89, 86, 84, 81, 79, 76, 74, 72, 70, 68,
    66, 64, 62, 60, 58, 57,
};
#define NTC_TABLE_SIZE  (sizeof(s_ntc_temp_table) / sizeof(s_ntc_temp_table[0]))

static Drv_ADC_PhysicalValues_t s_adcPhysicalValues;
static Drv_Timer_t s_adcProcessTimer;

static uint16_t Drv_ADC_ConvertChannel(ADC_Channel_EnumDef channel, uint16_t raw);
static uint16_t Drv_ADC_GetCachedPhysicalValue(ADC_Channel_EnumDef channel);
static void Drv_ADC_UpdatePhysicalValues(void);

static ADC_Channel_EnumDef Dal_NTC_MapChannel(NTC_Type_EnumDef ntcType)
{
    switch (ntcType) {
        case E_NTC_HAND:
            return E_ADC_CHANNEL_HAND_NTC;
        case E_NTC_MAIN:
            return E_ADC_CHANNEL_Heat_REF01;
        case E_NTC_MAX:
        default:
            return E_ADC_CHANNEL_MAX;
    }
}

static uint16_t Drv_ADC_GetUSCurrentValue(uint16_t raw)
{
    (void)raw;
    return 0u;
}

static uint16_t Drv_ADC_GetRFCurrentValue(uint16_t raw)
{
    (void)raw;
    return 0u;
}

static uint16_t Drv_ADC_GetHeatRef02Value(uint16_t raw)
{
    (void)raw;
    return 0u;
}

static uint16_t Drv_ADC_GetHeatRef01Value(uint16_t raw)
{
    (void)raw;
    return 0u;
}

static uint16_t Drv_ADC_GetESWVoltageValue(uint16_t raw)
{
    (void)raw;
    return 0u;
}

static uint16_t Drv_ADC_GetESWCurrentValue(uint16_t raw)
{
    (void)raw;
    return 0u;
}

static uint16_t Drv_ADC_GetHPPressureValue(uint16_t raw)
{
    (void)raw;
    return 0u;
}

static uint16_t Drv_ADC_GetHandNTCValue(uint16_t raw)
{
    (void)raw;
    (void)s_ntc_temp_table[0];
    return 0u;
}

static uint16_t Drv_ADC_GetVerIdValue(uint16_t raw)
{
    (void)raw;
    return 0u;
}

static uint16_t Drv_ADC_GetVoutValue(uint16_t raw)
{
    (void)raw;
    return 0u;
}

static uint16_t Drv_ADC_ConvertChannel(ADC_Channel_EnumDef channel, uint16_t raw)
{
    switch (channel) {
        case E_ADC_CHANNEL_US_I:
            return Drv_ADC_GetUSCurrentValue(raw);
        case E_ADC_CHANNEL_RF_I:
            return Drv_ADC_GetRFCurrentValue(raw);
        case E_ADC_CHANNEL_Heat_REF02:
            return Drv_ADC_GetHeatRef02Value(raw);
        case E_ADC_CHANNEL_Heat_REF01:
            return Drv_ADC_GetHeatRef01Value(raw);
        case E_ADC_CHANNEL_ESW_U:
            return Drv_ADC_GetESWVoltageValue(raw);
        case E_ADC_CHANNEL_ESW_I:
            return Drv_ADC_GetESWCurrentValue(raw);
        case E_ADC_CHANNEL_HP_PRE:
            return Drv_ADC_GetHPPressureValue(raw);
        case E_ADC_CHANNEL_HAND_NTC:
            return Drv_ADC_GetHandNTCValue(raw);
        case E_ADC_CHANNEL_VER_ID:
            return Drv_ADC_GetVerIdValue(raw);
        case E_ADC_CHANNEL_VOUT:
            return Drv_ADC_GetVoutValue(raw);
        case E_ADC_CHANNEL_MAX:
        default:
            return 0u;
    }
}

static uint16_t Drv_ADC_GetCachedPhysicalValue(ADC_Channel_EnumDef channel)
{
    switch (channel) {
        case E_ADC_CHANNEL_US_I:
            return s_adcPhysicalValues.usCurrent;
        case E_ADC_CHANNEL_RF_I:
            return s_adcPhysicalValues.rfCurrent;
        case E_ADC_CHANNEL_Heat_REF02:
            return s_adcPhysicalValues.heatRef02;
        case E_ADC_CHANNEL_Heat_REF01:
            return s_adcPhysicalValues.heatRef01;
        case E_ADC_CHANNEL_ESW_U:
            return s_adcPhysicalValues.eswVoltage;
        case E_ADC_CHANNEL_ESW_I:
            return s_adcPhysicalValues.eswCurrent;
        case E_ADC_CHANNEL_HP_PRE:
            return s_adcPhysicalValues.hpPressure;
        case E_ADC_CHANNEL_HAND_NTC:
            return s_adcPhysicalValues.handNTC;
        case E_ADC_CHANNEL_VER_ID:
            return s_adcPhysicalValues.verId;
        case E_ADC_CHANNEL_VOUT:
            return s_adcPhysicalValues.vout;
        case E_ADC_CHANNEL_MAX:
        default:
            return 0u;
    }
}

static void Drv_ADC_UpdatePhysicalValues(void)
{
    uint16_t rawValue;

    rawValue = Drv_ADC_ReadChannel(E_ADC_CHANNEL_US_I);
    s_adcPhysicalValues.usCurrent = Drv_ADC_ConvertChannel(E_ADC_CHANNEL_US_I, rawValue);

    rawValue = Drv_ADC_ReadChannel(E_ADC_CHANNEL_RF_I);
    s_adcPhysicalValues.rfCurrent = Drv_ADC_ConvertChannel(E_ADC_CHANNEL_RF_I, rawValue);

    rawValue = Drv_ADC_ReadChannel(E_ADC_CHANNEL_Heat_REF02);
    s_adcPhysicalValues.heatRef02 = Drv_ADC_ConvertChannel(E_ADC_CHANNEL_Heat_REF02, rawValue);

    rawValue = Drv_ADC_ReadChannel(E_ADC_CHANNEL_Heat_REF01);
    s_adcPhysicalValues.heatRef01 = Drv_ADC_ConvertChannel(E_ADC_CHANNEL_Heat_REF01, rawValue);

    rawValue = Drv_ADC_ReadChannel(E_ADC_CHANNEL_ESW_U);
    s_adcPhysicalValues.eswVoltage = Drv_ADC_ConvertChannel(E_ADC_CHANNEL_ESW_U, rawValue);

    rawValue = Drv_ADC_ReadChannel(E_ADC_CHANNEL_ESW_I);
    s_adcPhysicalValues.eswCurrent = Drv_ADC_ConvertChannel(E_ADC_CHANNEL_ESW_I, rawValue);

    rawValue = Drv_ADC_ReadChannel(E_ADC_CHANNEL_HP_PRE);
    s_adcPhysicalValues.hpPressure = Drv_ADC_ConvertChannel(E_ADC_CHANNEL_HP_PRE, rawValue);

    rawValue = Drv_ADC_ReadChannel(E_ADC_CHANNEL_HAND_NTC);
    s_adcPhysicalValues.handNTC = Drv_ADC_ConvertChannel(E_ADC_CHANNEL_HAND_NTC, rawValue);

    rawValue = Drv_ADC_ReadChannel(E_ADC_CHANNEL_VER_ID);
    s_adcPhysicalValues.verId = Drv_ADC_ConvertChannel(E_ADC_CHANNEL_VER_ID, rawValue);

    rawValue = Drv_ADC_ReadChannel(E_ADC_CHANNEL_VOUT);
    s_adcPhysicalValues.vout = Drv_ADC_ConvertChannel(E_ADC_CHANNEL_VOUT, rawValue);

    s_adcPhysicalValues.updateTickMs = Drv_Delay_GetTickMs();
}

void Drv_ADC_Init(void)
{
    s_adcProcessTimer.start_ms = 0u;
    s_adcProcessTimer.timeout_ms = 0u;
    s_adcProcessTimer.running = false;
    s_adcPhysicalValues.updateTickMs = 0u;
    Drv_ADC_UpdatePhysicalValues();
}

void Drv_ADC_Process(void)
{
    // 50ms period for ADC processing; can be adjusted as needed
    if (Drv_Timer_Tick(&s_adcProcessTimer, DRV_ADC_PROCESS_PERIOD_MS) == false) {
        return;
    }

    Drv_ADC_UpdatePhysicalValues();
}

uint16_t Drv_ADC_ReadChannel(ADC_Channel_EnumDef channel)
{
    if (channel >= E_ADC_CHANNEL_MAX) {
        return 0u;
    }

    if (channel == E_ADC_CHANNEL_VOUT) {
        return 0u;
    }

    return BSP_ADC_ReadRaw((BSP_ADC_Channel_t)channel);
}

uint16_t Drv_ADC_GetRealValue(ADC_Channel_EnumDef channel)
{
    return Drv_ADC_GetCachedPhysicalValue(channel);
}

const Drv_ADC_PhysicalValues_t *Drv_ADC_GetPhysicalValues(void)
{
    return &s_adcPhysicalValues;
}

void Drv_ADC_SetTempOverride(char *data)
{
    (void)Dal_NTC_MapChannel(E_NTC_HAND);
    (void)data;
}
