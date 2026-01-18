/***********************************************************************************
* @file     : app_ultrasound.c
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "app_ultrasound.h"


static US_CtrlInfo_t s_USCtrlInfo;



void App_Ultrasound_Init(void)
{
    
}

void App_Ultrasound_Process(void)
{
    // Process the ultrasound module
    switch(s_USCtrlInfo.runState)
    {       
        case E_US_RUN_WAIT:
            break;
        case E_US_RUN_INIT:
            break;
        case E_US_RUN_IDLE:
            break;
        case E_US_RUN_WORKING:
            break;
        case E_US_RUN_STOP:
            break;
        case E_US_RUN_ERROR:
            break;
        default:
            break;
    }
}
/**************************End of file********************************/
