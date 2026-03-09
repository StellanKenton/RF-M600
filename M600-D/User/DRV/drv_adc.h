/************************************************************************************
 * @file     : drv_adc.h
 * @brief    : ADC驱动层 - 提供电压值和处理后的物理量
 ***********************************************************************************/
#ifndef DRV_ADC_H
#define DRV_ADC_H

#include "bsp_adc.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 旧版兼容定义 ==================== */
/* NTC类型枚举（兼容旧代码） */
typedef enum {
    E_NTC_HAND = 0,   /* 换能器/手柄NTC (transducer IGBT temp) - ADC13 */
    E_NTC_MAIN,       /* 电路板NTC (circuit board temp) - ADC6 Heat_REF01 */
    E_NTC_MAX
} NTC_Type_EnumDef;

/* ADC通道枚举（兼容旧代码） */
typedef enum {
    E_ADC_CHANNEL_US_I = 0,
    E_ADC_CHANNEL_RF_I,
    E_ADC_CHANNEL_Heat_REF02,
    E_ADC_CHANNEL_Heat_REF01,
    E_ADC_CHANNEL_ESW_U,
    E_ADC_CHANNEL_ESW_I,
    E_ADC_CHANNEL_HP_PRE,
    E_ADC_CHANNEL_HAND_NTC,
    E_ADC_CHANNEL_VER_ID,   /* 保留，返回0 */
    E_ADC_CHANNEL_VOUT,
    E_ADC_CHANNEL_MAX
} ADC_Channel_EnumDef;

/* NTC温度故障码（兼容旧代码） */
#define NTC_FAULT_OPEN  0xFFFFu
#define NTC_FAULT_SHORT 0xFFEEu

/* ==================== 新版接口定义 ==================== */
/* 通道类型定义 */
typedef enum {
    DRV_ADC_CH_US_I = 0,       /* 超声电流 */
    DRV_ADC_CH_RF_I,           /* 射频电流 */
    DRV_ADC_CH_Heat_REF02,     /* 加热参考2 */
    DRV_ADC_CH_Heat_REF01,     /* 加热参考1/主板NTC */
    DRV_ADC_CH_ESW_U,          /* ESW电压 */
    DRV_ADC_CH_ESW_I,          /* ESW电流 */
    DRV_ADC_CH_HP_PRE,         /* 高压预压 */
    DRV_ADC_CH_HAND_NTC,       /* 手柄NTC */
    DRV_ADC_CH_VOUT,           /* 输出电压 */
    DRV_ADC_CH_MAX
} DRV_ADC_Channel_t;

/* NTC温度故障码 */
#define DRV_ADC_NTC_FAULT_OPEN  0xFFFFu  /* NTC开路 */
#define DRV_ADC_NTC_FAULT_SHORT 0xFFEEu  /* NTC短路 */

/* ==================== 新版接口 ==================== */
/**
 * @brief  读取指定通道的原始ADC值
 * @param  ch: ADC通道
 * @retval 原始ADC值 (0-4095)
 */
uint16_t DRV_ADC_ReadRaw(BSP_ADC_Channel_t ch);

/**
 * @brief  读取指定通道的电压值
 * @param  ch: ADC通道
 * @retval 电压值 (mV)
 */
uint16_t DRV_ADC_ReadVoltage(BSP_ADC_Channel_t ch);

/**
 * @brief  读取指定通道的处理后的物理量
 * @param  ch: ADC通道
 * @retval 处理后的值，根据通道类型不同：
 *         - 温度通道(NTC): 返回(温度+40)*10，范围0~1450对应-40~105°C
 *                         故障时返回DRV_ADC_NTC_FAULT_OPEN或DRV_ADC_NTC_FAULT_SHORT
 *         - 电流通道: 返回电流值(mA)
 *         - 电压通道: 返回电压值(mV)
 *         - VOUT通道: 返回补偿后的输出电压(mV)
 */
uint16_t DRV_ADC_GetProcessedValue(BSP_ADC_Channel_t ch);

/* ==================== 旧版兼容接口 ==================== */
/**
 * @brief  读取指定通道的原始ADC值（兼容旧代码）
 * @param  channel: ADC通道（旧枚举）
 * @retval 原始ADC值 (0-4095)
 */
uint16_t Drv_ADC_ReadChannel(ADC_Channel_EnumDef channel);

/**
 * @brief  读取指定通道的电压值（兼容旧代码）
 * @param  channel: ADC通道（旧枚举）
 * @retval 电压值 (mV)，返回uint32_t以兼容旧代码
 */
uint32_t Drv_ADC_ReadVoltage(ADC_Channel_EnumDef channel);

/**
 * @brief  读取VOUT电压（兼容旧代码）
 * @retval VOUT采样电压 (mV)
 */
uint16_t Drv_ADC_ReadVOUT(void);

/**
 * @brief  读取VOUT补偿后的电压（兼容旧代码）
 * @retval VOUT补偿后电压 (mV)
 */
uint16_t Drv_GetADCVout(void);

/**
 * @brief  读取指定通道的处理后的值（兼容旧代码）
 * @param  channel: ADC通道（旧枚举）
 * @retval 处理后的值 (mV)
 */
uint16_t Drv_ADC_GetRealValue(ADC_Channel_EnumDef channel);

/**
 * @brief  读取NTC温度值（兼容旧代码）
 * @param  ntcType: NTC类型
 * @retval 温度值：(温度+40)*10，范围0~1450对应-40~105°C
 *         故障时返回NTC_FAULT_OPEN或NTC_FAULT_SHORT
 */
uint16_t Drv_ADC_GetNTCValue(NTC_Type_EnumDef ntcType);

#ifdef __cplusplus
}
#endif

#endif /* DRV_ADC_H */
