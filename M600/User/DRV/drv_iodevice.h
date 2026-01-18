/************************************************************************************
* @file     : drv_iodevice.h
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRV_IODEVICE_H
#define DRV_IODEVICE_H

#include "main.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif


/**
 * @brief IO Device working mode enumeration
 */
typedef enum
{
    E_IODEVICE_MODE_ULTRASOUND = 0,              ///< Ultrasound mode (超声)
    E_IODEVICE_MODE_SHOCKWAVE,                   ///< Shockwave mode (冲击波)
    E_IODEVICE_MODE_RADIO_FREQUENCY,             ///< Radio Frequency mode (射频)
    E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT,      ///< Negative Pressure Heat mode (负压加热)
    E_IODEVICE_MODE_NOT_CONNECTED,               ///< Treatment head not connected (治疗头未连接)
    E_IODEVICE_MODE_ERROR,                       ///< Connection error (治疗头连接报错)
} IODevice_WorkingMode_EnumDef;

typedef enum
{
    CHANNEL_US = 0,
    CHANNEL_ESW,
    CHANNEL_RF,
    CHANNEL_MAX,
} IODevice_Channel_EnumDef;

typedef struct
{
    IODevice_Channel_EnumDef channel;
    uint8_t state;
} IODevice_Channel_State_t;

/**
 * @brief IO synchronization signals state structure
 */
typedef struct
{
    uint8_t us;   ///< IO_SYN_US state (0=Low, 1=High)
    uint8_t esw;  ///< IO_SYN_ESW state (0=Low, 1=High)
    uint8_t rf;   ///< IO_SYN_RF state (0=Low, 1=High)
} IODevice_SyncSignals_t;

/**
 * @brief Read IO synchronization signals from hardware
 * @param pSignals Pointer to structure to store the signal states
 */
void Drv_IODevice_ReadSyncSignals(IODevice_SyncSignals_t *pSignals);

/**
 * @brief Get working mode based on IO synchronization signals (logic only)
 * @param pSignals Pointer to structure containing the signal states
 * @retval IODevice_WorkingMode_EnumDef The working mode state
 */
IODevice_WorkingMode_EnumDef Drv_IODevice_GetWorkingMode(const IODevice_SyncSignals_t *pSignals);

/**
 * @brief Get probe status based on IO synchronization signals
 * @retval IODevice_WorkingMode_EnumDef The probe status state
 */
IODevice_WorkingMode_EnumDef Drv_IODevice_GetProbeStatus(void);
#ifdef __cplusplus
}
#endif
#endif  // DRV_IODEVICE_H
/**************************End of file********************************/
