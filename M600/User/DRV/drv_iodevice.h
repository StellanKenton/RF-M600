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
#include <stdbool.h>
#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif

/**
 * @brief GPIO Output pins enumeration (输出引脚枚举)
 */
 typedef enum
 {
     E_GPIO_OUT_BUZZER = 0,           ///< MCU_Buzzer - 蜂鸣器
     E_GPIO_OUT_CTR_US_RF,            ///< MCU_CTR_US_RF - 超声/射频控制 (PB12)
     E_GPIO_OUT_CTR_OUT,              ///< MCU_CTR_OUT - 输出控制 (PB12)
     E_GPIO_OUT_MCU_IO,               ///< MCU_I_O - IO控制 (PB15)
     E_GPIO_OUT_PWR_CTRL1,            ///< pwr_control1 - 功率控制1
     E_GPIO_OUT_PWR_CTRL2,            ///< pwr_control2 - 功率控制2
     E_GPIO_OUT_PWR_CTRL3,            ///< pwr_control3 - 功率控制3
     E_GPIO_OUT_PWR_CTRL4,            ///< pwr_control4 - 功率控制4
     E_GPIO_OUT_CTR_FAN,              ///< CTR_FAN - 风扇控制
     E_GPIO_OUT_CTR_HP_MOTOR,         ///< CTR_HP_motor - HP电机控制
     E_GPIO_OUT_CTR_HP_LOSE,          ///< CTR_HP_lose - HP释放控制
     E_GPIO_OUT_CTR_HEAT_HP,          ///< CTR_HEAT_HP - HP加热控制
     E_GPIO_OUT_MAX                   ///< 输出引脚数量
 } GPIO_Output_EnumDef;
 
 /**
  * @brief GPIO Input pins enumeration (输入引脚枚举)
  */
 typedef enum
 {
     E_GPIO_IN_FOOT = 0,              ///< MCU_FOOT - 脚踏开关
     E_GPIO_IN_SYN_US,                ///< IO_SYN_US - 超声同步信号
     E_GPIO_IN_SYN_RF,                ///< IO_SYN_RF - 射频同步信号
     E_GPIO_IN_SYN_ESW,               ///< IO_SYN_ESW - 冲击波同步信号
     E_GPIO_IN_MAX                    ///< 输入引脚数量
 } GPIO_Input_EnumDef;



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
    CHANNEL_SW,
    CHANNEL_RF,
    CHANNEL_NH,
    CHANNEL_CLOSE,
    CHANNEL_READY,
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

/**
 * @brief Read GPIO pin state
 * @param pin The GPIO pin to read
 * @retval The state of the GPIO pin (0=Low, 1=High)
 */
bool Dal_Read_Pin(GPIO_Input_EnumDef pin);

/**
 * @brief Write GPIO pin state
 * @param pin The GPIO pin to write
 * @param state The state of the GPIO pin (0=Low, 1=High)
 */
void Dal_Write_Pin(GPIO_Output_EnumDef pin, uint8_t state);

/**
 * @brief Get foot switch state
 * @retval The state of the foot switch (0=Open, 1=Closed)
 */
bool Drv_IODevice_GetFootSwitchState(void);
#ifdef __cplusplus
}
#endif
#endif  // DRV_IODEVICE_H
/**************************End of file********************************/
