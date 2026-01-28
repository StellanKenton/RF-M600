/**
* Copyright (c) 2023, AstroCeta, Inc. All rights reserved.
* \file app_system.h
* \brief Implementation of a ring buffer for efficient data handling.
* \date 2025-07-30
* \author AstroCeta, Inc.
**/
#include "app_system.h"
#include "log.h"
#include "cm_backtrace.h"
#include "drv_wdg.h"
#include "app_treatmgr.h"
#include "app_comm.h"

static System_Mgr_t s_SystemMgr = {E_SYSTEM_STANDBY_MODE, 0};



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
    App_TreatMgr_Init();
    LOG_I("Treatment manager initialized.");
    App_Comm_Init();
    LOG_I("Communication initialized.");
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

    App_Comm_Process();
    
}

/**
* @brief SystemProcess
* @retval None
**/
void SystemProcess(void)
{
    SystemManager();
    Drv_WatchDogFeed();
}
/**************************End of file********************************/


