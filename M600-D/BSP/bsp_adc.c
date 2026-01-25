/************************************************************************************
 * @file     : bsp_adc.c
 * @brief    : M600 ADC1 init with DMA - ported from M600 HAL
 * @details  : DMA continuous conversion, scan mode, 1.5 cycle sample. Channels PA0,1,5,6 / PB0,1 / PC2,3.
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

/* DMA buffer for ADC values - continuously updated */
static uint16_t s_adc_dma_buffer[BSP_ADC_CH_MAX];

void BSP_ADC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

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

    /* Configure DMA1 Channel1 for ADC1 */
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr      = (uint32_t)s_adc_dma_buffer;
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize         = BSP_ADC_CH_MAX;
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel1, ENABLE);

    /* Configure ADC1: scan mode, continuous conversion */
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode       = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel       = BSP_ADC_CH_MAX;
    ADC_Init(ADC1, &ADC_InitStructure);

    /* Configure regular channel sequence */
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_US_I],       1, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_RF_I],        2, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_Heat_REF02], 3, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_Heat_REF01], 4, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_ESW_U],      5, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_ESW_I],      6, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_HP_PRE],      7, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_HAND_NTC],   8, ADC_SampleTime_28Cycles5);

    /* Enable ADC DMA */
    ADC_DMACmd(ADC1, ENABLE);

    /* Enable ADC */
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1)) { }
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1)) { }

    /* Start ADC conversion */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

uint16_t BSP_ADC_ReadChannel(BSP_ADC_Channel_t ch)
{
    if (ch >= BSP_ADC_CH_MAX)
        return 0;
    return s_adc_dma_buffer[ch];
}

uint32_t BSP_ADC_ReadVoltage(BSP_ADC_Channel_t ch)
{
    uint16_t raw = BSP_ADC_ReadChannel(ch);
    return ((uint32_t)raw * BSP_ADC_REF_MV) / BSP_ADC_RESOLUTION;
}

const uint16_t* BSP_ADC_GetDmaBuffer(void)
{
    return s_adc_dma_buffer;
}
