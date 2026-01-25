/************************************************************************************
 * @file     : drv_soft_i2c.h
 * @brief    : Software I2C driver - Generic implementation supporting multiple instances
 * @details  : This driver provides a generic software I2C implementation that can be
 *             configured for multiple I2C instances with different GPIO pins.
 * @author   : Refactored from bsp_I2C
 * @date     : 2025-01-25
 * @version  : V1.0.0
 * @copyright: Copyright (c) 2025
 ***********************************************************************************/
#ifndef DRV_SOFT_I2C_H
#define DRV_SOFT_I2C_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Type Definitions ==================== */

/**
 * @brief I2C instance identifier
 */
typedef enum {
    DRV_SOFT_I2C_INSTANCE_1 = 0,
    DRV_SOFT_I2C_INSTANCE_2,
    DRV_SOFT_I2C_INSTANCE_MAX
} Drv_SoftI2C_Instance_t;

/**
 * @brief I2C GPIO configuration structure
 */
typedef struct {
    GPIO_TypeDef *SCL_Port;      /* SCL GPIO port */
    uint16_t SCL_Pin;            /* SCL GPIO pin */
    GPIO_TypeDef *SDA_Port;      /* SDA GPIO port */
    uint16_t SDA_Pin;            /* SDA GPIO pin */
    uint32_t SCL_RCC_Periph;     /* RCC peripheral for SCL port */
    uint32_t SDA_RCC_Periph;     /* RCC peripheral for SDA port */
} Drv_SoftI2C_Config_t;

/* ==================== API Functions ==================== */

/**
 * @brief Initialize software I2C instance
 * @param instance: I2C instance identifier
 * @param config: Pointer to GPIO configuration structure
 * @return true if initialization successful, false otherwise
 */
bool Drv_SoftI2C_Init(Drv_SoftI2C_Instance_t instance, const Drv_SoftI2C_Config_t *config);

/**
 * @brief Transmit data via software I2C
 * @param instance: I2C instance identifier
 * @param devAddr: Device address (7-bit, left-aligned, e.g., 0x50 for 0xA0>>1)
 * @param pData: Pointer to data buffer
 * @param length: Number of bytes to transmit
 * @return true if transmission successful, false otherwise
 */
bool Drv_SoftI2C_Transmit(Drv_SoftI2C_Instance_t instance, uint8_t devAddr, const uint8_t *pData, uint16_t length);

/**
 * @brief Receive data via software I2C
 * @param instance: I2C instance identifier
 * @param devAddr: Device address (7-bit, left-aligned, e.g., 0x50 for 0xA0>>1)
 * @param pData: Pointer to data buffer
 * @param length: Number of bytes to receive
 * @return true if reception successful, false otherwise
 */
bool Drv_SoftI2C_Receive(Drv_SoftI2C_Instance_t instance, uint8_t devAddr, uint8_t *pData, uint16_t length);

/**
 * @brief Write data to a register via software I2C
 * @param instance: I2C instance identifier
 * @param devAddr: Device address (7-bit, left-aligned)
 * @param regAddr: Register address
 * @param pData: Pointer to data buffer
 * @param length: Number of bytes to write
 * @return true if write successful, false otherwise
 */
bool Drv_SoftI2C_WriteReg(Drv_SoftI2C_Instance_t instance, uint8_t devAddr, uint8_t regAddr, const uint8_t *pData, uint16_t length);

/**
 * @brief Read data from a register via software I2C
 * @param instance: I2C instance identifier
 * @param devAddr: Device address (7-bit, left-aligned)
 * @param regAddr: Register address
 * @param pData: Pointer to data buffer
 * @param length: Number of bytes to read
 * @return true if read successful, false otherwise
 */
bool Drv_SoftI2C_ReadReg(Drv_SoftI2C_Instance_t instance, uint8_t devAddr, uint8_t regAddr, uint8_t *pData, uint16_t length);

/**
 * @brief Check if I2C bus is busy
 * @param instance: I2C instance identifier
 * @return true if bus is busy, false otherwise
 */
bool Drv_SoftI2C_IsBusy(Drv_SoftI2C_Instance_t instance);

#ifdef __cplusplus
}
#endif

#endif /* DRV_SOFT_I2C_H */
/**************************End of file********************************/
