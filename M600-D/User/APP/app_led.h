/************************************************************************************
* @file     : app_led.h
* @brief    : LED status indicator module
* @details  : Displays different LED patterns based on system status
* @author   : \.rumi
* @date     : 2025-03-08
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef APP_LED_H
#define APP_LED_H

#include <string.h>
#include <stdbool.h>
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LED_TASK_TIME 100 // 100ms
/**
 * @brief LED pattern types
 */
typedef enum {
    E_LED_PATTERN_OFF = 0,          ///< LED off
    E_LED_PATTERN_ON,               ///< LED always on
    E_LED_PATTERN_BREATHING,        ///< Breathing effect (1s period)
    E_LED_PATTERN_SLOW_BLINK,       ///< Slow blink (2s period)
    E_LED_PATTERN_NORMAL_BLINK,     ///< Normal blink (1s on, 1s off)
    E_LED_PATTERN_FAST_BLINK,       ///< Fast blink (200ms period)
    E_LED_PATTERN_MAX
} LED_Pattern_EnumDef;

/**
 * @brief Initialize LED module
 */
void App_LED_Init(void);

/**
 * @brief Process LED state machine (call periodically)
 */
void App_LED_Process(void);

/**
 * @brief Set LED pattern manually
 * @param pattern LED pattern to set
 */
void App_LED_SetPattern(LED_Pattern_EnumDef pattern);

/**
 * @brief Get current LED pattern
 * @retval Current LED pattern
 */
LED_Pattern_EnumDef App_LED_GetPattern(void);

#ifdef __cplusplus
}
#endif
#endif  // APP_LED_H
/**************************End of file********************************/

