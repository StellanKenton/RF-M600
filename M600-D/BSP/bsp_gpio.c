/************************************************************************************
 * @file     : bsp_gpio.c
 * @brief    : M600 GPIO init - pins from M600 gpio.c (STM32 Standard Library)
 * @details  : Outputs: Buzzer, pwr_control1~4, CTR_OUT, CTR_US_RF, HP_motor/lose/HEAT, FAN.
 *             Inputs: MCU_FOOT, IO_SYN_US/RF/ESW, MCU_I_O.
 ***********************************************************************************/
#include "bsp_gpio.h"
#include "bsp_adc.h"
#include "bsp_tim.h"
#include "bsp_usart.h"
#include "bsp_dac.h"
#include "bsp_i2c.h"
#include "bsp_iwdg.h"

void BSP_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* GPIO clocks */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
                           RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* Output level: all LOW */
    GPIO_ResetBits(GPIOC, MCU_Buzzer_Pin | pwr_control4_Pin | pwr_control3_Pin |
                          pwr_control2_Pin | pwr_control1_Pin);
    GPIO_ResetBits(GPIOB, MCU_CTR_OUT_Pin | MCU_CTR_US_RF_Pin | CTR_HP_motor_Pin |
                          CTR_HP_lose_Pin | CTR_HEAT_HP_Pin);
    GPIO_ResetBits(CTR_FAN_Port, CTR_FAN_Pin);

    /* GPIOC outputs: Buzzer, pwr_control1~4 */
    GPIO_InitStructure.GPIO_Pin   = MCU_Buzzer_Pin | pwr_control4_Pin | pwr_control3_Pin |
                                    pwr_control2_Pin | pwr_control1_Pin;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* GPIOC inputs: MCU_FOOT, IO_SYN_US, IO_SYN_RF, IO_SYN_ESW */
    GPIO_InitStructure.GPIO_Pin  = MCU_FOOT_Pin | IO_SYN_US_Pin | IO_SYN_RF_Pin | IO_SYN_ESW_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* GPIOB outputs: MCU_CTR_OUT, MCU_CTR_US_RF, CTR_HP_motor, CTR_HP_lose, CTR_HEAT_HP */
    GPIO_InitStructure.GPIO_Pin   = MCU_CTR_OUT_Pin | MCU_CTR_US_RF_Pin | CTR_HP_motor_Pin |
                                    CTR_HP_lose_Pin | CTR_HEAT_HP_Pin;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* GPIOB input: MCU_I_O */
    GPIO_InitStructure.GPIO_Pin  = MCU_I_O_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(MCU_I_O_Port, &GPIO_InitStructure);

    /* GPIOD output: CTR_FAN */
    GPIO_InitStructure.GPIO_Pin   = CTR_FAN_Pin;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(CTR_FAN_Port, &GPIO_InitStructure);
}

void BSP_Init(void)
{
    BSP_GPIO_Init();
    BSP_ADC_Init();
    BSP_DAC_Init();
    BSP_TIM1_Init();
    BSP_TIM4_Init();
    BSP_USART1_Init(115200);
    BSP_USART2_Init(115200);
    BSP_I2C1_Init();
    BSP_I2C2_Init();
    /* BSP_IWDG_Init();  optional, enable when using IWDG */
}
