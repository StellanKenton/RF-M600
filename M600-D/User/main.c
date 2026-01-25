/******************************************Copyright (C)****************************************
*
* -------------------------------------Company Information-------------------------------------
* Company    : Nanjing Medlander Medical Technology Co. Ltd..
* 
* URL		 : http://www.medlander.com/
* 
* -------------------------------------C File Descriptions-------------------------------------
* File    	 : main.c
* Version    : Ver 1.1      // 
* Department : Medlander@Hardware
* Author	 : Daisy
* Data		 : 2020.11.03
* Hardware   : STM32F103CBT6.ARM.Cortex M3
* Description: main Function
*
***********************************************************************************************/


/**************************������ͷ�ļ�*************************/
#include <stdio.h>
#include "stm32f10x.h"
#include "app_system.h"
#include "drv_init.h"

int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    Drv_System_Init();   /* DAL -> BSP: GPIO, ADC, DAC, TIM, USART, I2C, SysTick */

    __disable_irq();
    System_Init();
    __enable_irq();

    while (1)
    {
        SystemProcess();
    }
}

/*************************************************************************************************
*									END OF FILE
*************************************************************************************************/

