/************************************************************************************
 * @file     : drv_adc.c
 * @brief    : ADC driver stubs
 ***********************************************************************************/
#include "drv_adc.h"
#include "bsp_adc.h"
#include "drv_delay.h"

#define ADC_RESOLUTION  4096u
#define VOUT_DIVIDER_NUMERATOR  15.3529f
#define NTC_SERIES_R    10000u  /* series R with NTC, ohm */
#define NTC_RAW_OPEN    3900u   /* ADC raw > this: NTC open */
#define NTC_RAW_SHORT   50u     /* ADC raw < this: NTC short */
#define NTC_TEMP_OPEN_CODE   0xFFFFu
#define NTC_TEMP_SHORT_CODE  0xEEFFu
#define NTC_TEMP_MIN_C       (-40)
#define NTC_TEMP_MAX_C       105
#define NTC_TEMP_SCALE       10
#define DRV_ADC_PROCESS_PERIOD_MS  10u

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

static uint16_t Drv_ADC_ConvertChannel(BSP_ADC_Channel_t channel, uint16_t raw);
static uint16_t Drv_ADC_GetCachedPhysicalValue(BSP_ADC_Channel_t channel);
static void Drv_ADC_UpdatePhysicalValues(void);

static BSP_ADC_Channel_t Dal_NTC_MapChannel(NTC_Type_EnumDef ntcType)
{
    switch (ntcType) {
        case E_NTC_HAND:
            return BSP_ADC_CH_HAND_NTC;
        case E_NTC_MAIN:
            return BSP_ADC_CH_Heat_REF01;
        case E_NTC_MAX:
        default:
            return BSP_ADC_CH_MAX;
    }
}

static uint16_t Drv_ADC_GetUSCurrentValue(uint16_t raw)
{
    (void)raw;
    return 0u;
}

static uint16_t Drv_ADC_GetRFCurrentValue(uint16_t raw)
{
    if(raw < 3950) {
        s_adcPhysicalValues.isContactSkin = false;
    } else {
        s_adcPhysicalValues.isContactSkin = true;
    }
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
    uint32_t resistance10;
    uint32_t lowerResistance;
    uint32_t upperResistance;
    uint32_t deltaResistance;
    uint32_t offsetResistance;
    uint32_t tempOffset;
    uint32_t index;
    int32_t tempDeciC;

    if (raw >= NTC_RAW_OPEN) {
        return NTC_TEMP_OPEN_CODE;
    }

    if (raw <= NTC_RAW_SHORT) {
        return NTC_TEMP_SHORT_CODE;
    }
    raw = 4096 - raw;  // convert to NTC side raw
    resistance10 = ((uint32_t)raw * (NTC_SERIES_R / 10u) + ((uint32_t)(ADC_RESOLUTION - raw) / 2u)) /
                   (uint32_t)(ADC_RESOLUTION - raw);

    if (resistance10 >= s_ntc_temp_table[0]) {
        return (uint16_t)((int16_t)(NTC_TEMP_MIN_C * NTC_TEMP_SCALE));
    }

    if (resistance10 <= s_ntc_temp_table[NTC_TABLE_SIZE - 1u]) {
        return (uint16_t)((int16_t)(NTC_TEMP_MAX_C * NTC_TEMP_SCALE));
    }

    for (index = 0u; index < (NTC_TABLE_SIZE - 1u); index++) {
        lowerResistance = s_ntc_temp_table[index + 1u];
        upperResistance = s_ntc_temp_table[index];
        if ((resistance10 <= upperResistance) && (resistance10 >= lowerResistance)) {
            deltaResistance = upperResistance - lowerResistance;
            offsetResistance = upperResistance - resistance10;
            tempOffset = (offsetResistance * NTC_TEMP_SCALE + (deltaResistance / 2u)) / deltaResistance;
            tempDeciC = ((int32_t)NTC_TEMP_MIN_C + (int32_t)index) * NTC_TEMP_SCALE + (int32_t)tempOffset;
            return (uint16_t)((int16_t)tempDeciC);
        }
    }

    return (uint16_t)((int16_t)(NTC_TEMP_MAX_C * NTC_TEMP_SCALE));
}

static uint16_t Drv_ADC_GetVerIdValue(uint16_t raw)
{
    (void)raw;
    return 0u;
}

static uint16_t Drv_ADC_GetVoutValue(uint16_t raw)
{
    float rawAdf = (float)raw;
    float Voltage = (BSP_ADC_REF_MV / 10u) *((float)(rawAdf / (float)ADC_RESOLUTION));
    return (uint16_t)((uint32_t)VOUT_DIVIDER_NUMERATOR * Voltage);
}

static uint16_t Drv_ADC_ConvertChannel(BSP_ADC_Channel_t channel, uint16_t raw)
{
    switch (channel) {
        case BSP_ADC_CH_US_I:
            return Drv_ADC_GetUSCurrentValue(raw);
        case BSP_ADC_CH_RF_I:
            return Drv_ADC_GetRFCurrentValue(raw);
        case BSP_ADC_CH_Heat_REF02:
            return Drv_ADC_GetHeatRef02Value(raw);
        case BSP_ADC_CH_Heat_REF01:
            return Drv_ADC_GetHeatRef01Value(raw);
        case BSP_ADC_CH_ESW_U:
            return Drv_ADC_GetESWVoltageValue(raw);
        case BSP_ADC_CH_ESW_I:
            return Drv_ADC_GetESWCurrentValue(raw);
        case BSP_ADC_CH_HP_PRE:
            return Drv_ADC_GetHPPressureValue(raw);
        case BSP_ADC_CH_HAND_NTC:
            return Drv_ADC_GetHandNTCValue(raw);
        case BSP_ADC_CH_HARD_VER:
            return Drv_ADC_GetVerIdValue(raw);
        case BSP_ADC_CH_VOUT:
            return Drv_ADC_GetVoutValue(raw);
        case BSP_ADC_CH_MAX:
        default:
            return 0u;
    }
}

static uint16_t Drv_ADC_GetCachedPhysicalValue(BSP_ADC_Channel_t channel)
{
    switch (channel) {
        case BSP_ADC_CH_US_I:
            return s_adcPhysicalValues.usCurrent;
        case BSP_ADC_CH_RF_I:
            return s_adcPhysicalValues.rfCurrent;
        case BSP_ADC_CH_Heat_REF02:
            return s_adcPhysicalValues.heatRef02;
        case BSP_ADC_CH_Heat_REF01:
            return s_adcPhysicalValues.heatRef01;
        case BSP_ADC_CH_ESW_U:
            return s_adcPhysicalValues.eswVoltage;
        case BSP_ADC_CH_ESW_I:
            return s_adcPhysicalValues.eswCurrent;
        case BSP_ADC_CH_HP_PRE:
            return s_adcPhysicalValues.hpPressure;
        case BSP_ADC_CH_HAND_NTC:
            return s_adcPhysicalValues.handNTC;
        case BSP_ADC_CH_HARD_VER:
            return s_adcPhysicalValues.verId;
        case BSP_ADC_CH_VOUT:
            return s_adcPhysicalValues.vout;
        case BSP_ADC_CH_MAX:
        default:
            return 0u;
    }
}

static void Drv_ADC_UpdatePhysicalValues(void)
{
    uint16_t rawValue;

    rawValue = Drv_ADC_ReadChannel(BSP_ADC_CH_US_I);
    s_adcPhysicalValues.usCurrent = Drv_ADC_ConvertChannel(BSP_ADC_CH_US_I, rawValue);

    rawValue = Drv_ADC_ReadChannel(BSP_ADC_CH_RF_I);
    s_adcPhysicalValues.rfCurrent = Drv_ADC_ConvertChannel(BSP_ADC_CH_RF_I, rawValue);

    rawValue = Drv_ADC_ReadChannel(BSP_ADC_CH_Heat_REF02);
    s_adcPhysicalValues.heatRef02 = Drv_ADC_ConvertChannel(BSP_ADC_CH_Heat_REF02, rawValue);

    rawValue = Drv_ADC_ReadChannel(BSP_ADC_CH_Heat_REF01);
    s_adcPhysicalValues.heatRef01 = Drv_ADC_ConvertChannel(BSP_ADC_CH_Heat_REF01, rawValue);

    rawValue = Drv_ADC_ReadChannel(BSP_ADC_CH_ESW_U);
    s_adcPhysicalValues.eswVoltage = Drv_ADC_ConvertChannel(BSP_ADC_CH_ESW_U, rawValue);

    rawValue = Drv_ADC_ReadChannel(BSP_ADC_CH_ESW_I);
    s_adcPhysicalValues.eswCurrent = Drv_ADC_ConvertChannel(BSP_ADC_CH_ESW_I, rawValue);

    rawValue = Drv_ADC_ReadChannel(BSP_ADC_CH_HP_PRE);
    s_adcPhysicalValues.hpPressure = Drv_ADC_ConvertChannel(BSP_ADC_CH_HP_PRE, rawValue);

    rawValue = Drv_ADC_ReadChannel(BSP_ADC_CH_HAND_NTC);
    s_adcPhysicalValues.handNTC = Drv_ADC_ConvertChannel(BSP_ADC_CH_HAND_NTC, rawValue);

    rawValue = Drv_ADC_ReadChannel(BSP_ADC_CH_HARD_VER);
    s_adcPhysicalValues.verId = Drv_ADC_ConvertChannel(BSP_ADC_CH_HARD_VER, rawValue);

    rawValue = Drv_ADC_ReadVoutRaw();
    s_adcPhysicalValues.vout = Drv_ADC_GetVoutValue(rawValue);

    s_adcPhysicalValues.updateTickMs = Drv_Delay_GetTickMs();
}

void Drv_ADC_Init(void)
{
    s_adcProcessTimer.start_ms = 0u;
    s_adcProcessTimer.timeout_ms = 0u;
    s_adcProcessTimer.running = false;
    s_adcPhysicalValues.updateTickMs = 0u;
    BSP_ADC_RequestScan();
}

void Drv_ADC_Process(void)
{
    // 50ms period for ADC processing; can be adjusted as needed
    if (Drv_Timer_Tick(&s_adcProcessTimer, DRV_ADC_PROCESS_PERIOD_MS) == false) {
        return;
    }

    if (BSP_ADC_IsDataReady() != 0u) {
        Drv_ADC_UpdatePhysicalValues();
    }

    BSP_ADC_RequestScan();
}

uint16_t Drv_ADC_ReadChannel(BSP_ADC_Channel_t channel)
{
    if (channel >= BSP_ADC_CH_MAX) {
        return 0u;
    }

    return BSP_ADC_ReadRaw(channel);
}

uint16_t Drv_ADC_GetRealValue(BSP_ADC_Channel_t channel)
{
    return Drv_ADC_GetCachedPhysicalValue(channel);
}

uint16_t Drv_ADC_ReadVoutRaw(void)
{
    return BSP_ADC_ReadRaw(BSP_ADC_CH_VOUT);
}

uint16_t Drv_ADC_GetVoutRealValue(void)
{
    return s_adcPhysicalValues.vout;
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
