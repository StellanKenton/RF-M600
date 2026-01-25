/************************************************************************************
 * @file     : bsp_adc.c
 * @brief    : M600 ADC1 init and channel read - ported from M600 HAL
 * @details  : Single conversion, software trigger, 1.5 cycle sample. Channels PA0,1,5,6 / PB0,1 / PC2,3.
 ***********************************************************************************/
#include "bsp_adc.h"

static const uint8_t s_adc_ch[] = {
    ADC_Channel_0,   /* US_I     PA0 */
    ADC_Channel_1,   /* RF_I     PA1 */
    ADC_Channel_5,   /* Heat_REF02 PA5 */
    ADC_Channel_6,   /* Heat_REF01 PA6 */
    ADC_Channel_8,   /* ESW_U    PB0 */
    ADC_Channel_9,   /* ESW_I    PB1 */
    ADC_Channel_12,  /* HP_PRE   PC2 */
    ADC_Channel_13,  /* HAND_NTC PC3 */
};

void BSP_ADC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

    /* Analog pins: PA0,1,5,6 / PB0,1 / PC2,3 */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode       = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel       = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1)) { }
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1)) { }
}

uint16_t BSP_ADC_ReadChannel(BSP_ADC_Channel_t ch)
{
    if (ch >= BSP_ADC_CH_MAX)
        return 0;

    ADC_RegularChannelConfig(ADC1, s_adc_ch[ch], 1, ADC_SampleTime_1Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)) { }
    return (uint16_t)ADC_GetConversionValue(ADC1);
}

uint32_t BSP_ADC_ReadVoltage(BSP_ADC_Channel_t ch)
{
    uint16_t raw = BSP_ADC_ReadChannel(ch);
    return ((uint32_t)raw * BSP_ADC_REF_MV) / BSP_ADC_RESOLUTION;
}
