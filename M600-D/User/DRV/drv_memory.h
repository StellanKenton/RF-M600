/************************************************************************************
* @file     : drv_memory.h
* @brief    : Memory driver header file
* @details  : Read and write functions for storage space
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRV_MEMORY_H
#define DRV_MEMORY_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif

/**
 * @brief Initialize memory driver
 * @retval true if success, false if failed
 */
bool Drv_Memory_Init(void);

/**
 * @brief Read data from storage space
 * @param address Start address to read from (存储起始地址)
 * @param data Pointer to buffer to store read data (读取数据缓冲区指针)
 * @param length Number of bytes to read (读取字节数)
 * @retval true if success, false if failed
 */
bool Drv_Memory_Read(uint16_t address, uint8_t *data, uint16_t length);

/**
 * @brief Write data to storage space
 * @param address Start address to write to (存储起始地址)
 * @param data Pointer to data buffer to write (写入数据缓冲区指针)
 * @param length Number of bytes to write (写入字节数)
 * @retval true if success, false if failed
 */
bool Drv_Memory_Write(uint16_t address, const uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif
#endif  // DRV_MEMORY_H
/**************************End of file********************************/
