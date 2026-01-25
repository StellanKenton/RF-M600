/************************************************************************************
 * @file     : drv_iodevice.h
 * @brief    : IO device driver - DRV API, DAL calls BSP (Std lib)
 ***********************************************************************************/
#ifndef DRV_IODEVICE_H
#define DRV_IODEVICE_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    E_GPIO_OUT_BUZZER = 0,
    E_GPIO_OUT_CTR_US_RF,
    E_GPIO_OUT_CTR_OUT,
    E_GPIO_OUT_MCU_IO,
    E_GPIO_OUT_PWR_CTRL1,
    E_GPIO_OUT_PWR_CTRL2,
    E_GPIO_OUT_PWR_CTRL3,
    E_GPIO_OUT_PWR_CTRL4,
    E_GPIO_OUT_CTR_FAN,
    E_GPIO_OUT_CTR_HP_MOTOR,
    E_GPIO_OUT_CTR_HP_LOSE,
    E_GPIO_OUT_CTR_HEAT_HP,
    E_GPIO_OUT_MAX
} GPIO_Output_EnumDef;

typedef enum {
    E_GPIO_IN_FOOT = 0,
    E_GPIO_IN_SYN_US,
    E_GPIO_IN_SYN_RF,
    E_GPIO_IN_SYN_ESW,
    E_GPIO_IN_MAX
} GPIO_Input_EnumDef;

typedef enum {
    E_IODEVICE_MODE_ULTRASOUND = 0,
    E_IODEVICE_MODE_SHOCKWAVE,
    E_IODEVICE_MODE_RADIO_FREQUENCY,
    E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT,
    E_IODEVICE_MODE_NOT_CONNECTED,
    E_IODEVICE_MODE_ERROR,
} IODevice_WorkingMode_EnumDef;

typedef enum {
    CHANNEL_US = 0,
    CHANNEL_SW,
    CHANNEL_RF,
    CHANNEL_NH,
    CHANNEL_CLOSE,
    CHANNEL_READY,
    CHANNEL_MAX,
} IODevice_Channel_EnumDef;

typedef struct {
    IODevice_Channel_EnumDef channel;
    uint8_t state;
} IODevice_Channel_State_t;

typedef struct {
    uint8_t us;
    uint8_t esw;
    uint8_t rf;
} IODevice_SyncSignals_t;

void Drv_IODevice_ReadSyncSignals(IODevice_SyncSignals_t *pSignals);
IODevice_WorkingMode_EnumDef Drv_IODevice_GetWorkingMode(const IODevice_SyncSignals_t *pSignals);
IODevice_WorkingMode_EnumDef Drv_IODevice_GetProbeStatus(void);
void Drv_IODevice_WritePin(GPIO_Output_EnumDef pin, uint8_t state);
bool Drv_IODevice_GetFootSwitchState(void);
void Drv_IODevice_ChangeChannel(IODevice_Channel_EnumDef channel);
void Drv_IODevice_StartBuzzer(uint32_t duration_ms);
void Drv_IODevice_ProcessBuzzer(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_IODEVICE_H */
