/************************************************************************************
 * @file     : drv_24c02.c
 * @brief    : AT24C02 EEPROM driver - supports both software I2C (lib_aiic) and hardware I2C
 * @details  : Use macro DRV_24C02_USE_HW_I2C to select hardware I2C, otherwise use lib_aiic software I2C
 * @author   : \.rumi
 * @date     : 2025-01-25
 * @version  : V1.0.0
 * @copyright: Copyright (c) 2050
 ***********************************************************************************/
#include "drv_24c02.h"
#include "drv_delay.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stdbool.h"
#include "string.h"

#ifdef DRV_24C02_USE_HW_I2C
#include "bsp_i2c.h"
#else
#include "lib_aiic.h"
#endif

/* ==================== AT24C02 Constants ==================== */
#define DRV_24C02_PAGE_SIZE    8   /* 8 bytes per page */
#define DRV_24C02_CAPACITY     256

/* ==================== Software I2C Implementation (lib_aiic) ==================== */
#ifndef DRV_24C02_USE_HW_I2C

/* AT24C02 I2C node: PB10=SCL, PB11=SDA */
static iic_Node s_24c02_iic = {
    .SCLPort    = GPIOB,
    .SCL_Pin    = GPIO_Pin_10,
    .SDAPort    = GPIOB,
    .SDA_Pin    = GPIO_Pin_11,
    .SpeedDelay = 28
};

/**
 * @brief Initialize software I2C GPIO pins for lib_aiic
 */
static void Aiic_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* SCL: PB10, SDA: PB11 - open-drain output */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Release bus: SCL and SDA high */
    GPIO_SetBits(GPIOB, GPIO_Pin_10 | GPIO_Pin_11);
}

/**
 * @brief Read data using lib_aiic software I2C
 * @param pBuffer: Buffer to store read data
 * @param length: Number of bytes to read (supports full 256-byte range)
 * @param ReadAddress: Starting address
 * @param DeviceAddress: Device address (7-bit)
 * @return true if successful
 */
static bool Aiic_ReadByte(uint8_t *pBuffer, uint16_t length, uint16_t ReadAddress, uint8_t DeviceAddress)
{
    uint8_t chunk;
    /* lib_aiic Drv_IIC_ReadByte uses uint8_t for size, split if length > 255 */
    while (length > 0)
    {
        chunk = (length > 255) ? 255 : (uint8_t)length;
        if (Drv_IIC_ReadByte(&s_24c02_iic, DeviceAddress, ReadAddress, pBuffer, chunk) == 0)
            return false;
        pBuffer += chunk;
        ReadAddress += chunk;
        length -= chunk;
    }
    return true;
}

/**
 * @brief Write one byte using lib_aiic software I2C
 * @param SendByte: Byte to write
 * @param WriteAddress: Address to write
 * @param DeviceAddress: Device address (7-bit)
 * @return true if successful
 */
static bool Aiic_WriteByte(uint8_t SendByte, uint16_t WriteAddress, uint8_t DeviceAddress)
{
    Drv_IIC_WriteReg(&s_24c02_iic, DeviceAddress, (uint8_t)WriteAddress, SendByte);

    /* Wait for EEPROM write completion (typically 5-10ms) */
    Drv_Delay_ms(10);

    return true;
}

/**
 * @brief Page write: write up to 8 bytes within one page (no cross-page)
 * @param pBuffer: Data buffer
 * @param length: Bytes to write (1~8, must not cross page boundary)
 * @param WriteAddress: Starting address (must be page-aligned for the chunk)
 * @param DeviceAddress: Device address (7-bit)
 * @return true if successful
 */
static bool Aiic_WritePage(uint8_t *pBuffer, uint8_t length, uint16_t WriteAddress, uint8_t DeviceAddress)
{
    if (length == 0 || length > DRV_24C02_PAGE_SIZE) return false;

    Drv_IIC_WriteByte(&s_24c02_iic, DeviceAddress, (uint8_t)WriteAddress, pBuffer, length);

    /* Wait for EEPROM page write completion (typically 5-10ms) */
    Drv_Delay_ms(10);

    return true;
}

#endif /* !DRV_24C02_USE_HW_I2C */

/* ==================== Hardware I2C Implementation ==================== */
#ifdef DRV_24C02_USE_HW_I2C

/**
 * @brief Read data using hardware I2C
 * @param pBuffer: Buffer to store read data
 * @param length: Number of bytes to read
 * @param ReadAddress: Starting address
 * @param DeviceAddress: Device address (7-bit, left-aligned)
 * @return true if successful
 */
static bool HwI2C_ReadByte(uint8_t *pBuffer, uint16_t length, uint16_t ReadAddress, uint8_t DeviceAddress)
{
    uint8_t addr_buf[2];
    int ret;
    
    /* AT24C02 uses 8-bit address, split into high and low bytes */
    addr_buf[0] = (uint8_t)(ReadAddress & 0x00FF);
    
#if DRV_24C02_HW_I2C_PORT == 1
    /* Write address pointer */
    ret = BSP_I2C1_Transmit(DeviceAddress, addr_buf, 1);
    if(ret != 0) return false;
    
    /* Read data (hardware I2C automatically handles read bit) */
    ret = BSP_I2C1_Receive(DeviceAddress, pBuffer, length);
#else
    /* Write address pointer */
    ret = BSP_I2C2_Transmit(DeviceAddress, addr_buf, 1);
    if(ret != 0) return false;
    
    /* Read data (hardware I2C automatically handles read bit) */
    ret = BSP_I2C2_Receive(DeviceAddress, pBuffer, length);
#endif
    
    return (ret == 0);
}

/**
 * @brief Write one byte using hardware I2C
 * @param SendByte: Byte to write
 * @param WriteAddress: Address to write
 * @param DeviceAddress: Device address (7-bit, left-aligned)
 * @return true if successful
 */
static bool HwI2C_WriteByte(uint8_t SendByte, uint16_t WriteAddress, uint8_t DeviceAddress)
{
    uint8_t tx_buf[2];
    int ret;
    
    /* AT24C02 uses 8-bit address */
    tx_buf[0] = (uint8_t)(WriteAddress & 0x00FF);
    tx_buf[1] = SendByte;
    
#if DRV_24C02_HW_I2C_PORT == 1
    ret = BSP_I2C1_Transmit(DeviceAddress, tx_buf, 2);
#else
    ret = BSP_I2C2_Transmit(DeviceAddress, tx_buf, 2);
#endif
    
    if(ret != 0) return false;
    
    /* Wait for EEPROM write completion (typically 5-10ms) */
    Drv_Delay_ms(10);
    
    return true;
}

/**
 * @brief Page write: write up to 8 bytes within one page (no cross-page)
 * @param pBuffer: Data buffer
 * @param length: Bytes to write (1~8, must not cross page boundary)
 * @param WriteAddress: Starting address
 * @param DeviceAddress: Device address (7-bit, left-aligned)
 * @return true if successful
 */
static bool HwI2C_WritePage(uint8_t *pBuffer, uint8_t length, uint16_t WriteAddress, uint8_t DeviceAddress)
{
    uint8_t tx_buf[1 + DRV_24C02_PAGE_SIZE];  /* addr + max 8 data bytes */
    uint8_t i;
    int ret;
    
    if (length == 0 || length > DRV_24C02_PAGE_SIZE) return false;
    
    tx_buf[0] = (uint8_t)(WriteAddress & 0x00FF);
    for (i = 0; i < length; i++) {
        tx_buf[1 + i] = pBuffer[i];
    }
    
#if DRV_24C02_HW_I2C_PORT == 1
    ret = BSP_I2C1_Transmit(DeviceAddress, tx_buf, 1 + length);
#else
    ret = BSP_I2C2_Transmit(DeviceAddress, tx_buf, 1 + length);
#endif
    
    if (ret != 0) return false;
    
    /* Wait for EEPROM page write completion (typically 5-10ms) */
    Drv_Delay_ms(10);
    
    return true;
}

#endif /* DRV_24C02_USE_HW_I2C */

/* ==================== Public API Implementation ==================== */

void Drv_24C02_Init(void)
{
#ifndef DRV_24C02_USE_HW_I2C
    Aiic_Init();
#endif
    /* For hardware I2C, initialization should be done by BSP_I2C1_Init() or BSP_I2C2_Init() */
}

bool Drv_24C02_Read(uint8_t *pBuffer, uint16_t length, uint16_t ReadAddress)
{
    if (pBuffer == NULL || length == 0) return false;
    if (ReadAddress >= DRV_24C02_CAPACITY) return false;
    if ((ReadAddress + length) > DRV_24C02_CAPACITY) return false;
    
#ifdef DRV_24C02_USE_HW_I2C
    return HwI2C_ReadByte(pBuffer, length, ReadAddress, DRV_24C02_DEV_ADDR_7BIT);
#else
    return Aiic_ReadByte(pBuffer, length, ReadAddress, DRV_24C02_DEV_ADDR_7BIT);
#endif
}

bool Drv_24C02_WriteByte(uint8_t SendByte, uint16_t WriteAddress)
{
    if (WriteAddress >= DRV_24C02_CAPACITY) return false;
    
#ifdef DRV_24C02_USE_HW_I2C
    return HwI2C_WriteByte(SendByte, WriteAddress, DRV_24C02_DEV_ADDR_7BIT);
#else
    return Aiic_WriteByte(SendByte, WriteAddress, DRV_24C02_DEV_ADDR_7BIT);
#endif
}

/**
 * @brief Write multiple bytes with automatic page boundary handling
 * @details User only needs to provide (pBuffer, length, WriteAddress).
 *          Driver internally splits writes by 8-byte page boundaries.
 */
bool Drv_24C02_Write(uint8_t *pBuffer, uint16_t length, uint16_t WriteAddress)
{
    uint16_t remain;
    uint8_t chunk;
    
    if (pBuffer == NULL || length == 0) return false;
    if (WriteAddress >= DRV_24C02_CAPACITY) return false;
    if ((WriteAddress + length) > DRV_24C02_CAPACITY) return false;
    
    remain = length;
    while (remain > 0)
    {
        /* Bytes remaining in current page (0x00~0x07, 0x08~0x0F, ...) */
        chunk = DRV_24C02_PAGE_SIZE - (WriteAddress % DRV_24C02_PAGE_SIZE);
        if (chunk > remain) chunk = (uint8_t)remain;
        
#ifdef DRV_24C02_USE_HW_I2C
        if (!HwI2C_WritePage(pBuffer, chunk, WriteAddress, DRV_24C02_DEV_ADDR_7BIT))
            return false;
#else
        if (!Aiic_WritePage(pBuffer, chunk, WriteAddress, DRV_24C02_DEV_ADDR_7BIT))
            return false;
#endif
        pBuffer += chunk;
        WriteAddress += chunk;
        remain -= chunk;
    }
    
    return true;
}

/**************************End of file********************************/
