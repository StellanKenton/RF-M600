/************************************************************************************
 * @file     : bsp_SI5351.c
 * @brief    : SI5351 clock generator BSP driver
 * @details  : This file provides functions to configure and control the SI5351
 *             clock generator chip via software I2C. Supports frequency synthesis,
 *             PLL configuration, and multi-synth divider settings.
 * @author   : Refactored from original implementation
 * @date     : 2025-01-25
 * @version  : V1.0.0
 * @copyright: Copyright (c) 2025
 ***********************************************************************************/

#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_pwr.h"
#include "bsp_SI5351.h"
#include "drv_soft_i2c.h"
#include "delay.h"
#include "math.h"
#include <string.h>

/* ==================== Private Variables ==================== */

SI5351A_TypeDef St_SI5351A;
SI5351A_FRACTION_TypeDef St_fraction = {0};

/* ==================== Private Definitions ==================== */

/* SI5351 I2C Configuration */
#define SI5351_I2C_ADDR        0x60    /* SI5351 I2C device address (7-bit) */
#define SI5351_I2C_INSTANCE    DRV_SOFT_I2C_INSTANCE_1  /* Use I2C instance 1 */

/* ==================== Private Types ==================== */

typedef struct
{
	unsigned int address; /* 16-bit register address */
	unsigned char value;   /* 8-bit register data */
} si5351a_revb_register_t;

/* ==================== Private Data ==================== */

si5351a_revb_register_t const si5351a_1000_CLK0_registers[SI5351A_REVB_REG_CONFIG_NUM_REGS] =
{
    { 0x0002, 0x53 },
    { 0x0003, 0x00 },
    { 0x0004, 0x20 },
    { 0x0007, 0x00 },
    { 0x000F, 0x00 },
    { 0x0010, 0x0F },
    { 0x0011, 0x8C },
    { 0x0012, 0x8C },
    { 0x0013, 0x8C },
    { 0x0014, 0x8C },
    { 0x0015, 0x8C },
    { 0x0016, 0x8C },
    { 0x0017, 0x8C },
    { 0x001A, 0x00 },
    { 0x001B, 0x01 },
    { 0x001C, 0x00 },
    { 0x001D, 0x10 },
    { 0x001E, 0x00 },
    { 0x001F, 0x00 },
    { 0x0020, 0x00 },
    { 0x0021, 0x00 },
    { 0x002A, 0x00 },
    { 0x002B, 0x01 },
    { 0x002C, 0x01 },
    { 0x002D, 0xC0 },
    { 0x002E, 0x00 },
    { 0x002F, 0x00 },
    { 0x0030, 0x00 },
    { 0x0031, 0x00 },
    { 0x005A, 0x00 },
    { 0x005B, 0x00 },
    { 0x0095, 0x00 },
    { 0x0096, 0x00 },
    { 0x0097, 0x00 },
    { 0x0098, 0x00 },
    { 0x0099, 0x00 },
    { 0x009A, 0x00 },
    { 0x009B, 0x00 },
    { 0x00A2, 0x00 },
    { 0x00A3, 0x00 },
    { 0x00A4, 0x00 },
    { 0x00A5, 0x00 },
    { 0x00B7, 0x92 },
};

si5351a_revb_register_t const si5351a_1000_CLK1_registers[SI5351A_REVB_REG_CONFIG_NUM_REGS] =
{
    { 0x0002, 0x53 },
    { 0x0003, 0x00 },
    { 0x0004, 0x20 },
    { 0x0007, 0x00 },
    { 0x000F, 0x00 },
    { 0x0010, 0x8C },
    { 0x0011, 0x0F },
    { 0x0012, 0x8C },
    { 0x0013, 0x8C },
    { 0x0014, 0x8C },
    { 0x0015, 0x8C },
    { 0x0016, 0x8C },
    { 0x0017, 0x8C },
    { 0x001A, 0x00 },
    { 0x001B, 0x01 },
    { 0x001C, 0x00 },
    { 0x001D, 0x10 },
    { 0x001E, 0x00 },
    { 0x001F, 0x00 },
    { 0x0020, 0x00 },
    { 0x0021, 0x00 },
    { 0x0032, 0x00 },
    { 0x0033, 0x01 },
    { 0x0034, 0x01 },
    { 0x0035, 0xC0 },
    { 0x0036, 0x00 },
    { 0x0037, 0x00 },
    { 0x0038, 0x00 },
    { 0x0039, 0x00 },
    { 0x005A, 0x00 },
    { 0x005B, 0x00 },
    { 0x0095, 0x00 },
    { 0x0096, 0x00 },
    { 0x0097, 0x00 },
    { 0x0098, 0x00 },
    { 0x0099, 0x00 },
    { 0x009A, 0x00 },
    { 0x009B, 0x00 },
    { 0x00A2, 0x00 },
    { 0x00A3, 0x00 },
    { 0x00A4, 0x00 },
    { 0x00A6, 0x00 },
    { 0x00B7, 0x92 },
};

/* ==================== Private Functions ==================== */

/**
 * @brief Send a register value to SI5351 via software I2C
 * @param regAddr: Register address (8-bit)
 * @param value: Register value (8-bit)
 * @note This function replaces the old IICsendreg implementation
 *       and uses drv_soft_i2c driver functions
 */
static void IICsendreg(uint8_t regAddr, uint8_t value)
{
    Drv_SoftI2C_WriteReg(SI5351_I2C_INSTANCE, SI5351_I2C_ADDR, regAddr, &value, 1);
}

/**
 * @brief Map error enumeration to return value
 * @param err: Error enumeration value
 * @return int8_t error code:
 *         - 0:  Success (ERROR_SI5351_OK)
 *         - -1: Channel out of range (ERROR_SI5351_CH)
 *         - -2: Frequency out of range (ERROR_SI5351_FREQ)
 */
static int8_t return_err(SI5351_ERROR_EnumDef err)
{
    int8_t return_err = 0;

    switch( (uint8_t)err )
    {
    case ERROR_SI5351_OK:
        return_err = 0;
        break;
    case ERROR_SI5351_CH:
        return_err = -1;
        break;
    case ERROR_SI5351_FREQ:
        return_err = -2;
        break;
    default:
        break;
    }

    return return_err;
}

/* ==================== Public Functions ==================== */

/**
 * @brief Reset all 43 registers of SI5351 to default values
 * @note This function clears all configuration registers to 0x00
 */
void Si5351_SetFrequency_ALL_RESET(void)
{
    uint16_t i = 0;

    for(i=0; i<SI5351A_REVB_REG_CONFIG_NUM_REGS; i++)
    {
        IICsendreg(si5351a_1000_CLK0_registers[i].address,	0x00);
    }
}

/**
 * @brief Initialize SI5351 chip and structure St_SI5351A
 * @note This function initializes the SI5351 parameters and resets all registers
 */
void Si5351_Init(void)
{
    /* Initialize structure parameters */
    St_SI5351A.para.max_ch = 2;
    St_SI5351A.para.min_ch = 0;

    St_SI5351A.para.max_freq = 1300;  /* Maximum frequency: 1300 kHz */
    St_SI5351A.para.min_freq = 450;   /* Minimum frequency: 450 kHz */

    Si5351_SetFrequency_ALL_RESET();
	
    Si5351_StopPWM();
    delay_ms(100);
}

/**
 * @brief Configure SI5351 CLK1 to output specified frequency PWM signal for TIM ETR
 * @param freq: CLK1 output frequency in kHz
 */
void Si5351_Open_CLK1( uint32_t freq )
{
    Si5351_PWM_TIM( freq );
}

/**
 * @brief Stop SI5351A PWM output
 * @note This function disables CLK0 and CLK1 outputs
 */
void Si5351_StopPWM( void )
{
    IICsendreg( SI_CLK0_CONTROL, SI_POWEROFF );
    IICsendreg( SI_CLK1_CONTROL, SI_POWEROFF );
}

/**
 * @brief Calculate Synthesis parameters
 * @param para: Pointer to fraction structure to store calculated parameters
 * @param freq: PLL frequency and target frequency values
 * @note Calculates quotient, integer quotient, numerator and denominator
 *       for the synthesis divider configuration
 */
void Si5351_GetPara(SI5351A_FRACTION_TypeDef *para, SI5351A_FREQ_TypeDef freq)
{
	double tmp_freq = 0;
	
    para->quotient = (double)freq.pll_freq / freq.freq_out;

    para->int_quotient = (uint32_t)(floor(freq.pll_freq / freq.freq_out));
	
	tmp_freq = (double)(freq.pll_freq % freq.freq_out) / freq.freq_out;
	
    para->numerator = (uint32_t)(tmp_freq*DENOM_20BIT);
	
    para->denominator = DENOM_20BIT;
}

/**
 * @brief Enable CLK0 output
 */
void Si5351_CLK0_OUT(void)
{
    IICsendreg(SI_CLK0_CONTROL, si5351a_1000_CLK0_registers[BIT_CLK0_CONTROL].value);
}

/**
 * @brief Enable CLK1 output
 */
void Si5351_CLK1_OUT(void)
{
    IICsendreg(SI_CLK1_CONTROL, si5351a_1000_CLK1_registers[BIT_CLK1_CONTROL].value);
}

/**
 * @brief Set SI5351 output frequency (Professional version)
 * @param ch: Channel selection (0:CLK0, 1:CLK1, 2:CLK2)
 * @param frequency: Target frequency in kHz
 * @return int8_t error code:
 *         - 0:  Success
 *         - -1: Channel out of range (0-2)
 *         - -2: Frequency out of valid range
 * @note This function uses a fixed PLL frequency of 900 MHz
 */
int8_t Si5351_SetFrequency_Pro(uint8_t ch, uint32_t frequency)
{
    /* Frequency Plan:
     * PLL_A:
     *    Enabled Features = None
     *    Fvco             = 900 MHz
     *    M                = 36
     *    Input0:
     *       Source           = Crystal
     *       Source Frequency = 25 MHz
     *       Fpfd             = 25 MHz
     *       Load Capacitance = Load_08pF
     *    Output0:
     *       Features       = None
     *       Disabled State = StopLow
     *       R              = 1  (2^0)
     *       Fout           = Variable
     *       N              = Variable
     */
    uint32_t pllFreq = 900000;  /* PLL frequency: 900 MHz */

    SI5351_ERROR_EnumDef error = ERROR_SI5351_OK;

    switch((uint8_t)St_SI5351A.para.St_freq.which_source)
    {
    case SOURCE_SI5351_SELF:
        break;
    case SOURCE_SI5351_TIM:
        frequency *= 4;
        break;
    default:
        break;
    }

    St_SI5351A.para.St_freq.pll_freq = pllFreq;
    St_SI5351A.para.St_freq.freq_out = frequency * 1000 / 222.4;

    /* Configure PLL related registers */
    switch(ch)
    {
    case 0:
        IICsendreg(si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 0].address,	si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 0].value);
        IICsendreg(si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 1].address,  	si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 1].value);
        IICsendreg(si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 2].address, 	si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 2].value);
        IICsendreg(si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 3].address, 	si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 3].value);
        IICsendreg(si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 4].address, 	si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 4].value);
        IICsendreg(si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 5].address, 	si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 5].value);
        IICsendreg(si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 6].address, 	si5351a_1000_CLK0_registers[BIT_SYNTH_PLL_A + 6].value);
        break;
    case 1:
        IICsendreg(si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 0].address,	si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 0].value);
        IICsendreg(si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 1].address,  	si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 1].value);
        IICsendreg(si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 2].address, 	si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 2].value);
        IICsendreg(si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 3].address, 	si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 3].value);
        IICsendreg(si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 4].address, 	si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 4].value);
        IICsendreg(si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 5].address, 	si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 5].value);
        IICsendreg(si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 6].address, 	si5351a_1000_CLK1_registers[BIT_SYNTH_PLL_A + 6].value);
        break;
    default:
        break;
    }
    /* Configure Synthesis related registers */

    Si5351_GetPara(&St_fraction, St_SI5351A.para.St_freq);

    switch(ch)
    {
    case 0:
        SetMultisynth_all(SI_SYNTH_MS_0, St_fraction.int_quotient, St_fraction.numerator, St_fraction.denominator, SI_R_DIV_1);
        break;
    case 1:
        SetMultisynth_all(SI_SYNTH_MS_1, St_fraction.int_quotient, St_fraction.numerator, St_fraction.denominator, SI_R_DIV_1);
        break;
    default:
        break;
    }

    /* Configure CLK output control register */
    switch(ch)
    {
    case 0:
        IICsendreg(SI_CLK0_CONTROL, si5351a_1000_CLK0_registers[BIT_CLK0_CONTROL].value);
        break;
    case 1:
        IICsendreg(SI_CLK1_CONTROL, si5351a_1000_CLK1_registers[BIT_CLK1_CONTROL].value);
        break;
    default:
        break;
    }

    return return_err(error);
}

/**
 * @brief Set SI5351 output frequency
 * @param ch: Channel selection (0:CLK0, 1:CLK1, 2:CLK2)
 * @param frequency: Target frequency in kHz
 * @return int8_t error code:
 *         - 0:  Success
 *         - -1: Channel out of range (0-2)
 *         - -2: Frequency out of valid range
 * @note This function calculates PLL and divider values dynamically
 */
int8_t Si5351_SetFrequency(uint8_t ch, uint32_t frequency)
{
    uint32_t pllFreq;
    uint32_t xtalFreq = XTAL_FREQ;  /* Crystal frequency: 25 MHz */
    uint32_t l;
    float f;
    uint8_t mult;
    uint32_t num;
    uint32_t denom;
    uint32_t divider;

    SI5351_ERROR_EnumDef error = ERROR_SI5351_OK;

    if((frequency < St_SI5351A.para.min_freq) || (frequency > St_SI5351A.para.max_freq))
    {
        error = ERROR_SI5351_FREQ;
        return return_err(error);
    }

    switch((uint8_t)St_SI5351A.para.St_freq.which_source)
    {
    case SOURCE_SI5351_SELF:
        break;
    case SOURCE_SI5351_TIM:
        frequency *= 2;
        break;
    default:
        break;
    }

    /* Frequency calculation formulas:
     * f(out) = f(pll) / (M(x) * R(x))
     * f(pll) = f(crystal) * (a + b/c)
     * MSN_P1[17:0] = 128*a + Floor(128*b/c) - 512
     * MSN_P2[19:0] = 128*b - c*Floor(128*b/c)
     * MSN_P3[19:0] = c
     */

    divider = FREQ_PLL / frequency;  /* PLL frequency: 900 MHz, divided by target frequency */

    if (divider % 2)
    {
        divider--;  /* Ensure an even integer divider */
    }

    pllFreq = divider * frequency;      /* Calculate pllFrequency: divider * target frequency */
    mult = pllFreq / xtalFreq;          /* Determine integer multiplier for pllFrequency */
    l = pllFreq % xtalFreq;             /* Calculate remainder */
    f = l;                               /* Convert to float for calculation */
    f *= DENOM_20BIT;                   /* Scale numerator and denominator, 20-bit range */
    f /= xtalFreq;                      /* Each 20-bit (range 0..1048575) */
    num = f;                             /* Actual divider is mult + num/denom */
    denom = DENOM_20BIT;                /* For calculation, denominator is 1048575 */

    /* Configure PLL and multiplier registers */
    SetPLLClk(SI_SYNTH_PLL_A, mult, num, denom);

    switch(ch)
    {
    case 0:
        /* Configure MultiSynth divider for channel 0. R divider can be 2^n, range 1..128 */
        /* If output frequency is less than 1 MHz, use R divider stage */
        SetMultisynth(SI_SYNTH_MS_0, divider, SI_R_DIV_1);

        /* Reset PLL - this will cause a brief glitch. For small frequency changes, */
        /* PLL reset may be needed, but no glitch occurs */
        IICsendreg(SI_PLL_RESET, SI_PLLX_RESET);

        /* Configure CLK0 output (0x4F), connect MultiSynth0 to PLL */
        IICsendreg(SI_CLK0_CONTROL, 0x4F|SI_CLK_SRC_PLL_A);
        break;

    case 1:
        SetMultisynth(SI_SYNTH_MS_1, divider, SI_R_DIV_1);

        IICsendreg(SI_PLL_RESET, SI_PLLX_RESET);
        IICsendreg(SI_CLK1_CONTROL, 0x4F|SI_CLK_SRC_PLL_A);
        break;

    default:
        error = ERROR_SI5351_CH;
        break;
    }

    return return_err(error);
}

/**
 * @brief Configure PLL clock
 * @param pll_reg: PLL register address (SI_SYNTH_PLL_A or SI_SYNTH_PLL_B)
 * @param mult: Integer multiplier parameter
 * @param num: Numerator parameter
 * @param denom: Denominator parameter
 * @note PLL frequency = (mult + num/denom) * crystal_frequency
 */
void SetPLLClk(uint8_t pll_reg, uint8_t mult, uint32_t num, uint32_t denom)
{
    uint32_t P1;  /* PLL config register P1 */
    uint32_t P2;  /* PLL config register P2 */
    uint32_t P3;  /* PLL config register P3 */

    P1 = (uint32_t)(128 * ((float)num / (float)denom));
    P1 = (uint32_t)(128 * (uint32_t)(mult) + P1 - 512);
    P2 = (uint32_t)(128 * ((float)num / (float)denom));
    P2 = (uint32_t)(128 * num - denom * P2);
    P3 = denom;

    IICsendreg(pll_reg + 0, (P3 & 0x0000FF00) >> 8);
    IICsendreg(pll_reg + 1, (P3 & 0x000000FF));
    IICsendreg(pll_reg + 2, (P1 & 0x00030000) >> 16);
    IICsendreg(pll_reg + 3, (P1 & 0x0000FF00) >> 8);
    IICsendreg(pll_reg + 4, (P1 & 0x000000FF));
    IICsendreg(pll_reg + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
    IICsendreg(pll_reg + 6, (P2 & 0x0000FF00) >> 8);
    IICsendreg(pll_reg + 7, (P2 & 0x000000FF));
}

/**
 * @brief Configure multi-synth divider
 * @param synth_reg: Multi-synth register address
 *                   - SI_SYNTH_MS_0: CLK0
 *                   - SI_SYNTH_MS_1: CLK1
 *                   - SI_SYNTH_MS_2: CLK2
 * @param divider: Divider parameter (integer value)
 * @param rDiv: R-divider value (SI_R_DIV_1, SI_R_DIV_2, etc.)
 */
void SetMultisynth(uint8_t synth_reg, uint32_t divider, uint8_t rDiv)
{
    uint32_t P1;  /* Synth config register P1 */
    uint32_t P2;  /* Synth config register P2 */
    uint32_t P3;  /* Synth config register P3 */

    P1 = 128 * divider - 512;
    P2 = 0;  /* P2 = 0, P3 = 1 forces an integer value for the divider */
    P3 = 1;

    IICsendreg(synth_reg + 0,   (P3 & 0x0000FF00) >> 8);
    IICsendreg(synth_reg + 1,   (P3 & 0x000000FF));
    IICsendreg(synth_reg + 2,   ((P1 & 0x00030000) >> 16) | rDiv);
    IICsendreg(synth_reg + 3,   (P1 & 0x0000FF00) >> 8);
    IICsendreg(synth_reg + 4,   (P1 & 0x000000FF));
    IICsendreg(synth_reg + 5,   ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
    IICsendreg(synth_reg + 6,   (P2 & 0x0000FF00) >> 8);
    IICsendreg(synth_reg + 7,   (P2 & 0x000000FF));
}

/**
 * @brief Configure multi-synth divider with fractional parameters
 * @param synth_reg: Multi-synth register address
 *                   - SI_SYNTH_MS_0: CLK0
 *                   - SI_SYNTH_MS_1: CLK1
 *                   - SI_SYNTH_MS_2: CLK2
 * @param a: Integer part of divider (parameter1)
 * @param b: Numerator of fractional part (parameter2)
 * @param c: Denominator of fractional part (parameter3)
 * @param rDiv: R-divider value
 * @note Divider = a + b/c
 */
void SetMultisynth_all(uint8_t synth_reg, uint32_t a, uint32_t b, uint32_t c, uint8_t rDiv)
{
    uint32_t P1;  /* Synth config register P1 */
    uint32_t P2;  /* Synth config register P2 */
    uint32_t P3;  /* Synth config register P3 */
    uint32_t P_SYNTH[3] = {0};

    memset(St_SI5351A.data.reg_synth, 0, sizeof(St_SI5351A.data.reg_synth));

    P1 = (uint32_t)(128 * a + floor((double)128*b/c) - 512);
    P2 = (uint32_t)(128 * b - c * floor((double)128*b/c));
    P3 = c;

    P_SYNTH[0] = (uint32_t)(128 * (uint32_t)(St_fraction.int_quotient) + (128 * ((float)St_fraction.numerator / (float)St_fraction.denominator)) - 512);
    P_SYNTH[1] = (uint32_t)(128 * St_fraction.numerator - St_fraction.denominator * (128 * ((float)St_fraction.numerator / (float)St_fraction.denominator)));
    P_SYNTH[2] = St_fraction.denominator;

    St_SI5351A.data.reg_synth[0] = ((P_SYNTH[2] >> 8) & 0xFF);
    St_SI5351A.data.reg_synth[1] = (P_SYNTH[2] & 0xFF);
    St_SI5351A.data.reg_synth[2] = (((P_SYNTH[0] >> 16) & 0x03) | rDiv);
    St_SI5351A.data.reg_synth[3] = ((P_SYNTH[0] >> 8) & 0xFF);
    St_SI5351A.data.reg_synth[4] = (P_SYNTH[0] & 0xFF);
    St_SI5351A.data.reg_synth[5] = (((P_SYNTH[2] >> 12) & 0xF0) | ((P_SYNTH[1] >> 16) & 0x0F));
    St_SI5351A.data.reg_synth[6] = ((P_SYNTH[1] >> 8) & 0xFF);
    St_SI5351A.data.reg_synth[7] = (P_SYNTH[1] & 0xFF);

    IICsendreg(synth_reg + 0,   (P3 >> 8) & 0xFF);   			/* MS_P3[15:8] */
    IICsendreg(synth_reg + 1,   (P3 & 0xFF));		   			/* MS_P3[7:0] */
    IICsendreg(synth_reg + 2,   ((P1 >> 16) & 0x03) | rDiv);	/* MS_P1[17:16] */
    IICsendreg(synth_reg + 3,   (P1 >> 8) & 0xFF);   			/* MS_P1[15:8] */
    IICsendreg(synth_reg + 4,   (P1 & 0xFF));		   			/* MS_P1[7:0] */
    IICsendreg(synth_reg + 5,   ((P3 >> 12) & 0xF0) | ((P2 >> 16) & 0x0F)); 	/* MS_P3[19:16] + MS_P2[19:16] */
    IICsendreg(synth_reg + 6,   (P2 >> 8) & 0xFF);		/* MS_P2[15:8] */
    IICsendreg(synth_reg + 7,   (P2 & 0xFF));			/* MS_P2[7:0] */
}

/**
 * @brief Stop TIM1 PWM output
 */
void TIM1_PWM_STOP(void)
{
	TIM_CtrlPWMOutputs(TIM1, DISABLE);
    TIM_Cmd(TIM1, DISABLE);
}

/**
 * @brief Modify SI5351 frequency, use MCU timer frequency as source
 * @param freq: Target frequency in kHz
 * @note This function configures SI5351 to use TIM as external clock source
 */
void Si5351_PWM_TIM( uint32_t freq )
{
    St_SI5351A.para.St_freq.which_source = SOURCE_SI5351_TIM;
    St_SI5351A.para.St_freq.freq_out = freq;

    if((St_SI5351A.para.St_freq.freq_out < St_SI5351A.para.min_freq) || (St_SI5351A.para.St_freq.freq_out > St_SI5351A.para.max_freq))
    {
        return;
    }

    switch((uint8_t)St_SI5351A.para.St_freq.which_source)
    {
    case SOURCE_SI5351_SELF:
        /* Use SI5351 CLK0 as PWM source */
        break;
    case SOURCE_SI5351_TIM:
        Si5351_SetFrequency_Pro(1, St_SI5351A.para.St_freq.freq_out);  /* TIM1 uses SI5351 CLK1 as ETR */
        break;
    default:
        break;
    }
}

/**
 * @brief Generate PWM signal with specified frequency
 * @param freq: CLK1 output frequency in kHz
 * @note Frequency range: 700-1300 kHz, default: 840 kHz
 */
void PWM_Generate( uint32_t freq )
{
    if(freq < 700 || freq > 1300)
    {
        freq = DEFAULT_FREQUENCY;
    }
    Si5351_Open_CLK1( freq );
}

/**
 * @brief Initialize PWM generation related structures
 * @note This function initializes SI5351 and waits for stabilization
 */
void PWM_Generate_Config(void)
{
    Si5351_Init();
    delay_ms(100);  /* Wait for SI5351 to stabilize */
}

/**************************End of file********************************/
