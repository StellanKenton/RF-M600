/***********************************************************************************
* @file     : app_memory.c
* @brief    : Treatment parameters memory management module implementation
* @details  : Implementation of parameter save/load functions for four treatment modes
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "app_memory.h"
#include "drv_memory.h"
#include <stddef.h>

/* CRC16 polynomial: CRC-16-IBM (0x8005) */
#define CRC16_POLYNOMIAL 0x8005
#define CRC16_INIT_VALUE 0xFFFF

/* Memory address definitions for each treatment parameter structure */
#define MEM_ADDR_RF_PARAMS      0x0000      ///< Radio Frequency parameters address
#define MEM_ADDR_SW_PARAMS      0x0010      ///< Shock Wave parameters address
#define MEM_ADDR_NPH_PARAMS     0x0020      ///< Negative Pressure Heat parameters address
#define MEM_ADDR_US_PARAMS      0x0030      ///< Ultrasound parameters address

/**
 * @brief Calculate CRC16 checksum
 * @param data Pointer to data buffer
 * @param length Data length in bytes
 * @retval CRC16 checksum value
 */
static uint16_t Calculate_CRC16(const uint8_t *data, size_t length)
{
    uint16_t crc = CRC16_INIT_VALUE;
    size_t i, j;
    
    for (i = 0; i < length; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ CRC16_POLYNOMIAL;
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

/**
 * @brief Initialize memory module
 */
void App_Memory_Init(void)
{
    // Initialize memory driver
    Drv_Memory_Init();
}

/**
 * @brief Save Radio Frequency treatment parameters
 * @param params Pointer to RF treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_SaveRFParams(const RF_TreatParams_t *params)
{
    RF_TreatParams_t tempParams;
    uint16_t crc;
    
    if (params == NULL)
    {
        return false;
    }
    
    /* Copy parameters to temporary structure */
    tempParams = *params;
    
    /* Calculate CRC16 for all fields except CRC field itself */
    tempParams.CrcCode = 0;
    crc = Calculate_CRC16((const uint8_t *)&tempParams, sizeof(RF_TreatParams_t) - sizeof(uint16_t));
    tempParams.CrcCode = crc;
    
    return Drv_Memory_Write(MEM_ADDR_RF_PARAMS, (const uint8_t *)&tempParams, sizeof(RF_TreatParams_t));
}

/**
 * @brief Load Radio Frequency treatment parameters
 * @param params Pointer to store loaded RF treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_LoadRFParams(RF_TreatParams_t *params)
{
    RF_TreatParams_t tempParams;
    uint16_t storedCrc;
    uint16_t calculatedCrc;
    
    if (params == NULL)
    {
        return false;
    }
    
    /* Read parameters from memory */
    if (!Drv_Memory_Read(MEM_ADDR_RF_PARAMS, (uint8_t *)&tempParams, sizeof(RF_TreatParams_t)))
    {
        return false;
    }
    
    /* Save stored CRC value */
    storedCrc = tempParams.CrcCode;
    
    /* Calculate CRC16 for all fields except CRC field itself */
    tempParams.CrcCode = 0;
    calculatedCrc = Calculate_CRC16((const uint8_t *)&tempParams, sizeof(RF_TreatParams_t) - sizeof(uint16_t));
    
    /* Verify CRC16 */
    if (calculatedCrc != storedCrc)
    {
        return false;
    }
    
    /* Restore CRC value and copy to output */
    tempParams.CrcCode = storedCrc;
    *params = tempParams;
    
    return true;
}

/**
 * @brief Save Shock Wave treatment parameters
 * @param params Pointer to SW treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_SaveSWParams(const SW_TreatParams_t *params)
{
    SW_TreatParams_t tempParams;
    uint16_t crc;
    
    if (params == NULL)
    {
        return false;
    }
    
    /* Copy parameters to temporary structure */
    tempParams = *params;
    
    /* Calculate CRC16 for all fields except CRC field itself */
    tempParams.CrcCode = 0;
    crc = Calculate_CRC16((const uint8_t *)&tempParams, sizeof(SW_TreatParams_t) - sizeof(uint16_t));
    tempParams.CrcCode = crc;
    
    return Drv_Memory_Write(MEM_ADDR_SW_PARAMS, (const uint8_t *)&tempParams, sizeof(SW_TreatParams_t));
}

/**
 * @brief Load Shock Wave treatment parameters
 * @param params Pointer to store loaded SW treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_LoadSWParams(SW_TreatParams_t *params)
{
    SW_TreatParams_t tempParams;
    uint16_t storedCrc;
    uint16_t calculatedCrc;
    
    if (params == NULL)
    {
        return false;
    }
    
    /* Read parameters from memory */
    if (!Drv_Memory_Read(MEM_ADDR_SW_PARAMS, (uint8_t *)&tempParams, sizeof(SW_TreatParams_t)))
    {
        return false;
    }
    
    /* Save stored CRC value */
    storedCrc = tempParams.CrcCode;
    
    /* Calculate CRC16 for all fields except CRC field itself */
    tempParams.CrcCode = 0;
    calculatedCrc = Calculate_CRC16((const uint8_t *)&tempParams, sizeof(SW_TreatParams_t) - sizeof(uint16_t));
    
    /* Verify CRC16 */
    if (calculatedCrc != storedCrc)
    {
        return false;
    }
    
    /* Restore CRC value and copy to output */
    tempParams.CrcCode = storedCrc;
    *params = tempParams;
    
    return true;
}

/**
 * @brief Save Negative Pressure Heat treatment parameters
 * @param params Pointer to NPH treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_SaveNPHParams(const NPH_TreatParams_t *params)
{
    NPH_TreatParams_t tempParams;
    uint16_t crc;
    
    if (params == NULL)
    {
        return false;
    }
    
    /* Copy parameters to temporary structure */
    tempParams = *params;
    
    /* Calculate CRC16 for all fields except CRC field itself */
    tempParams.CrcCode = 0;
    crc = Calculate_CRC16((const uint8_t *)&tempParams, sizeof(NPH_TreatParams_t) - sizeof(uint16_t));
    tempParams.CrcCode = crc;
    
    return Drv_Memory_Write(MEM_ADDR_NPH_PARAMS, (const uint8_t *)&tempParams, sizeof(NPH_TreatParams_t));
}

/**
 * @brief Load Negative Pressure Heat treatment parameters
 * @param params Pointer to store loaded NPH treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_LoadNPHParams(NPH_TreatParams_t *params)
{
    NPH_TreatParams_t tempParams;
    uint16_t storedCrc;
    uint16_t calculatedCrc;
    
    if (params == NULL)
    {
        return false;
    }
    
    /* Read parameters from memory */
    if (!Drv_Memory_Read(MEM_ADDR_NPH_PARAMS, (uint8_t *)&tempParams, sizeof(NPH_TreatParams_t)))
    {
        return false;
    }
    
    /* Save stored CRC value */
    storedCrc = tempParams.CrcCode;
    
    /* Calculate CRC16 for all fields except CRC field itself */
    tempParams.CrcCode = 0;
    calculatedCrc = Calculate_CRC16((const uint8_t *)&tempParams, sizeof(NPH_TreatParams_t) - sizeof(uint16_t));
    
    /* Verify CRC16 */
    if (calculatedCrc != storedCrc)
    {
        return false;
    }
    
    /* Restore CRC value and copy to output */
    tempParams.CrcCode = storedCrc;
    *params = tempParams;
    
    return true;
}

/**
 * @brief Save Ultrasound treatment parameters
 * @param params Pointer to US treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_SaveUSParams(const US_TreatParams_t *params)
{
    US_TreatParams_t tempParams;
    uint16_t crc;
    
    if (params == NULL)
    {
        return false;
    }
    
    /* Copy parameters to temporary structure */
    tempParams = *params;
    
    /* Calculate CRC16 for all fields except CRC field itself */
    tempParams.CrcCode = 0;
    crc = Calculate_CRC16((const uint8_t *)&tempParams, sizeof(US_TreatParams_t) - sizeof(uint16_t));
    tempParams.CrcCode = crc;
    
    return Drv_Memory_Write(MEM_ADDR_US_PARAMS, (const uint8_t *)&tempParams, sizeof(US_TreatParams_t));
}

/**
 * @brief Load Ultrasound treatment parameters
 * @param params Pointer to store loaded US treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_LoadUSParams(US_TreatParams_t *params)
{
    US_TreatParams_t tempParams;
    uint16_t storedCrc;
    uint16_t calculatedCrc;
    
    if (params == NULL)
    {
        return false;
    }
    
    /* Read parameters from memory */
    if (!Drv_Memory_Read(MEM_ADDR_US_PARAMS, (uint8_t *)&tempParams, sizeof(US_TreatParams_t)))
    {
        return false;
    }
    
    /* Save stored CRC value */
    storedCrc = tempParams.CrcCode;
    
    /* Calculate CRC16 for all fields except CRC field itself */
    tempParams.CrcCode = 0;
    calculatedCrc = Calculate_CRC16((const uint8_t *)&tempParams, sizeof(US_TreatParams_t) - sizeof(uint16_t));
    
    /* Verify CRC16 */
    if (calculatedCrc != storedCrc)
    {
        return false;
    }
    
    /* Restore CRC value and copy to output */
    tempParams.CrcCode = storedCrc;
    *params = tempParams;
    
    return true;
}

/**************************End of file********************************/
