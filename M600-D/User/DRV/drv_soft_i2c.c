/************************************************************************************
 * @file     : drv_soft_i2c.c
 * @brief    : Software I2C driver - Generic implementation supporting multiple instances
 * @details  : This driver provides a generic software I2C implementation that can be
 *             configured for multiple I2C instances with different GPIO pins.
 * @author   : Refactored from bsp_I2C
 * @date     : 2025-01-25
 * @version  : V1.0.0
 * @copyright: Copyright (c) 2025
 ***********************************************************************************/
#include "drv_soft_i2c.h"
#include "delay.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include <stdbool.h>

/* ==================== Private Definitions ==================== */

#define I2C_DELAY_US    100     /* I2C timing delay in microseconds */

/* Maximum number of I2C instances */
#define MAX_I2C_INSTANCES   DRV_SOFT_I2C_INSTANCE_MAX

/* ==================== Private Variables ==================== */

static Drv_SoftI2C_Config_t s_i2c_configs[MAX_I2C_INSTANCES];
static bool s_i2c_initialized[MAX_I2C_INSTANCES] = {false, false};

/* ==================== Private Macros ==================== */

/* GPIO control macros - these will be set per instance */
#define SCL_H(inst)     GPIO_SetBits(s_i2c_configs[inst].SCL_Port, s_i2c_configs[inst].SCL_Pin)
#define SCL_L(inst)     GPIO_ResetBits(s_i2c_configs[inst].SCL_Port, s_i2c_configs[inst].SCL_Pin)
#define SDA_H(inst)     GPIO_SetBits(s_i2c_configs[inst].SDA_Port, s_i2c_configs[inst].SDA_Pin)
#define SDA_L(inst)     GPIO_ResetBits(s_i2c_configs[inst].SDA_Port, s_i2c_configs[inst].SDA_Pin)
#define SCL_READ(inst)  GPIO_ReadInputDataBit(s_i2c_configs[inst].SCL_Port, s_i2c_configs[inst].SCL_Pin)
#define SDA_READ(inst)  GPIO_ReadInputDataBit(s_i2c_configs[inst].SDA_Port, s_i2c_configs[inst].SDA_Pin)

/* ==================== Private Functions ==================== */

/**
 * @brief I2C delay function
 */
static void I2C_Delay(void)
{
    delay_us(I2C_DELAY_US);
}

/**
 * @brief Generate I2C start condition
 * @param instance: I2C instance identifier
 * @return true if start successful, false if bus is busy
 */
static bool I2C_Start(Drv_SoftI2C_Instance_t instance)
{
    if (instance >= MAX_I2C_INSTANCES || !s_i2c_initialized[instance])
    {
        return false;
    }

    SDA_H(instance);
    SCL_H(instance);
    I2C_Delay();
    
    /* Check if bus is busy */
    if (!SDA_READ(instance))
    {
        return false;  /* SDA line is low, bus is busy */
    }
    
    SDA_L(instance);
    I2C_Delay();
    
    if (SDA_READ(instance))
    {
        return false;  /* SDA line is high, bus error */
    }
    
    SCL_L(instance);
    I2C_Delay();
    return true;
}

/**
 * @brief Generate I2C stop condition
 * @param instance: I2C instance identifier
 */
static void I2C_Stop(Drv_SoftI2C_Instance_t instance)
{
    if (instance >= MAX_I2C_INSTANCES || !s_i2c_initialized[instance])
    {
        return;
    }

    SCL_L(instance);
    I2C_Delay();
    SDA_L(instance);
    I2C_Delay();
    SCL_H(instance);
    I2C_Delay();
    SDA_H(instance);
    I2C_Delay();
}

/**
 * @brief Wait for I2C ACK
 * @param instance: I2C instance identifier
 * @return true if ACK received, false if NACK
 */
static bool I2C_WaitAck(Drv_SoftI2C_Instance_t instance)
{
    uint16_t timeout = 1000;
    
    if (instance >= MAX_I2C_INSTANCES || !s_i2c_initialized[instance])
    {
        return false;
    }

    SCL_L(instance);
    SDA_H(instance);  /* Release SDA for slave to pull low */
    I2C_Delay();
    SCL_H(instance);
    I2C_Delay();
    
    /* Wait for ACK (SDA should be pulled low by slave) */
    while (SDA_READ(instance) && timeout > 0)
    {
        timeout--;
        if (timeout == 0)
        {
            SCL_L(instance);
            return false;  /* Timeout, NACK received */
        }
    }
    
    SCL_L(instance);
    I2C_Delay();
    return true;  /* ACK received */
}

/**
 * @brief Send ACK signal
 * @param instance: I2C instance identifier
 */
static void I2C_Ack(Drv_SoftI2C_Instance_t instance)
{
    if (instance >= MAX_I2C_INSTANCES || !s_i2c_initialized[instance])
    {
        return;
    }

    SCL_L(instance);
    SDA_L(instance);
    I2C_Delay();
    SCL_H(instance);
    I2C_Delay();
    SCL_L(instance);
    I2C_Delay();
}

/**
 * @brief Send NACK signal
 * @param instance: I2C instance identifier
 */
static void I2C_NoAck(Drv_SoftI2C_Instance_t instance)
{
    if (instance >= MAX_I2C_INSTANCES || !s_i2c_initialized[instance])
    {
        return;
    }

    SCL_L(instance);
    SDA_H(instance);
    I2C_Delay();
    SCL_H(instance);
    I2C_Delay();
    SCL_L(instance);
    I2C_Delay();
}

/**
 * @brief Send one byte via I2C
 * @param instance: I2C instance identifier
 * @param data: Byte to send
 * @return true if successful, false otherwise
 */
static bool I2C_SendByte(Drv_SoftI2C_Instance_t instance, uint8_t data)
{
    uint8_t i;
    
    if (instance >= MAX_I2C_INSTANCES || !s_i2c_initialized[instance])
    {
        return false;
    }

    for (i = 0; i < 8; i++)  /* Send 8 bits */
    {
        SCL_L(instance);  /* Pull clock low, prepare to set SDA value */
        
        if ((data & 0x80) == 0)
        {
            SDA_L(instance);
        }
        else
        {
            SDA_H(instance);
        }
        
        data <<= 1;
        I2C_Delay();
        SCL_H(instance);
        I2C_Delay();  /* Wait for slave to read data */
    }
    
    SCL_L(instance);
    SDA_H(instance);  /* Release SDA for ACK */
    I2C_Delay();
    SCL_H(instance);
    I2C_Delay();
    
    /* Wait for slave ACK */
    if (!I2C_WaitAck(instance))
    {
        return false;  /* NACK received */
    }
    
    SCL_L(instance);  /* Release clock line for next operation */
    return true;
}

/**
 * @brief Receive one byte via I2C
 * @param instance: I2C instance identifier
 * @return Received byte
 */
static uint8_t I2C_ReceiveByte(Drv_SoftI2C_Instance_t instance)
{
    uint8_t i;
    uint8_t data = 0;
    
    if (instance >= MAX_I2C_INSTANCES || !s_i2c_initialized[instance])
    {
        return 0;
    }

    SCL_L(instance);
    SDA_H(instance);  /* Release SDA */
    I2C_Delay();
    
    for (i = 0; i < 8; i++)  /* Receive 8 bits */
    {
        SCL_H(instance);
        I2C_Delay();
        data <<= 1;
        if (SDA_READ(instance))
        {
            data |= 0x01;
        }
        SCL_L(instance);
        I2C_Delay();
    }
    
    return data;
}

/* ==================== Public Functions ==================== */

/**
 * @brief Initialize software I2C instance
 */
bool Drv_SoftI2C_Init(Drv_SoftI2C_Instance_t instance, const Drv_SoftI2C_Config_t *config)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    if (instance >= MAX_I2C_INSTANCES || config == NULL)
    {
        return false;
    }
    
    /* Save configuration */
    s_i2c_configs[instance] = *config;
    
    /* Enable GPIO clocks */
    if (config->SCL_RCC_Periph != 0)
    {
        RCC_APB2PeriphClockCmd(config->SCL_RCC_Periph, ENABLE);
    }
    if (config->SDA_RCC_Periph != 0 && config->SDA_RCC_Periph != config->SCL_RCC_Periph)
    {
        RCC_APB2PeriphClockCmd(config->SDA_RCC_Periph, ENABLE);
    }
    
    /* Configure SCL pin as open-drain output */
    GPIO_InitStructure.GPIO_Pin = config->SCL_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(config->SCL_Port, &GPIO_InitStructure);
    
    /* Configure SDA pin as open-drain output */
    GPIO_InitStructure.GPIO_Pin = config->SDA_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(config->SDA_Port, &GPIO_InitStructure);
    
    /* Set SCL and SDA to high (idle state) */
    SCL_H(instance);
    SDA_H(instance);
    
    s_i2c_initialized[instance] = true;
    return true;
}

/**
 * @brief Transmit data via software I2C
 */
bool Drv_SoftI2C_Transmit(Drv_SoftI2C_Instance_t instance, uint8_t devAddr, const uint8_t *pData, uint16_t length)
{
    uint16_t i;
    
    if (instance >= MAX_I2C_INSTANCES || !s_i2c_initialized[instance] || pData == NULL || length == 0)
    {
        return false;
    }
    
    /* Generate start condition */
    if (!I2C_Start(instance))
    {
        return false;
    }
    
    /* Send device address with write bit (7-bit address, left-aligned) */
    if (!I2C_SendByte(instance, (devAddr << 1) & 0xFE))
    {
        I2C_Stop(instance);
        return false;
    }
    
    /* Send data */
    for (i = 0; i < length; i++)
    {
        if (!I2C_SendByte(instance, pData[i]))
        {
            I2C_Stop(instance);
            return false;
        }
    }
    
    /* Generate stop condition */
    I2C_Stop(instance);
    return true;
}

/**
 * @brief Receive data via software I2C
 */
bool Drv_SoftI2C_Receive(Drv_SoftI2C_Instance_t instance, uint8_t devAddr, uint8_t *pData, uint16_t length)
{
    uint16_t i;
    
    if (instance >= MAX_I2C_INSTANCES || !s_i2c_initialized[instance] || pData == NULL || length == 0)
    {
        return false;
    }
    
    /* Generate start condition */
    if (!I2C_Start(instance))
    {
        return false;
    }
    
    /* Send device address with read bit (7-bit address, left-aligned) */
    if (!I2C_SendByte(instance, (devAddr << 1) | 0x01))
    {
        I2C_Stop(instance);
        return false;
    }
    
    /* Receive data */
    for (i = 0; i < length; i++)
    {
        pData[i] = I2C_ReceiveByte(instance);
        if (i < length - 1)
        {
            I2C_Ack(instance);  /* Send ACK for all bytes except the last one */
        }
        else
        {
            I2C_NoAck(instance);  /* Send NACK for the last byte */
        }
    }
    
    /* Generate stop condition */
    I2C_Stop(instance);
    return true;
}

/**
 * @brief Write data to a register via software I2C
 */
bool Drv_SoftI2C_WriteReg(Drv_SoftI2C_Instance_t instance, uint8_t devAddr, uint8_t regAddr, const uint8_t *pData, uint16_t length)
{
    if (instance >= MAX_I2C_INSTANCES || !s_i2c_initialized[instance] || pData == NULL || length == 0)
    {
        return false;
    }
    
    /* Generate start condition */
    if (!I2C_Start(instance))
    {
        return false;
    }
    
    /* Send device address with write bit */
    if (!I2C_SendByte(instance, (devAddr << 1) & 0xFE))
    {
        I2C_Stop(instance);
        return false;
    }
    
    /* Send register address */
    if (!I2C_SendByte(instance, regAddr))
    {
        I2C_Stop(instance);
        return false;
    }
    
    /* Send data */
    for (uint16_t i = 0; i < length; i++)
    {
        if (!I2C_SendByte(instance, pData[i]))
        {
            I2C_Stop(instance);
            return false;
        }
    }
    
    /* Generate stop condition */
    I2C_Stop(instance);
    return true;
}

/**
 * @brief Read data from a register via software I2C
 */
bool Drv_SoftI2C_ReadReg(Drv_SoftI2C_Instance_t instance, uint8_t devAddr, uint8_t regAddr, uint8_t *pData, uint16_t length)
{
    if (instance >= MAX_I2C_INSTANCES || !s_i2c_initialized[instance] || pData == NULL || length == 0)
    {
        return false;
    }
    
    /* Generate start condition */
    if (!I2C_Start(instance))
    {
        return false;
    }
    
    /* Send device address with write bit (to write register address) */
    if (!I2C_SendByte(instance, (devAddr << 1) & 0xFE))
    {
        I2C_Stop(instance);
        return false;
    }
    
    /* Send register address */
    if (!I2C_SendByte(instance, regAddr))
    {
        I2C_Stop(instance);
        return false;
    }
    
    /* Generate repeated start condition */
    if (!I2C_Start(instance))
    {
        I2C_Stop(instance);
        return false;
    }
    
    /* Send device address with read bit */
    if (!I2C_SendByte(instance, (devAddr << 1) | 0x01))
    {
        I2C_Stop(instance);
        return false;
    }
    
    /* Receive data */
    for (uint16_t i = 0; i < length; i++)
    {
        pData[i] = I2C_ReceiveByte(instance);
        if (i < length - 1)
        {
            I2C_Ack(instance);  /* Send ACK for all bytes except the last one */
        }
        else
        {
            I2C_NoAck(instance);  /* Send NACK for the last byte */
        }
    }
    
    /* Generate stop condition */
    I2C_Stop(instance);
    return true;
}

/**
 * @brief Check if I2C bus is busy
 */
bool Drv_SoftI2C_IsBusy(Drv_SoftI2C_Instance_t instance)
{
    if (instance >= MAX_I2C_INSTANCES || !s_i2c_initialized[instance])
    {
        return true;  /* Consider uninitialized as busy */
    }
    
    /* Check if SDA line is low (bus is busy) */
    return !SDA_READ(instance);
}

/**************************End of file********************************/
