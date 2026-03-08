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
#include "drv_delay.h"
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

    s_RFParams = tempParams;
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

    s_SWParams = tempParams;
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

    s_NPHParams = tempParams;
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

    s_USParams = tempParams;
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

/**
 * @brief Process Ultrasound configuration
 */
static void App_Memory_ProcessUSConfig(void)
{
    UltraSound_TransData_t *pUS = App_Comm_GetUSTransData();

    if (pUS != NULL && pUS->flag.bits.Process_Config == 1)
    {
        uint8_t freq_result   = CONFIG_RESULT_SUCCESS;
        uint8_t voltage_result = CONFIG_RESULT_SUCCESS;
        uint8_t temp_result   = CONFIG_RESULT_SUCCESS;
        uint8_t current_high_result = CONFIG_RESULT_SUCCESS;
        uint8_t current_low_result = CONFIG_RESULT_SUCCESS;
        uint8_t remain_count_result = CONFIG_RESULT_SUCCESS;

        /* Check over limit */
        if (pUS->RxConfig.frequency < PARAM_US_FREQ_MIN || pUS->RxConfig.frequency > PARAM_US_FREQ_MAX)
        {
            freq_result = CONFIG_RESULT_OVER_LIMIT;
        }
        if (pUS->RxConfig.voltage < PARAM_US_VOLTAGE_MIN || pUS->RxConfig.voltage > PARAM_US_VOLTAGE_MAX)
        {
            voltage_result = CONFIG_RESULT_OVER_LIMIT;
        }
        if (pUS->RxConfig.temp_limit < PARAM_TEMP_MIN || pUS->RxConfig.temp_limit > PARAM_TEMP_MAX)
        {
            temp_result = CONFIG_RESULT_OVER_LIMIT;
        }

        if (freq_result == CONFIG_RESULT_SUCCESS && voltage_result == CONFIG_RESULT_SUCCESS && 
            temp_result == CONFIG_RESULT_SUCCESS && current_high_result == CONFIG_RESULT_SUCCESS &&
            current_low_result == CONFIG_RESULT_SUCCESS && remain_count_result == CONFIG_RESULT_SUCCESS)
        {
            US_TreatParams_t readback;
            s_USParams.Frequency = pUS->RxConfig.frequency;
            s_USParams.TempLimit = pUS->RxConfig.temp_limit;
            s_USParams.Voltage   = pUS->RxConfig.voltage;
            s_USParams.CurrentHigh = pUS->RxConfig.Current_HighLimit;
            s_USParams.CurrentLow = pUS->RxConfig.Current_LowLimit;
            s_USParams.TreatRemainTimes = pUS->RxConfig.remain_treatment_count;

            if (App_Memory_SaveUSParams(&s_USParams) == true)
            {
                if (!App_Memory_LoadUSParams(&readback) || memcmp(&readback, &s_USParams, sizeof(US_TreatParams_t)) != 0)
                {
                    freq_result = voltage_result = temp_result = CONFIG_RESULT_FAIL;
                    current_high_result = current_low_result = remain_count_result = CONFIG_RESULT_FAIL;
                }
            }
            else {
                freq_result = voltage_result = temp_result = CONFIG_RESULT_FAIL;
                current_high_result = current_low_result = remain_count_result = CONFIG_RESULT_FAIL;
            }
        }

        pUS->TxConfig.freq_result    = freq_result;
        pUS->TxConfig.voltage_result = voltage_result;
        pUS->TxConfig.temp_result   = temp_result;
        pUS->TxConfig.current_highlimit_result = current_high_result;
        pUS->TxConfig.current_lowlimit_result = current_low_result;
        pUS->TxConfig.remain_treatment_count_result = remain_count_result;
        pUS->flag.bits.Rely_Config  = 1;
        pUS->flag.bits.Process_Config = 0;
    }
}

/**
 * @brief Process Radio Frequency configuration
 */
static void App_Memory_ProcessRFConfig(void)
{
    RF_TransData_t *pRF = App_Comm_GetRFTransData();

    if (pRF != NULL && pRF->flag.bits.Process_Config == 1)
    {
        uint8_t temp_result = CONFIG_RESULT_SUCCESS;
        uint8_t current_high_result = CONFIG_RESULT_SUCCESS;
        uint8_t current_low_result = CONFIG_RESULT_SUCCESS;
        uint8_t remain_count_result = CONFIG_RESULT_SUCCESS;

        if (pRF->RxConfig.temp_limit < PARAM_TEMP_MIN || pRF->RxConfig.temp_limit > PARAM_TEMP_MAX)
        {
            temp_result = CONFIG_RESULT_OVER_LIMIT;
        }

        if (temp_result == CONFIG_RESULT_SUCCESS && current_high_result == CONFIG_RESULT_SUCCESS &&
            current_low_result == CONFIG_RESULT_SUCCESS && remain_count_result == CONFIG_RESULT_SUCCESS)
        {
            RF_TreatParams_t readback;
            s_RFParams.TempLimit = pRF->RxConfig.temp_limit;
            s_RFParams.CurrentHigh = pRF->RxConfig.Current_HighLimit;
            s_RFParams.CurrentLow = pRF->RxConfig.Current_LowLimit;
            s_RFParams.TreatRemainTimes = pRF->RxConfig.remain_treatment_count;

            if (App_Memory_SaveRFParams(&s_RFParams) == true)
            {
                if (!App_Memory_LoadRFParams(&readback) || memcmp(&readback, &s_RFParams, sizeof(RF_TreatParams_t)) != 0)
                {
                    temp_result = CONFIG_RESULT_FAIL;
                    current_high_result = current_low_result = remain_count_result = CONFIG_RESULT_FAIL;
                }
            }
            else
            {
                temp_result = CONFIG_RESULT_FAIL;
                current_high_result = current_low_result = remain_count_result = CONFIG_RESULT_FAIL;
            }
        }

        pRF->TxConfig.temp_result   = temp_result;
        pRF->TxConfig.current_highlimit_result = current_high_result;
        pRF->TxConfig.current_lowlimit_result = current_low_result;
        pRF->TxConfig.remain_treatment_count_result = remain_count_result;
        pRF->flag.bits.Rely_Config  = 1;
        pRF->flag.bits.Process_Config = 0;
    }
}

/**
 * @brief Process Heat configuration
 */
static void App_Memory_ProcessHeatConfig(void)
{
    Heat_TransData_t *pHeat = App_Comm_GetHeatTransData();

    if (pHeat != NULL && pHeat->flag.bits.Process_Config == 1)
    {
        uint8_t preheat_state_result = CONFIG_RESULT_SUCCESS;
        uint8_t work_time_result = CONFIG_RESULT_SUCCESS;
        uint8_t temp_limit_result = CONFIG_RESULT_SUCCESS;
        uint8_t preheat_temp_result = CONFIG_RESULT_SUCCESS;
        uint8_t remain_count_result = CONFIG_RESULT_SUCCESS;

        /* Validate temp_limit */
        if (pHeat->RxConfig.temp_limit < PARAM_TEMP_MIN || pHeat->RxConfig.temp_limit > PARAM_TEMP_MAX)
        {
            temp_limit_result = CONFIG_RESULT_OVER_LIMIT;
        }

        /* Validate preheat_temp_limit */
        if (pHeat->RxConfig.preheat_temp_limit < PARAM_TEMP_MIN || pHeat->RxConfig.preheat_temp_limit > PARAM_TEMP_MAX)
        {
            preheat_temp_result = CONFIG_RESULT_OVER_LIMIT;
        }

        /* Validate work_time */
        if (pHeat->RxConfig.work_time > PARAM_WORK_TIME_MAX)
        {
            work_time_result = CONFIG_RESULT_OVER_LIMIT;
        }

        if (preheat_state_result == CONFIG_RESULT_SUCCESS && work_time_result == CONFIG_RESULT_SUCCESS &&
            temp_limit_result == CONFIG_RESULT_SUCCESS && preheat_temp_result == CONFIG_RESULT_SUCCESS &&
            remain_count_result == CONFIG_RESULT_SUCCESS)
        {
            if (pHeat->RxConfig.preheat_state == 0x01)
            {
                s_NPHParams.PreheatEnable    = 1;
                s_NPHParams.PreheatTempLimit = pHeat->RxConfig.preheat_temp_limit;
                s_NPHParams.PreheatTime      = pHeat->RxConfig.work_time;
            }
            else
            {
                s_NPHParams.PreheatEnable = 0;
            }
            
            s_NPHParams.TempLimit = pHeat->RxConfig.temp_limit;
            s_NPHParams.TreatRemainTimes = pHeat->RxConfig.remain_treatment_count;

            NPH_TreatParams_t readback;
            if (App_Memory_SaveNPHParams(&s_NPHParams) == true)
            {
                if (!App_Memory_LoadNPHParams(&readback) || memcmp(&readback, &s_NPHParams, sizeof(NPH_TreatParams_t)) != 0)
                {
                    preheat_state_result = work_time_result = temp_limit_result = CONFIG_RESULT_FAIL;
                    preheat_temp_result = remain_count_result = CONFIG_RESULT_FAIL;
                }
            }
            else
            {
                preheat_state_result = work_time_result = temp_limit_result = CONFIG_RESULT_FAIL;
                preheat_temp_result = remain_count_result = CONFIG_RESULT_FAIL;
            }
        }

        pHeat->TxConfig.preheat_state_result = preheat_state_result;
        pHeat->TxConfig.work_time_result = work_time_result;
        pHeat->TxConfig.temp_limit_result = temp_limit_result;
        pHeat->TxConfig.preheat_temp_limit_result = preheat_temp_result;
        pHeat->TxConfig.remain_treatment_count_result = remain_count_result;
        pHeat->flag.bits.Rely_Config  = 1;
        pHeat->flag.bits.Process_Config = 0;
    }
}

/**
 * @brief Process Shock Wave configuration
 */
static void App_Memory_ProcessSWConfig(void)
{
    SW_TransData_t *pSW = App_Comm_GetSWTransData();

    if (pSW != NULL && pSW->flag.bits.Process_Config == 1)
    {
        uint8_t temp_result = CONFIG_RESULT_SUCCESS;
        uint8_t esw_p_high_result = CONFIG_RESULT_SUCCESS;
        uint8_t esw_p_low_result = CONFIG_RESULT_SUCCESS;
        uint8_t remain_count_result = CONFIG_RESULT_SUCCESS;
        uint8_t esw_n_high_result = CONFIG_RESULT_SUCCESS;
        uint8_t esw_n_low_result = CONFIG_RESULT_SUCCESS;

        /* Validate temp_limit */
        if (pSW->RxConfig.temp_limit < PARAM_TEMP_MIN || pSW->RxConfig.temp_limit > PARAM_TEMP_MAX)
        {
            temp_result = CONFIG_RESULT_OVER_LIMIT;
        }

        if (temp_result == CONFIG_RESULT_SUCCESS && esw_p_high_result == CONFIG_RESULT_SUCCESS &&
            esw_p_low_result == CONFIG_RESULT_SUCCESS && remain_count_result == CONFIG_RESULT_SUCCESS &&
            esw_n_high_result == CONFIG_RESULT_SUCCESS && esw_n_low_result == CONFIG_RESULT_SUCCESS)
        {
            SW_TreatParams_t readback;
            s_SWParams.TempLimit = pSW->RxConfig.temp_limit;
            s_SWParams.CurrentHigh_ESW_P = pSW->RxConfig.ESW_P_Current_HighLimit;
            s_SWParams.CurrentLow_ESW_P = pSW->RxConfig.ESW_P_Current_LowLimit;
            s_SWParams.TreatRemainTimes = pSW->RxConfig.remain_treatment_count;
            s_SWParams.CurrentHigh_ESW_N = pSW->RxConfig.ESW_N_Current_HighLimit;
            s_SWParams.CurrentLow_ESW_N = pSW->RxConfig.ESW_N_Current_LowLimit;

            if (App_Memory_SaveSWParams(&s_SWParams) == true)
            {
                if (!App_Memory_LoadSWParams(&readback) || memcmp(&readback, &s_SWParams, sizeof(SW_TreatParams_t)) != 0)
                {
                    temp_result = esw_p_high_result = esw_p_low_result = CONFIG_RESULT_FAIL;
                    remain_count_result = esw_n_high_result = esw_n_low_result = CONFIG_RESULT_FAIL;
                }
            }
            else
            {
                temp_result = esw_p_high_result = esw_p_low_result = CONFIG_RESULT_FAIL;
                remain_count_result = esw_n_high_result = esw_n_low_result = CONFIG_RESULT_FAIL;
            }
        }

        pSW->TxConfig.temp_result = temp_result;
        pSW->TxConfig.ESW_P_current_highlimit_result = esw_p_high_result;
        pSW->TxConfig.ESW_P_current_lowlimit_result = esw_p_low_result;
        pSW->TxConfig.remain_treatment_count_result = remain_count_result;
        pSW->TxConfig.ESW_N_current_highlimit_result = esw_n_high_result;
        pSW->TxConfig.ESW_N_current_lowlimit_result = esw_n_low_result;
        pSW->flag.bits.Rely_Config = 1;
        pSW->flag.bits.Process_Config = 0;
    }
}

/**
 * @brief Process all treatment parameter configurations
 */
void App_Memory_Process(void)
{
	static Drv_Timer_t CommTimer;
    if(Drv_Timer_Tick(&CommTimer, MEM_TASK_TIME) == false){
        return;
    }
    App_Memory_ProcessUSConfig();
    App_Memory_ProcessRFConfig();
    App_Memory_ProcessHeatConfig();
    App_Memory_ProcessSWConfig();
}

/**************************End of file********************************/
