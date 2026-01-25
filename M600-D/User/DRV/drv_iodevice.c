/************************************************************************************
 * @file     : drv_iodevice.c
 * @brief    : IO device driver - DRV calls DAL, DAL calls BSP (Std lib)
 ***********************************************************************************/
#include "drv_iodevice.h"
#include "drv_delay.h"
#include "bsp_gpio.h"
#include <stddef.h>

#define IODEVICE_DEBOUNCE_TIME_MS  50u
#define BUZZER_DEFAULT_DURATION_MS 2000u

static IODevice_WorkingMode_EnumDef s_lastStableMode = E_IODEVICE_MODE_NOT_CONNECTED;
static IODevice_WorkingMode_EnumDef s_pendingMode = E_IODEVICE_MODE_NOT_CONNECTED;
static uint32_t s_debounceStartTick = 0;
static uint8_t s_debounceActive = 0;
static bool s_buzzerActive = false;
static uint32_t s_buzzerStartTime = 0;
static uint32_t s_buzzerDuration = 0;

/* DAL: only called from DRV; calls BSP */
static bool Dal_Read_Pin(GPIO_Input_EnumDef pin)
{
    if (pin >= E_GPIO_IN_MAX)
        return false;
    return BSP_GPIO_ReadPin((BSP_GPIO_Input_t)pin) ? true : false;
}

static void Dal_Write_Pin(GPIO_Output_EnumDef pin, uint8_t state)
{
    if (pin >= E_GPIO_OUT_MAX)
        return;
    BSP_GPIO_WritePin((BSP_GPIO_Output_t)pin, state ? 1 : 0);
}

void Drv_IODevice_WritePin(GPIO_Output_EnumDef pin, uint8_t state)
{
    Dal_Write_Pin(pin, state);
}

void Drv_IODevice_ReadSyncSignals(IODevice_SyncSignals_t *pSignals)
{
    if (pSignals == NULL)
        return;
    pSignals->us  = Dal_Read_Pin(E_GPIO_IN_SYN_US)  ? 1 : 0;
    pSignals->esw = Dal_Read_Pin(E_GPIO_IN_SYN_ESW) ? 1 : 0;
    pSignals->rf  = Dal_Read_Pin(E_GPIO_IN_SYN_RF)  ? 1 : 0;
}

static IODevice_WorkingMode_EnumDef Drv_IODevice_DecodeWorkingMode(const IODevice_SyncSignals_t *pSignals)
{
    if (pSignals->us == 0 && pSignals->esw == 1 && pSignals->rf == 1)
        return E_IODEVICE_MODE_ULTRASOUND;
    if (pSignals->us == 1 && pSignals->esw == 0 && pSignals->rf == 1)
        return E_IODEVICE_MODE_SHOCKWAVE;
    if (pSignals->us == 1 && pSignals->esw == 1 && pSignals->rf == 0)
        return E_IODEVICE_MODE_RADIO_FREQUENCY;
    if (pSignals->us == 1 && pSignals->esw == 0 && pSignals->rf == 0)
        return E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT;
    if (pSignals->us == 1 && pSignals->esw == 1 && pSignals->rf == 1)
        return E_IODEVICE_MODE_NOT_CONNECTED;
    return E_IODEVICE_MODE_ERROR;
}

IODevice_WorkingMode_EnumDef Drv_IODevice_GetWorkingMode(const IODevice_SyncSignals_t *pSignals)
{
    if (pSignals == NULL)
        return E_IODEVICE_MODE_ERROR;
    IODevice_WorkingMode_EnumDef currentMode = Drv_IODevice_DecodeWorkingMode(pSignals);
    uint32_t currentTick = Dal_GetTick();
    if (currentMode == s_lastStableMode) {
        s_debounceActive = 0;
        return s_lastStableMode;
    }
    if (!s_debounceActive || currentMode != s_pendingMode) {
        s_pendingMode = currentMode;
        s_debounceStartTick = currentTick;
        s_debounceActive = 1;
        return s_lastStableMode;
    }
    if ((currentTick - s_debounceStartTick) >= IODEVICE_DEBOUNCE_TIME_MS) {
        s_lastStableMode = s_pendingMode;
        s_debounceActive = 0;
    }
    return s_lastStableMode;
}

IODevice_WorkingMode_EnumDef Drv_IODevice_GetProbeStatus(void)
{
    IODevice_SyncSignals_t s;
    Drv_IODevice_ReadSyncSignals(&s);
    return Drv_IODevice_GetWorkingMode(&s);
}

void Drv_IODevice_ChangeChannel(IODevice_Channel_EnumDef channel)
{
    switch (channel) {
        case CHANNEL_US:
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL2, 1);
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL3, 0);
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL4, 0);
            break;
        case CHANNEL_RF:
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL2, 1);
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL3, 0);
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL4, 0);
            break;
        case CHANNEL_SW:
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL2, 0);
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL3, 1);
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL4, 1);
            break;
        case CHANNEL_NH:
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL2, 0);
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL3, 0);
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL4, 0);
            break;
        case CHANNEL_CLOSE:
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL2, 0);
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL3, 0);
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL4, 0);
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL1, 0);
            break;
        case CHANNEL_READY:
            Dal_Write_Pin(E_GPIO_OUT_PWR_CTRL1, 1);
            break;
        default:
            break;
    }
}

bool Drv_IODevice_GetFootSwitchState(void)
{
    return Dal_Read_Pin(E_GPIO_IN_FOOT);
}

void Drv_IODevice_StartBuzzer(uint32_t duration_ms)
{
    if (duration_ms == 0)
        duration_ms = BUZZER_DEFAULT_DURATION_MS;
    Dal_Write_Pin(E_GPIO_OUT_BUZZER, 1);
    s_buzzerActive = true;
    s_buzzerStartTime = Dal_GetTick();
    s_buzzerDuration = duration_ms;
}

void Drv_IODevice_ProcessBuzzer(void)
{
    if (!s_buzzerActive)
        return;
    uint32_t now = Dal_GetTick();
    if ((now - s_buzzerStartTime) >= s_buzzerDuration) {
        Dal_Write_Pin(E_GPIO_OUT_BUZZER, 0);
        s_buzzerActive = false;
    }
}
