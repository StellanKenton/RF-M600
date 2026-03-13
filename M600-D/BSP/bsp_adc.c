/************************************************************************************
 * @file     : bsp_adc.c
 * @brief    : M600 ADC1 init with DMA - ported from M600 HAL
 * @details  : DMA continuous conversion, scan mode. Channels PA0,1,5,6,7 / PB0,1 / PC3,4.
 ***********************************************************************************/
#include "bsp_adc.h"

static const uint8_t s_adc_ch[] = {
    ADC_Channel_0,   /* RF_I      PA0 */
    ADC_Channel_1,   /* US_I      PA1 */
    ADC_Channel_5,   /* Heat_REF01 PA5 */
    ADC_Channel_6,   /* Heat_REF02 PA6 */
    ADC_Channel_8,   /* ESW_U     PB0 */
    ADC_Channel_9,   /* ESW_I     PB1 */
    ADC_Channel_7,   /* HP_PRE    PA7 */
    ADC_Channel_13,  /* HAND_NTC  PC3 */
    ADC_Channel_14,  /* HARD_VER  PC4 */
};

/* Double buffer for ADC values
 * - s_adc_dma_buffer: DMA writes here (working buffer)
 * - s_adc_read_buffer: Application reads from here (read buffer)
 * Buffer organization: Each array element corresponds to one channel
 * [0] = RF_I (PA0), [1] = US_I (PA1), [2] = Heat_REF01 (PA5), [3] = Heat_REF02 (PA6)
 * [4] = ESW_U (PB0), [5] = ESW_I (PB1), [6] = HP_PRE (PA7), [7] = HAND_NTC (PC3)
 * [8] = HARD_VER (PC4)
 */
static uint16_t s_adc_dma_buffer[BSP_ADC_CH_MAX];  /* DMA working buffer */
static uint16_t s_adc_read_buffer[BSP_ADC_CH_MAX];  /* Application read buffer */
static volatile uint8_t s_adc_buffer_ready = 0;     /* Buffer ready flag */

static uint16_t BSP_ADC_ConvertToVoltageMv(uint16_t raw)
{
    if (raw > (BSP_ADC_RESOLUTION - 1u))
        raw = (BSP_ADC_RESOLUTION - 1u);

    return (uint16_t)((((uint32_t)raw * BSP_ADC_REF_MV) + (BSP_ADC_RESOLUTION / 2u)) /
                      BSP_ADC_RESOLUTION);
}

void BSP_ADC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    uint8_t i;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    /* Analog pins: PA0,1,5,6,7 / PB0,1 / PC3,4 */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_3 | GPIO_Pin_4;
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
    
    /* Enable DMA transfer complete interrupt for double buffering */
    DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
    DMA_ITConfig(DMA1_Channel1, DMA_IT_TE, ENABLE);
    
    /* Enable DMA interrupt in NVIC */
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    DMA_Cmd(DMA1_Channel1, ENABLE);
    
    /* Initialize read buffer */
    for (i = 0; i < BSP_ADC_CH_MAX; i++) {
        s_adc_read_buffer[i] = 0;
    }

    /* Configure ADC1: scan mode, continuous conversion */
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode       = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel       = BSP_ADC_CH_MAX;
    ADC_Init(ADC1, &ADC_InitStructure);

    /* Configure regular channel sequence */
    /* ADC clock = PCLK2 / 6. With STM32F103 PCLK2 typically at 72 MHz,
     * ADCCLK = 12 MHz, so 1 cycle = 1 / 12 MHz = 83.3 ns.
     * ADC_SampleTime_71Cycles5 means 71.5 sampling cycles, about 5.96 us.
     * Including the fixed 12.5 conversion cycles, one full conversion is
     * 84 cycles total, about 7.00 us per channel.
     */
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_RF_I],       1, ADC_SampleTime_71Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_US_I],       2, ADC_SampleTime_71Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_Heat_REF01], 3, ADC_SampleTime_71Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_Heat_REF02], 4, ADC_SampleTime_71Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_ESW_U],      5, ADC_SampleTime_71Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_ESW_I],      6, ADC_SampleTime_71Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_HP_PRE],     7, ADC_SampleTime_71Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_HAND_NTC],   8, ADC_SampleTime_71Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_HARD_VER],   9, ADC_SampleTime_71Cycles5);

    /* Enable ADC DMA before enabling ADC */
    ADC_DMACmd(ADC1, ENABLE);

    /* Enable ADC */
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1)) { }
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1)) { }

    /* Start ADC continuous conversion */
    /* In continuous mode with DMA circular mode, this triggers continuous conversions */
    /* The ADC will keep converting automatically, and DMA will continuously update the buffer */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

uint16_t BSP_ADC_ReadChannel(BSP_ADC_Channel_t ch)
{
    if (ch >= BSP_ADC_CH_MAX)
    {
        return 0;
    }

    /* Read from read buffer (double buffering) */
    return s_adc_read_buffer[ch];
}

uint16_t BSP_ADC_ReadVoltage(BSP_ADC_Channel_t ch)
{
    uint16_t raw = BSP_ADC_ReadChannel(ch);
    return BSP_ADC_ConvertToVoltageMv(raw);
}


const uint16_t* BSP_ADC_GetDmaBuffer(void)
{
    /* Return read buffer (safe for application use) */
    return (const uint16_t*)s_adc_read_buffer;
}

/* DMA transfer complete interrupt handler - called from stm32f103_it.c */
void BSP_ADC_DMA_TC_Handler(void)
{
    /* Copy DMA working buffer to read buffer (atomic operation) */
    uint8_t i;

    for (i = 0; i < BSP_ADC_CH_MAX; i++) {
        s_adc_read_buffer[i] = s_adc_dma_buffer[i];
    }
    s_adc_buffer_ready = 1;
}
