/************************************************************************************
 * @file     : drv_delay.h
 * @brief    : Delay/tick driver - DRV API, DAL calls BSP (Std lib)
 ***********************************************************************************/
#ifndef DRV_DELAY_H
#define DRV_DELAY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYSTEM_TICK_PER_SECOND  100u  /* us per Drv_SysTick_Increment (if used) */

void Dal_Delay(uint32_t ms);   /* DAL: calls BSP_Delay_ms; only used inside DRV */
uint32_t Dal_GetTick(void);    /* DAL: calls BSP_GetTick_ms; only used inside DRV */

void Drv_SysTick_Increment(void);
uint64_t Drv_GetSystemTickUs(void);
uint64_t Drv_GetSystemTickMs(void);
uint32_t Drv_Delay_GetTickMs(void);   /* for APP: ms since boot (BSP tick) */

typedef struct {
    uint32_t start_ms;
    uint32_t timeout_ms;
    bool     running;
} Drv_Timer_t;

bool Drv_Timer_Tick(Drv_Timer_t *pTimer, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* DRV_DELAY_H */
