/***********************************************************************************
* @file     : app_led.c
* @brief    : LED status indicator module implementation
* @details  : Displays different LED patterns based on system status
* @author   : \.rumi
* @date     : 2025-03-08
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "app_led.h"
#include "app_system.h"
#include "drv_iodevice.h"
#include "bsp_delay.h"
#include <math.h>
#include "drv_delay.h"
/* LED timing constants (in milliseconds) */
#define LED_BREATHING_PERIOD_MS     1000    ///< Breathing effect period
#define LED_SLOW_BLINK_PERIOD_MS    2000    ///< Slow blink period
#define LED_NORMAL_BLINK_PERIOD_MS  2000    ///< Normal blink period (1s on, 1s off)
#define LED_FAST_BLINK_PERIOD_MS    200     ///< Fast blink period

/* PWM resolution for breathing effect */
#define LED_PWM_STEPS               100     ///< Number of PWM steps for breathing

/**
 * @brief LED manager structure
 */
typedef struct {
    LED_Pattern_EnumDef pattern;        ///< Current LED pattern
    uint32_t lastUpdateTick;            ///< Last update timestamp
    uint16_t cycleCounter;              ///< Cycle counter for pattern generation
    uint8_t brightness;                 ///< Current brightness (0-100)
} LED_Manager_t;

/* Static LED manager instance */
static LED_Manager_t s_LEDMgr;

/**
 * @brief Calculate breathing brightness using sine wave
 * @param phase Phase in cycle (0-999 for 1000ms period)
 * @retval Brightness value (0-100)
 */
static uint8_t LED_CalculateBreathingBrightness(uint16_t phase)
{
    /* Use sine wave for smooth breathing effect */
    float angle = (float)phase * 2.0f * 3.14159f / LED_BREATHING_PERIOD_MS;
    float sine_val = sinf(angle);
    /* Map sine wave (-1 to 1) to brightness (0 to 100) */
    uint8_t brightness = (uint8_t)((sine_val + 1.0f) * 50.0f);
    return brightness;
}

/**
 * @brief Update LED output based on current brightness
 * @param brightness Brightness value (0-100)
 */
static void LED_UpdateOutput(uint8_t brightness)
{
    /* For simple on/off control, use threshold */
    /* For breathing effect, could implement hardware PWM */
    if (brightness > 50) {
        Drv_IODevice_WritePin(E_GPIO_OUT_LED, 1);  // LED ON
    } else {
        Drv_IODevice_WritePin(E_GPIO_OUT_LED, 0);  // LED OFF
    }
}

/**
 * @brief Update LED pattern based on system mode
 */
static void LED_UpdatePatternFromSystemMode(void)
{
    LED_Pattern_EnumDef newPattern;
    System_Mode_EnumDef systemMode = System_GetMode();
    
    switch (systemMode) {
        case E_SYSTEM_NORMAL_MODE:
            newPattern = E_LED_PATTERN_NORMAL_BLINK;  // 1s on, 1s off blink
            break;
            
        case E_SYSTEM_STANDBY_MODE:
            newPattern = E_LED_PATTERN_SLOW_BLINK;
            break;
            
        case E_SYSTEM_UPDATE_MODE:
            newPattern = E_LED_PATTERN_FAST_BLINK;
            break;
            
        default:
            newPattern = E_LED_PATTERN_OFF;
            break;
    }
    
    /* Update pattern if changed */
    if (s_LEDMgr.pattern != newPattern) {
        s_LEDMgr.pattern = newPattern;
        s_LEDMgr.cycleCounter = 0;
        s_LEDMgr.lastUpdateTick = BSP_GetTick_ms();
    }
}

/**
 * @brief Process breathing pattern
 */
static void LED_ProcessBreathing(void)
{
    uint32_t sysTick = BSP_GetTick_ms();
    uint32_t elapsed = sysTick - s_LEDMgr.lastUpdateTick;
    uint16_t phase = (uint16_t)(elapsed % LED_BREATHING_PERIOD_MS);
    
    s_LEDMgr.brightness = LED_CalculateBreathingBrightness(phase);
    LED_UpdateOutput(s_LEDMgr.brightness);
}

/**
 * @brief Process slow blink pattern
 */
static void LED_ProcessSlowBlink(void)
{
    uint32_t sysTick = BSP_GetTick_ms();
    uint32_t elapsed = sysTick - s_LEDMgr.lastUpdateTick;
    uint16_t phase = (uint16_t)(elapsed % LED_SLOW_BLINK_PERIOD_MS);
    
    /* 50% duty cycle */
    if (phase < LED_SLOW_BLINK_PERIOD_MS / 2) {
        s_LEDMgr.brightness = 100;
    } else {
        s_LEDMgr.brightness = 0;
    }
    
    LED_UpdateOutput(s_LEDMgr.brightness);
}

/**
 * @brief Process normal blink pattern (1s on, 1s off)
 */
static void LED_ProcessNormalBlink(void)
{
    uint32_t sysTick = BSP_GetTick_ms();
    uint32_t elapsed = sysTick - s_LEDMgr.lastUpdateTick;
    uint16_t phase = (uint16_t)(elapsed % LED_NORMAL_BLINK_PERIOD_MS);
    
    /* 50% duty cycle */
    if (phase < LED_NORMAL_BLINK_PERIOD_MS / 2) {
        s_LEDMgr.brightness = 100;
    } else {
        s_LEDMgr.brightness = 0;
    }
    
    LED_UpdateOutput(s_LEDMgr.brightness);
}

/**
 * @brief Process fast blink pattern
 */
static void LED_ProcessFastBlink(void)
{
    uint32_t sysTick = BSP_GetTick_ms();
    uint32_t elapsed = sysTick - s_LEDMgr.lastUpdateTick;
    uint16_t phase = (uint16_t)(elapsed % LED_FAST_BLINK_PERIOD_MS);
    
    /* 50% duty cycle */
    if (phase < LED_FAST_BLINK_PERIOD_MS / 2) {
        s_LEDMgr.brightness = 100;
    } else {
        s_LEDMgr.brightness = 0;
    }
    
    LED_UpdateOutput(s_LEDMgr.brightness);
}

/**
 * @brief Initialize LED module
 */
void App_LED_Init(void)
{
    /* Initialize LED manager structure */
    s_LEDMgr.pattern = E_LED_PATTERN_OFF;
    s_LEDMgr.lastUpdateTick = 0;
    s_LEDMgr.cycleCounter = 0;
    s_LEDMgr.brightness = 0;
    
    /* Turn off LED initially */
    Drv_IODevice_WritePin(E_GPIO_OUT_LED, 0);
}

/**
 * @brief Process LED state machine (call periodically)
 */
void App_LED_Process(void)
{
    static Drv_Timer_t CommTimer;
    if(Drv_Timer_Tick(&CommTimer, LED_TASK_TIME) == false){
        return;
    }
    /* Update pattern based on system mode */
    LED_UpdatePatternFromSystemMode();
    
    /* Process current pattern */
    switch (s_LEDMgr.pattern) {
        case E_LED_PATTERN_OFF:
            s_LEDMgr.brightness = 0;
            LED_UpdateOutput(0);
            break;
            
        case E_LED_PATTERN_ON:
            s_LEDMgr.brightness = 100;
            LED_UpdateOutput(100);
            break;
            
        case E_LED_PATTERN_BREATHING:
            LED_ProcessBreathing();
            break;
            
        case E_LED_PATTERN_SLOW_BLINK:
            LED_ProcessSlowBlink();
            break;
            
        case E_LED_PATTERN_NORMAL_BLINK:
            LED_ProcessNormalBlink();
            break;
            
        case E_LED_PATTERN_FAST_BLINK:
            LED_ProcessFastBlink();
            break;
            
        default:
            LED_UpdateOutput(0);
            break;
    }
}

/**
 * @brief Set LED pattern manually
 * @param pattern LED pattern to set
 */
void App_LED_SetPattern(LED_Pattern_EnumDef pattern)
{
    if (pattern < E_LED_PATTERN_MAX) {
        s_LEDMgr.pattern = pattern;
        s_LEDMgr.cycleCounter = 0;
        s_LEDMgr.lastUpdateTick = BSP_GetTick_ms();
    }
}

/**
 * @brief Get current LED pattern
 * @retval Current LED pattern
 */
LED_Pattern_EnumDef App_LED_GetPattern(void)
{
    return s_LEDMgr.pattern;
}

/**************************End of file********************************/

