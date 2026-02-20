/***********************************************************************************
* @file     : drv_memory.c
* @brief    : Memory driver implementation
* @details  : Implementation of read and write functions for storage space
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drv_memory.h"
#include "drv_24c02.h"
#include <string.h>
#include "log.h"

#define MEMORY_BOOT_CHECK_LEN      16U
#define MEMORY_SN_PREFIX           "M600-SN"
#define MEMORY_SN_PREFIX_LEN       7U
#define MEMORY_SN_DEFAULT_STR      "M600-SN000000007"

/* Memory configuration */
#define MEMORY_SIZE             0x1000      ///< Total memory size (4KB)
#define MEMORY_PAGE_SIZE        0x0100      ///< Memory page size (256 bytes)

/* Memory status */
static bool s_MemoryInitialized = false;

/**
 * @brief Initialize memory driver
 * @retval true if success, false if failed
 */
bool Drv_Memory_Init(void)
{
    uint8_t boot_data[MEMORY_BOOT_CHECK_LEN] = {0};
    static uint8_t default_sn[] = MEMORY_SN_DEFAULT_STR;
    bool need_default_sn = false;  /* 需要写入默认序列号 */
    uint16_t i;

    if (!Drv_24C02_Read(boot_data, MEMORY_BOOT_CHECK_LEN, 0))
    {
        return false;
    }

    /* 前面字节不是"M600-SN"，且剩下的字节存在0xFF -> 判定为错误序列号，需写入默认序列号 */
    if (memcmp(boot_data, MEMORY_SN_PREFIX, MEMORY_SN_PREFIX_LEN) != 0)
    {
        for (i = MEMORY_SN_PREFIX_LEN; i < MEMORY_BOOT_CHECK_LEN; i++)
        {
            if (boot_data[i] == 0xFF)
            {
                need_default_sn = true;  /* 剩余字节存在0xFF，判定为错误 */
                break;
            }
        }
    } else {
        // output boot_data to Log
        LOG_I("boot_data: %s", boot_data);
    }

    if (need_default_sn)
    {
        if (!Drv_24C02_Write(default_sn, (uint16_t)(sizeof(default_sn) - 1U), 0))
        {
            return false;
        }
    }

    s_MemoryInitialized = true;
    return true;
}

/**
 * @brief Read data from storage space
 * @param address Start address to read from (存储起始地址)
 * @param data Pointer to buffer to store read data (读取数据缓冲区指针)
 * @param length Number of bytes to read (读取字节数)
 * @retval true if success, false if failed
 */
bool Drv_Memory_Read(uint16_t address, uint8_t *data, uint16_t length)
{
    // Check if memory is initialized
    if (!s_MemoryInitialized)
    {
        return false;
    }

    // Check parameters
    if (data == NULL || length == 0)
    {
        return false;
    }

    // Check address range
    if (address >= MEMORY_SIZE || (address + length) > MEMORY_SIZE)
    {
        return false;
    }

    // TODO: Implement actual memory read operation
    // For EEPROM (I2C):
    // - Send start condition
    // - Send device address + write bit
    // - Send memory address (high byte, low byte)
    // - Send repeated start condition
    // - Send device address + read bit
    // - Read data bytes
    // - Send stop condition
    //
    // For Flash:
    // - Unlock Flash if needed
    // - Read from Flash memory address
    // - Lock Flash if needed

    // Placeholder: Copy from a simulated memory buffer
    // In actual implementation, replace this with real hardware access
    for (uint16_t i = 0; i < length; i++)
    {
        data[i] = 0xFF;  // Default value, replace with actual read
    }

    return true;
}

/**
 * @brief Write data to storage space
 * @param address Start address to write to (存储起始地址)
 * @param data Pointer to data buffer to write (写入数据缓冲区指针)
 * @param length Number of bytes to write (写入字节数)
 * @retval true if success, false if failed
 */
bool Drv_Memory_Write(uint16_t address, const uint8_t *data, uint16_t length)
{
    // Check if memory is initialized
    if (!s_MemoryInitialized)
    {
        return false;
    }

    // Check parameters
    if (data == NULL || length == 0)
    {
        return false;
    }

    // Check address range
    if (address >= MEMORY_SIZE || (address + length) > MEMORY_SIZE)
    {
        return false;
    }

    // TODO: Implement actual memory write operation
    // For EEPROM (I2C):
    // - Send start condition
    // - Send device address + write bit
    // - Send memory address (high byte, low byte)
    // - Send data bytes (may need to split into pages)
    // - Wait for write completion (polling or delay)
    // - Send stop condition
    //
    // For Flash:
    // - Unlock Flash if needed
    // - Erase page/sector if needed
    // - Program Flash memory address
    // - Verify write operation
    // - Lock Flash if needed

    // Placeholder: Write to a simulated memory buffer
    // In actual implementation, replace this with real hardware access
    // Note: EEPROM writes may need page boundaries consideration
    // Note: Flash writes may need erase before write

    return true;
}

/**************************End of file********************************/
