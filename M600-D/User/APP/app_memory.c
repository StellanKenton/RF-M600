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
#include "app_comm.h"
#include <string.h>
#include "log.h"

/* CRC16 polynomial: CRC-16-IBM (0x8005) */
#define CRC16_POLYNOMIAL 0x8005
#define CRC16_INIT_VALUE 0xFFFF

/* Memory address definitions for each treatment parameter structure */
#define MEM_ADDR_RF_PARAMS      0x0080      ///< Radio Frequency parameters address (base=128, step=32)
#define MEM_ADDR_SW_PARAMS      0x00A0      ///< Shock Wave parameters address
#define MEM_ADDR_NPH_PARAMS     0x00C0      ///< Negative Pressure Heat parameters address
#define MEM_ADDR_US_PARAMS      0x00E0      ///< Ultrasound parameters address

/* Static parameter storage - loaded at init, used by Get functions */
static RF_TreatParams_t  s_RFParams;
static SW_TreatParams_t  s_SWParams;
static NPH_TreatParams_t s_NPHParams;
static US_TreatParams_t  s_USParams;

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
 * @brief Initialize memory module - load all treatment params from memory
 *        Uses default values if load fails
 */
void App_Memory_Init(void)
{
    Drv_Memory_Init();

    /* Load RF params */
    if (!App_Memory_LoadRFParams(&s_RFParams))
    {
        memset(&s_RFParams, 0, sizeof(s_RFParams));
        s_RFParams.TempLimit = 400;
        s_RFParams.TreatRemainTimes = 0;
        s_RFParams.CurrentHigh = 1000;
        s_RFParams.CurrentLow = 500;
        LOG_W("Failed to load RF params, using defaults");
    } else {
        LOG_I("RF: Parameters loaded - temp_limit=%d, remain_times=%d, current=[%d, %d]",
                      s_RFParams.TempLimit, s_RFParams.TreatRemainTimes,
                      s_RFParams.CurrentLow, s_RFParams.CurrentHigh);
    }

    /* Load SW params */
    if (!App_Memory_LoadSWParams(&s_SWParams))
    {
        memset(&s_SWParams, 0, sizeof(s_SWParams));
        s_SWParams.TempLimit = 400;
        s_SWParams.CurrentHigh_ESW_P = 1000;
        s_SWParams.CurrentLow_ESW_P = 500;
        s_SWParams.CurrentHigh_ESW_N = 1000;
        s_SWParams.CurrentLow_ESW_N = 50;
        LOG_W("Failed to load SW params, using defaults");
    } else {
        LOG_I("SW: Parameters loaded - temp_limit=%d, ESW_P=[%d, %d], ESW_N=[%d, %d]",
                      s_SWParams.TempLimit,
                      s_SWParams.CurrentLow_ESW_P, s_SWParams.CurrentHigh_ESW_P,
                      s_SWParams.CurrentLow_ESW_N, s_SWParams.CurrentHigh_ESW_N);
    }

    /* Load NPH params */
    if (!App_Memory_LoadNPHParams(&s_NPHParams))
    {
        memset(&s_NPHParams, 0, sizeof(s_NPHParams));
        s_NPHParams.TempLimit = 400;
        s_NPHParams.PreheatEnable = 0;
        LOG_W("Failed to load NPH params, using defaults");
    } else {
        LOG_I("NPH: Parameters loaded - temp_limit=%d, preheat_enable=%d, preheat_temp=%d, preheat_time=%d",
                      s_NPHParams.TempLimit, s_NPHParams.PreheatEnable,
                      s_NPHParams.PreheatTempLimit, s_NPHParams.PreheatTime);
    }

    /* Load US params */
    if (!App_Memory_LoadUSParams(&s_USParams))
    {
        memset(&s_USParams, 0, sizeof(s_USParams));
        s_USParams.Frequency = 1200;
        s_USParams.TempLimit = 400;
        s_USParams.Voltage = 1500;
        s_USParams.CurrentHigh = 1000;
        s_USParams.CurrentLow = 500;
        LOG_W("Failed to load US params, using defaults");
    } else {
        LOG_I("US: Parameters loaded - freq=%d, temp_limit=%d, voltage=%d, current=[%d, %d]",
                      s_USParams.Frequency, s_USParams.TempLimit, s_USParams.Voltage,
                      s_USParams.CurrentLow, s_USParams.CurrentHigh);
    }
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

    s_RFParams = *params;
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

    s_SWParams = *params;
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

    s_NPHParams = *params;
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

    s_USParams = *params;
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

/**
 * @brief Get RF treatment parameters (static storage, loaded at init)
 * @retval Pointer to RF params, never NULL
 */
const RF_TreatParams_t *App_Memory_GetRFParams(void)
{
    return &s_RFParams;
}

/**
 * @brief Get SW treatment parameters (static storage, loaded at init)
 * @retval Pointer to SW params, never NULL
 */
const SW_TreatParams_t *App_Memory_GetSWParams(void)
{
    return &s_SWParams;
}

/**
 * @brief Get NPH treatment parameters (static storage, loaded at init)
 * @retval Pointer to NPH params, never NULL
 */
const NPH_TreatParams_t *App_Memory_GetNPHParams(void)
{
    return &s_NPHParams;
}

/**
 * @brief Get US treatment parameters (static storage, loaded at init)
 * @retval Pointer to US params, never NULL
 */
const US_TreatParams_t *App_Memory_GetUSParams(void)
{
    return &s_USParams;
}

void App_Memory_Process(void)
{
    UltraSound_TransData_t *pUS = App_Comm_GetUSTransData();
    RF_TransData_t *pRF = App_Comm_GetRFTransData();
    SW_TransData_t *pSW = App_Comm_GetSWTransData();
    Heat_TransData_t *pHeat = App_Comm_GetHeatTransData();

    (void)pSW;

    if(pUS != NULL && pUS->flag.bits.Process_Config == 1)
    {
        s_USParams.Frequency = pUS->RxConfig.frequency;
        s_USParams.TempLimit = pUS->RxConfig.temp_limit;
        s_USParams.Voltage   = pUS->RxConfig.voltage;

        App_Memory_SaveUSParams(&s_USParams);
        pUS->flag.bits.Process_Config = 0;
    }

    if(pRF != NULL && pRF->flag.bits.Process_Config == 1)
    {
        s_RFParams.TempLimit = pRF->RxConfig.temp_limit;

        App_Memory_SaveRFParams(&s_RFParams);
        pRF->flag.bits.Process_Config = 0;
    }

    if(pHeat != NULL && pHeat->flag.bits.Process_Config == 1)
    {
        if (pHeat->RxConfig.preheat_state == 0x01)
        {
            s_NPHParams.PreheatEnable    = 1;
            s_NPHParams.PreheatTempLimit = pHeat->RxConfig.temp_limit;
            s_NPHParams.PreheatTime      = pHeat->RxConfig.work_time;
        }
        else
        {
            s_NPHParams.PreheatEnable = 0;
        }

        App_Memory_SaveNPHParams(&s_NPHParams);
        pHeat->flag.bits.Process_Config = 0;
    }


}

/**************************End of file********************************/
