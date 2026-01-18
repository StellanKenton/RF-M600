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

/* Memory address definitions for each treatment parameter structure */
#define MEM_ADDR_RF_PARAMS      0x0000      ///< Radio Frequency parameters address
#define MEM_ADDR_SW_PARAMS      0x0010      ///< Shock Wave parameters address
#define MEM_ADDR_NPH_PARAMS     0x0020      ///< Negative Pressure Heat parameters address
#define MEM_ADDR_US_PARAMS      0x0030      ///< Ultrasound parameters address

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
    if (params == NULL)
    {
        return false;
    }
    
    return Drv_Memory_Write(MEM_ADDR_RF_PARAMS, (const uint8_t *)params, sizeof(RF_TreatParams_t));
}

/**
 * @brief Load Radio Frequency treatment parameters
 * @param params Pointer to store loaded RF treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_LoadRFParams(RF_TreatParams_t *params)
{
    if (params == NULL)
    {
        return false;
    }
    
    return Drv_Memory_Read(MEM_ADDR_RF_PARAMS, (uint8_t *)params, sizeof(RF_TreatParams_t));
}

/**
 * @brief Save Shock Wave treatment parameters
 * @param params Pointer to SW treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_SaveSWParams(const SW_TreatParams_t *params)
{
    if (params == NULL)
    {
        return false;
    }
    
    return Drv_Memory_Write(MEM_ADDR_SW_PARAMS, (const uint8_t *)params, sizeof(SW_TreatParams_t));
}

/**
 * @brief Load Shock Wave treatment parameters
 * @param params Pointer to store loaded SW treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_LoadSWParams(SW_TreatParams_t *params)
{
    if (params == NULL)
    {
        return false;
    }
    
    return Drv_Memory_Read(MEM_ADDR_SW_PARAMS, (uint8_t *)params, sizeof(SW_TreatParams_t));
}

/**
 * @brief Save Negative Pressure Heat treatment parameters
 * @param params Pointer to NPH treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_SaveNPHParams(const NPH_TreatParams_t *params)
{
    if (params == NULL)
    {
        return false;
    }
    
    return Drv_Memory_Write(MEM_ADDR_NPH_PARAMS, (const uint8_t *)params, sizeof(NPH_TreatParams_t));
}

/**
 * @brief Load Negative Pressure Heat treatment parameters
 * @param params Pointer to store loaded NPH treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_LoadNPHParams(NPH_TreatParams_t *params)
{
    if (params == NULL)
    {
        return false;
    }
    
    return Drv_Memory_Read(MEM_ADDR_NPH_PARAMS, (uint8_t *)params, sizeof(NPH_TreatParams_t));
}

/**
 * @brief Save Ultrasound treatment parameters
 * @param params Pointer to US treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_SaveUSParams(const US_TreatParams_t *params)
{
    if (params == NULL)
    {
        return false;
    }
    
    return Drv_Memory_Write(MEM_ADDR_US_PARAMS, (const uint8_t *)params, sizeof(US_TreatParams_t));
}

/**
 * @brief Load Ultrasound treatment parameters
 * @param params Pointer to store loaded US treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_LoadUSParams(US_TreatParams_t *params)
{
    if (params == NULL)
    {
        return false;
    }
    
    return Drv_Memory_Read(MEM_ADDR_US_PARAMS, (uint8_t *)params, sizeof(US_TreatParams_t));
}

/**************************End of file********************************/
