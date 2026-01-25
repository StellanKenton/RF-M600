/************************************************************************************
 * @file     : bsp_dac.c
 * @brief    : M600 DAC Ch1 init - PA4, ported from M600 HAL
 * @details  : Trigger none, output buffer enable. 12-bit right align.
 ***********************************************************************************/
#include "bsp_dac.h"



void BSP_DAC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    DAC_InitTypeDef DAC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    DAC_StructInit(&DAC_InitStructure);
    DAC_InitStructure.DAC_Trigger      = DAC_Trigger_None;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
    DAC_Init(DAC_Channel_1, &DAC_InitStructure);
    DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_SetChannel1Data(DAC_Align_12b_R, 0);
}

void BSP_DAC_SetValue(uint16_t value)
{
    if (value > 4095)
        value = 4095;
    DAC_SetChannel1Data(DAC_Align_12b_R, value);
}

void BSP_DAC_SetVoltage(uint16_t mv)
{
    uint32_t v;
    if (mv > BSP_DAC_REF_MV)
        mv = BSP_DAC_REF_MV;
    v = ((uint32_t)mv * (BSP_DAC_RES - 1)) / BSP_DAC_REF_MV;
    DAC_SetChannel1Data(DAC_Align_12b_R, (uint16_t)v);
}
