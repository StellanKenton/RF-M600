/************************************************************************************
 * @file     : bsp_gpio.c
 * @brief    : M600 GPIO init - pins from M600 gpio.c (STM32 Standard Library)
 * @details  : Outputs: Buzzer, pwr_control1~4, CTR_OUT, CTR_US_RF, HP_motor/lose/HEAT, FAN.
 *             Inputs: MCU_FOOT, IO_SYN_US/RF/ESW, MCU_I_O.
 ***********************************************************************************/
#include "bsp_gpio.h"
#include "bsp_adc.h"
#include "bsp_tim.h"
#include "bsp_usart.h"
#include "bsp_dac.h"
#include "bsp_i2c.h"
#include "bsp_iwdg.h"
#include "bsp_delay.h"

void BSP_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable PWR and BKP clocks, let PC13/PC14/PC15 work as GPIO */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);
    BKP_TamperPinCmd(DISABLE);
    /* Disable the LSE oscillator (PC14, PC15) */
    RCC_LSEConfig(RCC_LSE_OFF);

    /* GPIO clocks */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
                           RCC_APB2Periph_AFIO, ENABLE);

    /* Disable JTAG and keep SWD so PA15/PB3/PB4 can be used as normal GPIOs. */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    /* --- Set initial output levels LOW before configuring as outputs --- */
    /* GPIOC outputs: Buzzer(PC13), pwr_control5(PC12), CTR_HP_motor(PC7),
                      CTR_HP_lose(PC8), CTR_FAN(PC6), LED(PC15) */
    GPIO_ResetBits(GPIOC, MCU_Buzzer_Pin | pwr_control5_Pin |
                          CTR_HP_motor_Pin | CTR_HP_lose_Pin |
                          CTR_FAN_Pin | LED_PIN);
    /* GPIOB outputs: pwr_control1(PB5), pwr_control2(PB4), pwr_control3(PB3),
                      CTR_HEAT_HP(PB5-note: same pin as pwr_control1 per .h),
                      MCU_CTR_US_RF(PB12), MCU_CTR_OUT(PB14),
                      ESW_P(PB8), ESW_N(PB9) */
    GPIO_ResetBits(GPIOB, pwr_control1_Pin | pwr_control2_Pin | pwr_control3_Pin |
                          CTR_FAN_HP_Pin | MCU_CTR_US_RF_Pin | MCU_CTR_OUT_Pin |
                          ESW_P_Pin | ESW_N_Pin);
    /* GPIOD output: pwr_control4(PD2) */
    GPIO_ResetBits(GPIOD, pwr_control4_Pin);

    /* --- GPIOC outputs ---
       PC6  : CTR_FAN
       PC7  : CTR_HP_motor
       PC8  : CTR_HP_lose
       PC12 : pwr_control5
       PC13 : MCU_Buzzer
       PC15 : LED */
    GPIO_InitStructure.GPIO_Pin   = CTR_FAN_Pin | CTR_HP_motor_Pin | CTR_HP_lose_Pin |
                                    pwr_control5_Pin | MCU_Buzzer_Pin | LED_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

     /* --- GPIOC inputs ---
         PC1  : MCU_I_O
         PC10 : IO_SYN_RF
         PC11 : IO_SYN_ESW
         PC14 : MCU_FOOT */
     GPIO_InitStructure.GPIO_Pin  = MCU_I_O_Pin | IO_SYN_RF_Pin |
                                              IO_SYN_ESW_Pin | MCU_FOOT_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

     /* --- GPIOA input ---
         PA15 : IO_SYN_US */
     GPIO_InitStructure.GPIO_Pin  = IO_SYN_US_Pin;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
     GPIO_Init(IO_SYN_US_Port, &GPIO_InitStructure);

    /* --- GPIOB outputs ---
       PB3  : pwr_control3
       PB4  : pwr_control2
       PB5  : pwr_control1 / CTR_HEAT_HP (same pin per .h)
       PB8  : ESW_P
       PB9  : ESW_N
       PB12 : MCU_CTR_US_RF
       PB14 : MCU_CTR_OUT */
    GPIO_InitStructure.GPIO_Pin   = pwr_control3_Pin | pwr_control2_Pin | pwr_control1_Pin |
                                    CTR_FAN_HP_Pin  | ESW_P_Pin | ESW_N_Pin |
                                    MCU_CTR_US_RF_Pin | MCU_CTR_OUT_Pin;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* --- GPIOD output ---
       PD2 : pwr_control4 */
    GPIO_InitStructure.GPIO_Pin   = pwr_control4_Pin;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    /* --- Pull all output pins LOW after init --- */
    GPIO_ResetBits(GPIOC, CTR_FAN_Pin | CTR_HP_motor_Pin | CTR_HP_lose_Pin |
                          pwr_control5_Pin | MCU_Buzzer_Pin | LED_PIN);
    GPIO_ResetBits(GPIOB, pwr_control3_Pin | pwr_control2_Pin | pwr_control1_Pin |
                          CTR_FAN_HP_Pin  | ESW_P_Pin | ESW_N_Pin |
                          MCU_CTR_US_RF_Pin | MCU_CTR_OUT_Pin);
    GPIO_ResetBits(GPIOD, pwr_control4_Pin);
}

uint8_t BSP_GPIO_ReadPin(GPIO_Input_EnumDef pin)
{
    GPIO_TypeDef *port;
    uint16_t gpio_pin;
    switch (pin) {
        case E_GPIO_IN_FOOT:    port = MCU_FOOT_Port;    gpio_pin = MCU_FOOT_Pin;    break;
        case E_GPIO_IN_SYN_US:  port = IO_SYN_US_Port;   gpio_pin = IO_SYN_US_Pin;   break;
        case E_GPIO_IN_SYN_RF:  port = IO_SYN_RF_Port;   gpio_pin = IO_SYN_RF_Pin;   break;
        case E_GPIO_IN_SYN_ESW: port = IO_SYN_ESW_Port;  gpio_pin = IO_SYN_ESW_Pin;  break;
        default: return 0;
    }
    return GPIO_ReadInputDataBit(port, gpio_pin) ? 1 : 0;
}

void BSP_GPIO_WritePin(GPIO_Output_EnumDef pin, uint8_t state)
{
    GPIO_TypeDef *port;
    uint16_t gpio_pin;
    switch (pin) {
        case E_GPIO_OUT_BUZZER:      port = MCU_Buzzer_Port;     gpio_pin = MCU_Buzzer_Pin;     break;
        case E_GPIO_OUT_CTR_US_RF:   port = MCU_CTR_US_RF_Port;  gpio_pin = MCU_CTR_US_RF_Pin;  break;
        case E_GPIO_OUT_CTR_OUT:     port = MCU_CTR_OUT_Port;    gpio_pin = MCU_CTR_OUT_Pin;    break;
        case E_GPIO_OUT_MCU_IO:      port = MCU_I_O_Port;        gpio_pin = MCU_I_O_Pin;        break;
        case E_GPIO_OUT_PWR_CTRL1:   port = pwr_control1_Port;   gpio_pin = pwr_control1_Pin;   break;
        case E_GPIO_OUT_PWR_CTRL2:   port = pwr_control2_Port;   gpio_pin = pwr_control2_Pin;   break;
        case E_GPIO_OUT_PWR_CTRL3:   port = pwr_control3_Port;   gpio_pin = pwr_control3_Pin;   break;
        case E_GPIO_OUT_PWR_CTRL4:   port = pwr_control4_Port;   gpio_pin = pwr_control4_Pin;   break;
        case E_GPIO_OUT_PWR_CTRL5:   port = pwr_control5_Port;   gpio_pin = pwr_control5_Pin;   break;
        case E_GPIO_OUT_CTR_FAN:     port = CTR_FAN_Port;        gpio_pin = CTR_FAN_Pin;        break;
        case E_GPIO_OUT_CTR_HP_MOTOR: port = CTR_HP_motor_Port;  gpio_pin = CTR_HP_motor_Pin;   break;
        case E_GPIO_OUT_CTR_HP_LOSE: port = CTR_HP_lose_Port;    gpio_pin = CTR_HP_lose_Pin;    break;
        case E_GPIO_OUT_CTR_HEAT_HP: port = CTR_FAN_HP_Port;    gpio_pin = CTR_FAN_HP_Pin;    break;
        case E_GPIO_OUT_ESW_P:       port = ESW_P_Port;          gpio_pin = ESW_P_Pin;         break;
        case E_GPIO_OUT_ESW_N:       port = ESW_N_Port;          gpio_pin = ESW_N_Pin;         break;
        case E_GPIO_OUT_LED:         port = LED_PORT;        		 gpio_pin = LED_PIN;       break;
        default: return;
    }
    if (state)
        GPIO_SetBits(port, gpio_pin);
    else
        GPIO_ResetBits(port, gpio_pin);
}

void BSP_Init(void)
{
    BSP_SysTick_Init();
    BSP_GPIO_Init();
    BSP_ADC_Init();
    BSP_DAC_Init();
    BSP_TIM1_Init();
    BSP_TIM2_Init();
    BSP_USART1_Init(115200);
    BSP_USART2_Init(9600);
    //BSP_I2C1_Init();
    //BSP_I2C2_Init();
    /* BSP_IWDG_Init();  optional, enable when using IWDG */
}
