/************************************************************************************
 * @file     : bsp_SI5351.h
 * @brief    : SI5351 clock generator BSP driver header
 * @details  : This file contains all the function prototypes, type definitions,
 *             and constants for the bsp_SI5351.c file
 * @author   : Refactored from original implementation
 * @date     : 2025-01-25
 * @version  : V1.0.0
 * @copyright: Copyright (c) 2025
 ***********************************************************************************/

#ifndef _BSP_SI5351_H_
#define _BSP_SI5351_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Macro Definitions ==================== */

#define	FREQ_PLL			900000000	/* PLL frequency: 900 MHz */
#define XTAL_FREQ			25000000	/* Crystal frequency: 25 MHz */
#define	DENOM_20BIT			1048575		/* 20-bit denominator for fractional calculations */

#define SI5351A_REVB_REG_CONFIG_NUM_REGS				43

/* Register definitions */
#define SI_CLK0_CONTROL		0x10
#define SI_CLK1_CONTROL		0x11
#define SI_CLK2_CONTROL		0x12

#define SI_SYNTH_PLL_A		0x1A
#define SI_SYNTH_PLL_B		0x22

#define SI_SYNTH_MS_0		0x2A
#define SI_SYNTH_MS_1		0x32
#define SI_SYNTH_MS_2		0x3A

#define SI_PLL_RESET		0xB1
#define DEFAULT_FREQUENCY   840		/* Default frequency: 840 kHz */

/* R-division ratio definitions */
#define SI_R_DIV_1			0x00
#define SI_R_DIV_2			0b00010000
#define SI_R_DIV_4			0b00100000
#define SI_R_DIV_8			0b00110000
#define SI_R_DIV_16			0b01000000
#define SI_R_DIV_32			0b01010000
#define SI_R_DIV_64			0b01100000
#define SI_R_DIV_128		0b01110000

#define SI_CLK_SRC_PLL_A	0x00
#define SI_CLK_SRC_PLL_B	0x20

#define SI_POWEROFF			0x80
#define SI_PLLX_RESET		0xA0

/* ==================== Type Definitions ==================== */

/**
 * @brief SI5351 error enumeration
 */
typedef enum
{
	ERROR_SI5351_OK,		/* Success */
	ERROR_SI5351_CH,		/* Channel out of range */
	ERROR_SI5351_FREQ		/* Frequency out of range */
}SI5351_ERROR_EnumDef;

/**
 * @brief SI5351 clock source enumeration
 */
typedef enum
{
	SOURCE_SI5351_SELF,		/* Use SI5351 to directly output PWM */
	SOURCE_SI5351_TIM		/* Use SI5351 as external clock, output to TIM ETR */
}SI5351_SOURCE_EnumDef;

/**
 * @brief SI5351 frequency configuration structure
 * @note Used for calculating Synthesis register values
 */
typedef struct
{
	uint32_t pll_freq;			/* PLL frequency */
	uint32_t freq_out;			/* Target output frequency */
	SI5351_SOURCE_EnumDef which_source;	/* Clock source selection */
}SI5351A_FREQ_TypeDef;

/**
 * @brief SI5351 parameter structure
 */
typedef struct
{
	SI5351_ERROR_EnumDef err;
	uint32_t max_freq;		/* Maximum allowed frequency value */
	uint32_t min_freq;		/* Minimum allowed frequency value */
	uint8_t max_ch;			/* Maximum channel number */
	uint8_t min_ch;			/* Minimum channel number */
	SI5351A_FREQ_TypeDef St_freq;
}SI5351A_PARA_TypeDef;

/**
 * @brief SI5351 register data structure
 */
typedef struct
{
	uint8_t reg_pll[8];		/* PLL register values */
	uint8_t reg_synth[8];	/* Multi-synth register values */
}SI5351A_DATA_TypeDef;

/**
 * @brief SI5351 main structure
 */
typedef struct
{
	SI5351A_DATA_TypeDef data;
	SI5351A_PARA_TypeDef para;
}SI5351A_TypeDef;

/**
 * @brief SI5351 register bit enumeration
 */
typedef enum
{
	BIT_CLK0_CONTROL = 5,
	BIT_CLK1_CONTROL = 6,
	BIT_CLK2_CONTROL = 7,
	BIT_SYNTH_PLL_A = 13,
	BIT_SYNTH_MS_0 = 21
}SI5351_BIT_EnumDef;

/**
 * @brief SI5351 fraction structure for divider calculations
 * @note quotient = int_quotient + numerator/denominator
 */
typedef struct
{
	double quotient;			/* Quotient */
	uint32_t int_quotient;	/* Integer quotient */
	uint32_t numerator;		/* Numerator */
	uint32_t denominator;	/* Denominator */
}SI5351A_FRACTION_TypeDef;

/* ==================== Function Prototypes ==================== */

void Si5351_Init(void);
void Si5351_Open_CLK1(uint32_t freq);
void Si5351_StopPWM(void);
void Si5351_SetFrequency_ALL_RESET(void);
void Si5351_CLK0_OUT(void);
void Si5351_CLK1_OUT(void);
void Si5351_PWM_TIM(uint32_t freq);

int8_t Si5351_SetFrequency(uint8_t ch, uint32_t frequency);
int8_t Si5351_SetFrequency_Pro(uint8_t ch, uint32_t frequency);

void SetPLLClk(uint8_t pll, uint8_t mult, uint32_t num, uint32_t denom);
void SetMultisynth(uint8_t synth, uint32_t divider, uint8_t rDiv);
void SetMultisynth_all(uint8_t synth_reg, uint32_t a, uint32_t b, uint32_t c, uint8_t rDiv);
void Si5351_GetPara(SI5351A_FRACTION_TypeDef *para, SI5351A_FREQ_TypeDef freq);

void PWM_Generate_Config(void);
void PWM_Generate(uint32_t freq);
void TIM1_PWM_STOP(void);

#ifdef __cplusplus
}
#endif

#endif /* _BSP_SI5351_H_ */

/**************************End of file********************************/
