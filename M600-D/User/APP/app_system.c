/**
* Copyright (c) 2023, AstroCeta, Inc. All rights reserved.
* \file app_system.c
* \brief System management - init, mode switch, treatment manager and communication process.
* \date 2025-07-30
* \author AstroCeta, Inc.
**/
#include "app_system.h"
#include "log.h"
#include "cm_backtrace.h"
#include "drv_wdg.h"
#include "app_treatmgr.h"
#include "app_comm.h"
#include "app_handcomm.h"
#include "drv_adc.h"
#include "drv_si5351.h"
#include "app_memory.h"
#include "app_led.h"
#include "bsp_delay.h"

static System_Mgr_t s_SystemMgr = {E_SYSTEM_STANDBY_MODE, 0};

System_Mode_EnumDef System_GetMode(void)
{
    return s_SystemMgr.eMode;
}

uint32_t System_GetTick(void)
{
    return s_SystemMgr.sysTick;
}

void System_ChangeMode(System_Mode_EnumDef newMode)
{
    if(newMode != s_SystemMgr.eMode && newMode < E_SYSTEM_MODE_MAX){
        s_SystemMgr.eMode = newMode;
        switch(newMode)
        {
            case E_SYSTEM_STANDBY_MODE:
                LOG_I("System mode changed to STANDBY_MODE");
                break;
            case E_SYSTEM_NORMAL_MODE:
                LOG_I("System mode changed to NORMAL_MODE");
                break;
            case E_SYSTEM_UPDATE_MODE:
                LOG_I("System mode changed to UPDATE_MODE");
                break;
            case E_SYSTEM_MODE_MAX:
            default:
                LOG_I("System mode changed to UNKNOWN");
                break;
        }
    }
}

void System_Init(void)
{
    Log_Init();
    Drv_WatchDogResartCheck();
    cm_backtrace_init(FIRMWARE_NAME, FIRMWARE_VERSION, HARDWARE_VERSION);
    LOG_I("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&");
    LOG_I("System initialized.");
    LOG_I("Firmware: %s, Version: %s, Hardware: %s", FIRMWARE_NAME, FIRMWARE_VERSION, HARDWARE_VERSION);

    // Initialize the treatment manager
    LOG_I("Startup trace: entering App_TreatMgr_Init()");
    App_TreatMgr_Init();
    LOG_I("Startup trace: App_TreatMgr_Init() returned");
    LOG_I("Treatment manager initialized.");
    App_Comm_Init();
    LOG_I("Communication initialized.");
    App_HandComm_Init();
    LOG_I("Handle communication initialized.");
    App_LED_Init();
    LOG_I("LED module initialized.");
    App_Memory_Init();
    LOG_I("Memory module initialized.");

    // output pwm test - 1MHz complementary PWM on TIM1
//    Drv_SI5351_Init();                      // Initialize Si5351
//    Drv_SI5351_SetFrequency(923);          // Set Si5351 CLK1 to 2MHz as TIM1 ETR clock source (2MHz / 2 = 1MHz PWM)
//    Drv_SI5351_SetComplementaryPWM(true);   // Enable TIM1 complementary PWM output (CH1/CH1N)
//    LOG_I("TIM1 complementary PWM enabled at 1MHz");
}

void SystemManager(void)
{
    switch(s_SystemMgr.eMode)
    {
        case E_SYSTEM_STANDBY_MODE:
            // Handle standby mode
            System_ChangeMode(E_SYSTEM_NORMAL_MODE);
            break;
        case E_SYSTEM_NORMAL_MODE:
            // Handle normal mode
            App_TreatMgr_Process();
            break;
        case E_SYSTEM_UPDATE_MODE:
            // Handle update mode
            break;
        case E_SYSTEM_MODE_MAX:
        default:
            // Handle unexpected mode
            break;
    }
	
	App_LED_Process();
    App_Memory_Process();       // Process memory configuration for all treatment modules
    App_Comm_Process();         // Process communication with external devices
    App_HandComm_Process();     // Process handle communication
    Log_Process(10);
}

/**
* @brief SystemProcess
* @retval None
**/
void SystemProcess(void)
{
    /* Update system tick */
    s_SystemMgr.sysTick = BSP_GetTick_ms();
    
    SystemManager();
    Drv_WatchDogFeed();
}
/**************************End of file********************************/
