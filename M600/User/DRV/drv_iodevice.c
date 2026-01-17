/***********************************************************************************
* @file     : drv_iodevice.c
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drv_iodevice.h"

/**
 * @brief Read IO synchronization signals from hardware
 * @param pSignals Pointer to structure to store the signal states
 */
void Drv_IODevice_ReadSyncSignals(IODevice_SyncSignals_t *pSignals)
{
    GPIO_PinState us_state, esw_state, rf_state;
    
    if (pSignals == NULL)
    {
        return;
    }
    
    // Read IO synchronization signals from hardware
    us_state = HAL_GPIO_ReadPin(IO_SYN_US_GPIO_Port, IO_SYN_US_Pin);
    
    // Read ESW signal - if IO_SYN_ESW is not defined, use IO_SYN_RFC12 as alternative
    #ifdef IO_SYN_ESW_Pin
    esw_state = HAL_GPIO_ReadPin(IO_SYN_ESW_GPIO_Port, IO_SYN_ESW_Pin);
    #else
    // Use IO_SYN_RFC12 as ESW signal if IO_SYN_ESW is not defined
    esw_state = HAL_GPIO_ReadPin(IO_SYN_RFC12_GPIO_Port, IO_SYN_RFC12_Pin);
    #endif
    
    rf_state = HAL_GPIO_ReadPin(IO_SYN_RF_GPIO_Port, IO_SYN_RF_Pin);
    
    // Convert GPIO_PinState to boolean (GPIO_PIN_SET = High, GPIO_PIN_RESET = Low)
    pSignals->us = (us_state == GPIO_PIN_SET) ? 1 : 0;
    pSignals->esw = (esw_state == GPIO_PIN_SET) ? 1 : 0;
    pSignals->rf = (rf_state == GPIO_PIN_SET) ? 1 : 0;
}

/**
 * @brief Get working mode based on IO synchronization signals (logic only)
 * 
 * Working mode truth table:
 * IO_SYN_US | IO_SYN_ESW | IO_SYN_RF | Working Mode
 * ----------|------------|-----------|------------------
 *     L     |     H      |     H     | Ultrasound (超声)
 *     H     |     L      |     H     | Shockwave (冲击波)
 *     H     |     H      |     L     | Radio Frequency (射频)
 *     H     |     L      |     L     | Negative Pressure Heat (负压加热)
 *     H     |     H      |     H     | Treatment Head Not Connected (治疗头未连接)
 *   Others  |   Others   |   Others  | Connection Error (治疗头连接报错)
 * 
 * @param pSignals Pointer to structure containing the signal states
 * @retval IODevice_WorkingMode_EnumDef The working mode state
 */
IODevice_WorkingMode_EnumDef Drv_IODevice_GetWorkingMode(const IODevice_SyncSignals_t *pSignals)
{
    if (pSignals == NULL)
    {
        return E_IODEVICE_MODE_ERROR;
    }
    
    // Determine working mode based on truth table
    // IO_SYN_US = L, IO_SYN_ESW = H, IO_SYN_RF = H -> Ultrasound
    if (pSignals->us == 0 && pSignals->esw == 1 && pSignals->rf == 1)
    {
        return E_IODEVICE_MODE_ULTRASOUND;
    }
    // IO_SYN_US = H, IO_SYN_ESW = L, IO_SYN_RF = H -> Shockwave
    else if (pSignals->us == 1 && pSignals->esw == 0 && pSignals->rf == 1)
    {
        return E_IODEVICE_MODE_SHOCKWAVE;
    }
    // IO_SYN_US = H, IO_SYN_ESW = H, IO_SYN_RF = L -> Radio Frequency
    else if (pSignals->us == 1 && pSignals->esw == 1 && pSignals->rf == 0)
    {
        return E_IODEVICE_MODE_RADIO_FREQUENCY;
    }
    // IO_SYN_US = H, IO_SYN_ESW = L, IO_SYN_RF = L -> Negative Pressure Heat
    else if (pSignals->us == 1 && pSignals->esw == 0 && pSignals->rf == 0)
    {
        return E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT;
    }
    // IO_SYN_US = H, IO_SYN_ESW = H, IO_SYN_RF = H -> Treatment Head Not Connected
    else if (pSignals->us == 1 && pSignals->esw == 1 && pSignals->rf == 1)
    {
        return E_IODEVICE_MODE_NOT_CONNECTED;
    }
    // All other combinations -> Connection Error
    else
    {
        return E_IODEVICE_MODE_ERROR;
    }
}


IODevice_WorkingMode_EnumDef Drv_IODevice_GetProbeStatus(void)
{
    IODevice_SyncSignals_t s_SyncSignals;
    Drv_IODevice_ReadSyncSignals(&s_SyncSignals);
    return Drv_IODevice_GetWorkingMode(&s_SyncSignals);
}

/**************************End of file********************************/
