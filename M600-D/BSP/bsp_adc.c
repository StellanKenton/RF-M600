/************************************************************************************
 * @file     : bsp_adc.c
 * @brief    : M600 ADC1 init - one settled channel sampled per request
 * @details  : Software-triggered per-channel conversions with first-sample discard.
 ***********************************************************************************/
#include "bsp_adc.h"

/* Logical channel -> STM32 ADC hardware channel mapping. */
static const uint8_t s_adc_hw_ch[] = {
    ADC_Channel_0,   /* RF_I      PA0 */
    ADC_Channel_1,   /* US_I      PA1 */
    ADC_Channel_5,   /* Heat_REF01 PA5 */
    ADC_Channel_6,   /* Heat_REF02 PA6 */
    ADC_Channel_8,   /* ESW_U     PB0 */
    ADC_Channel_9,   /* ESW_I     PB1 */
    ADC_Channel_7,   /* HP_PRE    PA7 */
    ADC_Channel_13,  /* HAND_NTC  PC3 */
    ADC_Channel_14,  /* HARD_VER  PC4 */
    ADC_Channel_15,  /* VOUT      PC5 */
};

/*
 * Settled sampling policy:
 * - After each mux switch, discard the first conversion.
 * - Average the following stable conversions.
 */
#define BSP_ADC_SETTLE_DISCARD_COUNT  4u
#define BSP_ADC_STABLE_SAMPLE_COUNT   4u

static uint16_t s_adc_read_buffer[BSP_ADC_CH_MAX];  /* Application read buffer */
static volatile uint8_t s_adc_buffer_ready = 0;     /* Buffer ready flag */
static uint16_t BSP_ADC_ReadSingleConversion(void)
{
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) {
    }

    return ADC_GetConversionValue(ADC1);
}

static uint16_t BSP_ADC_ReadSettledChannel(BSP_ADC_Channel_t ch)
{
    uint32_t accumulated = 0u;
    uint8_t sampleIndex;

    /* Discard phase: minimum sample time to pre-charge cap with minimal pin loading.
     * 1.5 cycles = 125 ns per sample switch closure. */
    ADC_RegularChannelConfig(ADC1, s_adc_hw_ch[ch], 1u, ADC_SampleTime_1Cycles5);

    for (sampleIndex = 0u; sampleIndex < BSP_ADC_SETTLE_DISCARD_COUNT; sampleIndex++) {
        (void)BSP_ADC_ReadSingleConversion();
    }

    /* Stable phase: proper sample time for accurate conversion.
     * 7.5 cycles = 625 ns, 7.1 tau for 10 kohm source (99.9 % settled). */
    ADC_RegularChannelConfig(ADC1, s_adc_hw_ch[ch], 1u, BSP_ADC_SAMPLE_TIME);

    for (sampleIndex = 0u; sampleIndex < BSP_ADC_STABLE_SAMPLE_COUNT; sampleIndex++) {
        accumulated += (uint32_t)BSP_ADC_ReadSingleConversion();
    }

    /* Park mux on internal VREFINT channel to stop loading the external pin. */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_17, 1u, BSP_ADC_SAMPLE_TIME);

    return (uint16_t)((accumulated + (BSP_ADC_STABLE_SAMPLE_COUNT / 2u)) / BSP_ADC_STABLE_SAMPLE_COUNT);
}

static void BSP_ADC_SampleOneChannelInternal(BSP_ADC_Channel_t channel)
{
    s_adc_buffer_ready = 0u;
    s_adc_read_buffer[channel] = BSP_ADC_ReadSettledChannel(channel);
    s_adc_buffer_ready = 1u;
}

void BSP_ADC_SampleOneChannel(BSP_ADC_Channel_t ch)
{
    if (ch < BSP_ADC_CH_MAX) {
        BSP_ADC_SampleOneChannelInternal(ch);
    }
}

static float BSP_ADC_ConvertToVoltageV(uint16_t raw)
{
    if (raw > (BSP_ADC_RESOLUTION - 1u))
        raw = (BSP_ADC_RESOLUTION - 1u);

    return ((float)raw * ((float)BSP_ADC_REF_MV / 1000.0f)) / (float)BSP_ADC_RESOLUTION;
}

void BSP_ADC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    uint8_t i;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

    /* Analog pins: PA0,1,5,6,7 / PB0,1 / PC3,4,5 */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    
    /* Initialize read buffer */
    for (i = 0; i < BSP_ADC_CH_MAX; i++) {
        s_adc_read_buffer[i] = 0;
    }

    /* Configure ADC1 for software-triggered single conversions. */
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode       = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel       = 1u;
    ADC_Init(ADC1, &ADC_InitStructure);

    /* ADC timing: split sample-time strategy.
     * ADCCLK = PCLK2 / 6 = 12 MHz (83.3 ns / cycle).
     * Discard phase uses ADC_SampleTime_1Cycles5 (125 ns) for minimal loading.
     * Stable phase uses ADC_SampleTime_7Cycles5 (625 ns, 7.1 tau for 10 kohm).
     * One stable conversion = (7.5 + 12.5) / 12 MHz = 1.67 us.
     */
    ADC_RegularChannelConfig(ADC1, s_adc_hw_ch[BSP_ADC_CH_RF_I], 1u, BSP_ADC_SAMPLE_TIME);

    /* Enable internal VREFINT so we can park the mux there between conversions. */
    ADC_TempSensorVrefintCmd(ENABLE);

    /* Enable ADC */
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1)) { }
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1)) { }

    /* Prime HAND_NTC once at startup for isolation testing. */
    BSP_ADC_SampleOneChannelInternal(BSP_ADC_CH_HAND_NTC);
}

uint16_t BSP_ADC_ReadRaw(BSP_ADC_Channel_t ch)
{
    if (ch >= BSP_ADC_CH_MAX)
    {
        return 0;
    }

    /* Read from read buffer (double buffering) */
    return s_adc_read_buffer[ch];
}

float BSP_ADC_ReadVoltage(BSP_ADC_Channel_t ch)
{
    uint16_t raw = BSP_ADC_ReadRaw(ch);
    return BSP_ADC_ConvertToVoltageV(raw);
}

uint16_t BSP_ADC_ReadChannel(BSP_ADC_Channel_t ch)
{
    return BSP_ADC_ReadRaw(ch);
}


const uint16_t* BSP_ADC_GetDmaBuffer(void)
{
    /* Backward-compatible accessor for the latest stable samples. */
    return (const uint16_t*)s_adc_read_buffer;
}

void BSP_ADC_RequestScan(void)
{
    BSP_ADC_SampleOneChannelInternal(BSP_ADC_CH_HAND_NTC);
}

uint8_t BSP_ADC_IsDataReady(void)
{
    return s_adc_buffer_ready;
}

/* DMA transfer complete interrupt handler - called from stm32f103_it.c */
void BSP_ADC_DMA_TC_Handler(void)
{
    /* ADC DMA is no longer used; keep this symbol for compatibility. */
}
