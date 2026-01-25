/************************************************************************************
 * @file     : drv_24c02.h
 * @brief    : AT24C02 EEPROM driver - DRV API, supports both software I2C and hardware I2C
 * @details  : Use macro DRV_24C02_USE_HW_I2C to select hardware I2C, otherwise use software I2C
 * @author   : \.rumi
 * @date     : 2025-01-25
 * @version  : V1.0.0
 * @copyright: Copyright (c) 2050
 ***********************************************************************************/
#ifndef DRV_24C02_H
#define DRV_24C02_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */
/* Uncomment the following line to use hardware I2C, otherwise software I2C will be used */
/* #define DRV_24C02_USE_HW_I2C */

/* AT24C02 device address 
 * Note: For software I2C, use 8-bit address (0xA0)
 *       For hardware I2C, use 7-bit address (0x50 = 0xA0 >> 1)
 */
#define DRV_24C02_DEV_ADDR_8BIT    0xA0  /* 8-bit address for software I2C */
#define DRV_24C02_DEV_ADDR_7BIT    0x50  /* 7-bit address for hardware I2C */

/* I2C port selection for hardware I2C (I2C1 or I2C2) */
#ifndef DRV_24C02_HW_I2C_PORT
#define DRV_24C02_HW_I2C_PORT 2  /* 1 for I2C1, 2 for I2C2 */
#endif

/* ==================== API Functions ==================== */

/**
 * @brief Initialize 24C02 driver
 * @note For software I2C, this function initializes GPIO pins
 *       For hardware I2C, make sure BSP_I2C1_Init() or BSP_I2C2_Init() is called first
 */
void Drv_24C02_Init(void);

/**
 * @brief Read data from 24C02
 * @param pBuffer: Pointer to buffer to store read data
 * @param length: Number of bytes to read
 * @param ReadAddress: Starting address to read from (0-255 for AT24C02)
 * @return true if read successful, false otherwise
 */
bool Drv_24C02_Read(uint8_t *pBuffer, uint16_t length, uint16_t ReadAddress);

/**
 * @brief Write data to 24C02
 * @param SendByte: Byte to write
 * @param WriteAddress: Address to write to (0-255 for AT24C02)
 * @return true if write successful, false otherwise
 */
bool Drv_24C02_WriteByte(uint8_t SendByte, uint16_t WriteAddress);

/**
 * @brief Write multiple bytes to 24C02
 * @param pBuffer: Pointer to buffer containing data to write
 * @param length: Number of bytes to write
 * @param WriteAddress: Starting address to write to (0-255 for AT24C02)
 * @return true if write successful, false otherwise
 */
bool Drv_24C02_Write(uint8_t *pBuffer, uint16_t length, uint16_t WriteAddress);

#ifdef __cplusplus
}
#endif

#endif /* DRV_24C02_H */
/**************************End of file********************************/
