/************************************************************************************
 * @file     : bsp_adc.c
 * @brief    : M600 ADC BSP实现 - 负责获取原始ADC值
 * @details  : ADC1 DMA连续采集，扫描模式，双缓冲机制
 ***********************************************************************************/
#include "bsp_adc.h"

static const uint8_t s_adc_ch[] = {
    ADC_Channel_0,   /*0 US_I     PA0 */
    ADC_Channel_1,   /*1 RF_I     PA1 */
    ADC_Channel_5,   /*2 Heat_REF02 PA5 */
    ADC_Channel_6,   /*3 Heat_REF01 PA6 */
    ADC_Channel_8,   /*4 ESW_U    PB0 */
    ADC_Channel_9,   /*5 ESW_I    PB1 */
    ADC_Channel_12,  /*6 HP_PRE   PC2 */
    ADC_Channel_13,  /*7 HAND_NTC PC3 */
    ADC_Channel_15,  /*8 VOUT     PC5 */
};

/* ADC双缓冲区
 * - s_adc_dma_buffer: DMA写入缓冲区（工作缓冲区）
 * - s_adc_read_buffer: 应用读取缓冲区（读缓冲区）
 * 缓冲区组织：每个数组元素对应一个通道
 * [0] = US_I (PA0), [1] = RF_I (PA1), [2] = Heat_REF02 (PA5), [3] = Heat_REF01 (PA6)
 * [4] = ESW_U (PB0), [5] = ESW_I (PB1), [6] = HP_PRE (PC2), [7] = HAND_NTC (PC3), [8] = VOUT (PC5)
 */
static uint16_t s_adc_dma_buffer[BSP_ADC_CH_MAX];   /* DMA工作缓冲区 */
static uint16_t s_adc_read_buffer[BSP_ADC_CH_MAX];  /* 应用读缓冲区 */
static volatile uint8_t s_adc_buffer_ready = 0;     /* 缓冲区就绪标志 */

void BSP_ADC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    /* 配置模拟输入引脚: PA0,1,5,6 / PB0,1 / PC2,3,5 */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    /* 配置DMA1通道1用于ADC1 */
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
    
    /* 使能DMA传输完成中断用于双缓冲 */
    DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
    DMA_ITConfig(DMA1_Channel1, DMA_IT_TE, ENABLE);
    
    /* 配置DMA中断 */
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    DMA_Cmd(DMA1_Channel1, ENABLE);
    
    /* 初始化读缓冲区 */
    for (uint8_t i = 0; i < BSP_ADC_CH_MAX; i++) {
        s_adc_read_buffer[i] = 0;
    }

    /* 配置ADC1: 扫描模式，连续转换 */
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode       = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel       = BSP_ADC_CH_MAX;
    ADC_Init(ADC1, &ADC_InitStructure);

    /* 配置规则通道序列 */
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_US_I],       1, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_RF_I],        2, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_Heat_REF02], 3, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_Heat_REF01], 4, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_ESW_U],      5, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_ESW_I],      6, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_HP_PRE],      7, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_HAND_NTC],   8, ADC_SampleTime_28Cycles5);
    ADC_RegularChannelConfig(ADC1, s_adc_ch[BSP_ADC_CH_VOUT],       9, ADC_SampleTime_28Cycles5);

    /* 使能ADC DMA */
    ADC_DMACmd(ADC1, ENABLE);

    /* 使能ADC并校准 */
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1)) { }
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1)) { }

    /* 启动ADC连续转换 */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

uint16_t BSP_ADC_ReadChannel(BSP_ADC_Channel_t ch)
{
    if (ch >= BSP_ADC_CH_MAX)
        return 0;
    /* 从读缓冲区读取（双缓冲机制保证数据一致性） */
    return s_adc_read_buffer[ch];
}

/* DMA传输完成中断处理函数 - 从stm32f103_it.c中调用 */
void BSP_ADC_DMA_TC_Handler(void)
{
    /* 将DMA工作缓冲区复制到读缓冲区（原子操作） */
    for (uint8_t i = 0; i < BSP_ADC_CH_MAX; i++) {
        s_adc_read_buffer[i] = s_adc_dma_buffer[i];
    }
    s_adc_buffer_ready = 1;
}
