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


/**************************包含的头文件*************************/
#include <stdio.h>
#include "delay.h"


int main()
{
	SystemInit();									// 系统初始化
		
	SystemCoreClockUpdate();						// 时钟更新函数
	
//	CopyIntVertorTable();               			// 中断向量偏移地址重定义
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	// 设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	
	
	__disable_irq();
	
	__enable_irq();
	
	while(1)
	{

	}
}

/*************************************************************************************************
*									END OF FILE
*************************************************************************************************/

