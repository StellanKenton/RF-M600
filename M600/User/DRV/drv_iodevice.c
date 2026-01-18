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
#include "dal_pinctrl.h"
#include "stm32f1xx_hal.h"
#include "main.h"
/* Debounce configuration */
#define IODEVICE_DEBOUNCE_TIME_MS   50   ///< Debounce time in milliseconds

/* Static variables for debounce */
static IODevice_WorkingMode_EnumDef s_lastStableMode = E_IODEVICE_MODE_NOT_CONNECTED;
static IODevice_WorkingMode_EnumDef s_pendingMode = E_IODEVICE_MODE_NOT_CONNECTED;
static uint32_t s_debounceStartTick = 0;
static uint8_t s_debounceActive = 0;
static IODevice_Channel_State_t s_CurChannel;


bool Dal_Read_Pin(GPIO_Input_EnumDef pin)
{
    switch (pin)
    {
        case E_GPIO_IN_FOOT:
            return HAL_GPIO_ReadPin((GPIO_TypeDef*)MCU_FOOT_GPIO_Port, MCU_FOOT_Pin);
        case E_GPIO_IN_SYN_US:
            return HAL_GPIO_ReadPin((GPIO_TypeDef*)IO_SYN_US_GPIO_Port, IO_SYN_US_Pin);
        case E_GPIO_IN_SYN_RF:
            return HAL_GPIO_ReadPin((GPIO_TypeDef*)IO_SYN_RF_GPIO_Port, IO_SYN_RF_Pin);
        case E_GPIO_IN_SYN_ESW:
            return HAL_GPIO_ReadPin((GPIO_TypeDef*)IO_SYN_ESW_GPIO_Port, IO_SYN_ESW_Pin);
    }
    return false;
}

void Dal_Write_Pin(GPIO_Output_EnumDef pin, uint8_t state)
{
    switch (pin)
    {
        case E_GPIO_OUT_BUZZER:
            HAL_GPIO_WritePin((GPIO_TypeDef*)MCU_Buzzer_GPIO_Port, MCU_Buzzer_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_CTR_US_RF:
            HAL_GPIO_WritePin((GPIO_TypeDef*)MCU_CTR_US_RF_GPIO_Port, MCU_CTR_US_RF_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_CTR_OUT:
            HAL_GPIO_WritePin((GPIO_TypeDef*)MCU_CTR_OUT_GPIO_Port, MCU_CTR_OUT_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_MCU_IO:
            HAL_GPIO_WritePin((GPIO_TypeDef*)MCU_I_O_GPIO_Port, MCU_I_O_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_PWR_CTRL1:
            HAL_GPIO_WritePin((GPIO_TypeDef*)pwr_control1_GPIO_Port, pwr_control1_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_PWR_CTRL2:
            HAL_GPIO_WritePin((GPIO_TypeDef*)pwr_control2_GPIO_Port, pwr_control2_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_PWR_CTRL3:
            HAL_GPIO_WritePin((GPIO_TypeDef*)pwr_control3_GPIO_Port, pwr_control3_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_PWR_CTRL4:
            HAL_GPIO_WritePin((GPIO_TypeDef*)pwr_control4_GPIO_Port, pwr_control4_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_CTR_FAN:
            HAL_GPIO_WritePin((GPIO_TypeDef*)CTR_FAN_GPIO_Port, CTR_FAN_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_CTR_HP_MOTOR:
            HAL_GPIO_WritePin((GPIO_TypeDef*)CTR_HP_motor_GPIO_Port, CTR_HP_motor_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_CTR_HP_LOSE:
            HAL_GPIO_WritePin((GPIO_TypeDef*)CTR_HP_lose_GPIO_Port, CTR_HP_lose_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_CTR_HEAT_HP:
            HAL_GPIO_WritePin((GPIO_TypeDef*)CTR_HEAT_HP_GPIO_Port, CTR_HEAT_HP_Pin, (GPIO_PinState)state);
            break;
        default:
            break;
    }
}



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
    us_state = Dal_Read_Pin(E_GPIO_IN_SYN_US);
    
    // Read ESW signal - if IO_SYN_ESW is not defined, use IO_SYN_RFC12 as alternative
    esw_state = Dal_Read_Pin(E_GPIO_IN_SYN_ESW);
    
    // Read RF signal
    rf_state = Dal_Read_Pin(E_GPIO_IN_SYN_RF);
    
    // Convert GPIO_PinState to boolean (GPIO_PIN_SET = High, GPIO_PIN_RESET = Low)
    pSignals->us = (us_state == GPIO_PIN_SET) ? 1 : 0;
    pSignals->esw = (esw_state == GPIO_PIN_SET) ? 1 : 0;
    pSignals->rf = (rf_state == GPIO_PIN_SET) ? 1 : 0;
}

/**
 * @brief Decode working mode from IO synchronization signals (no debounce)
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
 * @retval IODevice_WorkingMode_EnumDef The working mode state (raw, no debounce)
 */
static IODevice_WorkingMode_EnumDef Drv_IODevice_DecodeWorkingMode(const IODevice_SyncSignals_t *pSignals)
{
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

/**
 * @brief Get working mode based on IO synchronization signals with debounce
 * 
 * This function applies debounce logic to prevent mode flickering caused by 
 * signal noise during probe connection/disconnection. The mode will only 
 * change after the new mode has been stable for IODEVICE_DEBOUNCE_TIME_MS.
 * 
 * @param pSignals Pointer to structure containing the signal states
 * @retval IODevice_WorkingMode_EnumDef The working mode state (debounced)
 */
IODevice_WorkingMode_EnumDef Drv_IODevice_GetWorkingMode(const IODevice_SyncSignals_t *pSignals)
{
    IODevice_WorkingMode_EnumDef currentMode;
    uint32_t currentTick;
    
    if (pSignals == NULL)
    {
        return E_IODEVICE_MODE_ERROR;
    }
    
    // Decode the current mode from signals
    currentMode = Drv_IODevice_DecodeWorkingMode(pSignals);
    currentTick = HAL_GetTick();
    
    // If current mode matches the last stable mode, reset debounce
    if (currentMode == s_lastStableMode)
    {
        s_debounceActive = 0;
        return s_lastStableMode;
    }
    
    // If this is a new pending mode, start debounce timer
    if (!s_debounceActive || currentMode != s_pendingMode)
    {
        s_pendingMode = currentMode;
        s_debounceStartTick = currentTick;
        s_debounceActive = 1;
        return s_lastStableMode;
    }
    
    // Check if debounce time has elapsed
    if ((currentTick - s_debounceStartTick) >= IODEVICE_DEBOUNCE_TIME_MS)
    {
        // Mode has been stable for debounce period, update stable mode
        s_lastStableMode = s_pendingMode;
        s_debounceActive = 0;
    }
    
    return s_lastStableMode;
}


IODevice_WorkingMode_EnumDef Drv_IODevice_GetProbeStatus(void)
{
    IODevice_SyncSignals_t s_SyncSignals;
    Drv_IODevice_ReadSyncSignals(&s_SyncSignals);
    return Drv_IODevice_GetWorkingMode(&s_SyncSignals);
}


void Drv_IODevice_ChangeChannel(IODevice_Channel_EnumDef channel)
{
    s_CurChannel = channel;
    switch (channel)
    {
        case CHANNEL_US:
            Dal_Write_Pin(E_GPIO_OUT_CTR_US_RF, 1);
            Dal_Write_Pin(E_GPIO_OUT_CTR_OUT, 1);
            break;
        case CHANNEL_ESW:
            Dal_Write_Pin(E_GPIO_OUT_CTR_ESW, 1);
            break;
        case CHANNEL_HP:
            Dal_Write_Pin(E_GPIO_OUT_CTR_HP, 1);
            break;
        case CHANNEL_RF:
            Dal_Write_Pin(E_GPIO_OUT_CTR_RF, 1);
            break;
        default:
            break;
    }
}


/**************************End of file********************************/
