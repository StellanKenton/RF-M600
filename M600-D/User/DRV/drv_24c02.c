/************************************************************************************
 * @file     : drv_24c02.c
 * @brief    : AT24C02 EEPROM driver - supports both software I2C and hardware I2C
 * @details  : Use macro DRV_24C02_USE_HW_I2C to select hardware I2C, otherwise use software I2C
 * @author   : \.rumi
 * @date     : 2025-01-25
 * @version  : V1.0.0
 * @copyright: Copyright (c) 2050
 ***********************************************************************************/
#include "drv_24c02.h"
#include "bsp_delay.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#ifdef DRV_24C02_USE_HW_I2C
#include "bsp_i2c.h"
#else
/* Software I2C GPIO definitions for I2C2 (PB10=SCL, PB11=SDA) */
#define I2C2_SCL_PIN     GPIO_Pin_10
#define I2C2_SCL_PORT    GPIOB
#define I2C2_SDA_PIN     GPIO_Pin_11
#define I2C2_SDA_PORT    GPIOB

/* Software I2C GPIO control macros */
#define SDA2_H()         GPIO_SetBits(I2C2_SDA_PORT, I2C2_SDA_PIN)
#define SDA2_L()         GPIO_ResetBits(I2C2_SDA_PORT, I2C2_SDA_PIN)
#define SCL2_H()         GPIO_SetBits(I2C2_SCL_PORT, I2C2_SCL_PIN)
#define SCL2_L()         GPIO_ResetBits(I2C2_SCL_PORT, I2C2_SCL_PIN)
#define SDA2_READ()      GPIO_ReadInputDataBit(I2C2_SDA_PORT, I2C2_SDA_PIN)
#define SCL2_READ()      GPIO_ReadInputDataBit(I2C2_SCL_PORT, I2C2_SCL_PIN)

/* I2C delay function */
static void I2C_delay(void)
{
    uint8_t i = 10;
    while(i--);
}
#endif

/* ==================== Software I2C Implementation ==================== */
#ifndef DRV_24C02_USE_HW_I2C

/**
 * @brief Initialize software I2C GPIO pins
 */
static void SoftI2C_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* Enable GPIOB clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    /* Configure SCL (PB10) and SDA (PB11) as open-drain output */
    GPIO_InitStructure.GPIO_Pin   = I2C2_SCL_PIN | I2C2_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C2_SCL_PORT, &GPIO_InitStructure);
    
    /* Set SCL and SDA to high */
    SCL2_H();
    SDA2_H();
}

/**
 * @brief Generate I2C start condition
 * @return true if successful, false if bus is busy
 */
static bool SoftI2C_Start(void)
{
    SDA2_H();
    SCL2_H();
    I2C_delay();
    if(!SDA2_READ()) return false;  /* SDA line is low, bus is busy */
    SDA2_L();
    I2C_delay();
    if(SDA2_READ()) return false;    /* SDA line is high, bus error */
    SCL2_L();
    I2C_delay();
    return true;
}

/**
 * @brief Generate I2C stop condition
 */
static void SoftI2C_Stop(void)
{
    SCL2_L();
    I2C_delay();
    SDA2_L();
    I2C_delay();
    SCL2_H();
    I2C_delay();
    SDA2_H();
    I2C_delay();
}

/**
 * @brief Wait for I2C ACK
 * @return true if ACK received, false if NACK
 */
static bool SoftI2C_WaitAck(void)
{
    SCL2_L();
    I2C_delay();
    SDA2_H();
    I2C_delay();
    SCL2_H();
    I2C_delay();
    if(SDA2_READ())
    {
        SCL2_L();
        return false;  /* NACK */
    }
    SCL2_L();
    return true;  /* ACK */
}

/**
 * @brief Send ACK
 */
static void SoftI2C_Ack(void)
{
    SCL2_L();
    I2C_delay();
    SDA2_L();
    I2C_delay();
    SCL2_H();
    I2C_delay();
    SCL2_L();
    I2C_delay();
}

/**
 * @brief Send NACK
 */
static void SoftI2C_NoAck(void)
{
    SCL2_L();
    I2C_delay();
    SDA2_H();
    I2C_delay();
    SCL2_H();
    I2C_delay();
    SCL2_L();
    I2C_delay();
}

/**
 * @brief Send one byte via I2C
 * @param SendByte: Byte to send
 */
static void SoftI2C_SendByte(uint8_t SendByte)
{
    uint8_t i = 8;
    while(i--)
    {
        SCL2_L();
        I2C_delay();
        if(SendByte & 0x80)
            SDA2_H();
        else
            SDA2_L();
        SendByte <<= 1;
        I2C_delay();
        SCL2_H();
        I2C_delay();
    }
    SCL2_L();
}

/**
 * @brief Receive one byte via I2C
 * @return Received byte
 */
static uint8_t SoftI2C_ReceiveByte(void)
{
    uint8_t i = 8;
    uint8_t ReceiveByte = 0;

    SDA2_H();
    while(i--)
    {
        ReceiveByte <<= 1;
        SCL2_L();
        I2C_delay();
        SCL2_H();
        I2C_delay();
        if(SDA2_READ())
        {
            ReceiveByte |= 0x01;
        }
    }
    SCL2_L();
    return ReceiveByte;
}

/**
 * @brief Read data using software I2C
 * @param pBuffer: Buffer to store read data
 * @param length: Number of bytes to read
 * @param ReadAddress: Starting address
 * @param DeviceAddress: Device address
 * @return true if successful
 */
static bool SoftI2C_ReadByte(uint8_t *pBuffer, uint16_t length, uint16_t ReadAddress, uint8_t DeviceAddress)
{
    if(!SoftI2C_Start()) return false;
    
    /* Send device address + high address bits */
    SoftI2C_SendByte((((ReadAddress & 0x0700) >> 7) | DeviceAddress) & 0xFFFE);
    if(!SoftI2C_WaitAck())
    {
        SoftI2C_Stop();
        return false;
    }
    
    /* Send low address */
    SoftI2C_SendByte((uint8_t)(ReadAddress & 0x00FF));
    SoftI2C_WaitAck();
    
    /* Restart for read */
    SoftI2C_Start();
    SoftI2C_SendByte(((ReadAddress & 0x0700) >> 7) | DeviceAddress | 0x0001);
    SoftI2C_WaitAck();
    
    /* Read data */
    while(length)
    {
        *pBuffer = SoftI2C_ReceiveByte();
        if(length == 1)
            SoftI2C_NoAck();
        else
            SoftI2C_Ack();
        pBuffer++;
        length--;
    }
    SoftI2C_Stop();
    return true;
}

/**
 * @brief Write one byte using software I2C
 * @param SendByte: Byte to write
 * @param WriteAddress: Address to write
 * @param DeviceAddress: Device address
 * @return true if successful
 */
static bool SoftI2C_WriteByte(uint8_t SendByte, uint16_t WriteAddress, uint8_t DeviceAddress)
{
    if(!SoftI2C_Start()) return false;
    
    /* Send device address + high address bits */
    SoftI2C_SendByte((((WriteAddress & 0x0700) >> 7) | DeviceAddress) & 0xFFFE);
    if(!SoftI2C_WaitAck())
    {
        SoftI2C_Stop();
        return false;
    }
    
    /* Send low address */
    SoftI2C_SendByte((uint8_t)(WriteAddress & 0x00FF));
    SoftI2C_WaitAck();
    
    /* Send data */
    SoftI2C_SendByte(SendByte);
    SoftI2C_WaitAck();
    SoftI2C_Stop();
    
    /* Wait for EEPROM write completion (typically 5-10ms) */
    BSP_Delay_ms(10);
    
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
    BSP_Delay_ms(10);
    
    return true;
}

#endif /* DRV_24C02_USE_HW_I2C */

/* ==================== Public API Implementation ==================== */

void Drv_24C02_Init(void)
{
#ifndef DRV_24C02_USE_HW_I2C
    SoftI2C_Init();
#endif
    /* For hardware I2C, initialization should be done by BSP_I2C1_Init() or BSP_I2C2_Init() */
}

bool Drv_24C02_Read(uint8_t *pBuffer, uint16_t length, uint16_t ReadAddress)
{
    if(pBuffer == NULL || length == 0) return false;
    if(ReadAddress > 255) return false;  /* AT24C02 has 256 bytes */
    
#ifdef DRV_24C02_USE_HW_I2C
    return HwI2C_ReadByte(pBuffer, length, ReadAddress, DRV_24C02_DEV_ADDR_7BIT);
#else
    return SoftI2C_ReadByte(pBuffer, length, ReadAddress, DRV_24C02_DEV_ADDR_8BIT);
#endif
}

bool Drv_24C02_WriteByte(uint8_t SendByte, uint16_t WriteAddress)
{
    if(WriteAddress > 255) return false;  /* AT24C02 has 256 bytes */
    
#ifdef DRV_24C02_USE_HW_I2C
    return HwI2C_WriteByte(SendByte, WriteAddress, DRV_24C02_DEV_ADDR_7BIT);
#else
    return SoftI2C_WriteByte(SendByte, WriteAddress, DRV_24C02_DEV_ADDR_8BIT);
#endif
}

bool Drv_24C02_Write(uint8_t *pBuffer, uint16_t length, uint16_t WriteAddress)
{
    uint16_t i;
    
    if(pBuffer == NULL || length == 0) return false;
    if(WriteAddress > 255) return false;  /* AT24C02 has 256 bytes */
    if((WriteAddress + length) > 256) return false;  /* Address overflow */
    
    /* Write byte by byte */
    for(i = 0; i < length; i++)
    {
        if(!Drv_24C02_WriteByte(pBuffer[i], WriteAddress + i))
        {
            return false;
        }
    }
    
    return true;
}

/**************************End of file********************************/
