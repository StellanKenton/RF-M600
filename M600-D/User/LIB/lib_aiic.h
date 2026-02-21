/**
* Copyright (c) 2023, AstroCeta, Inc. All rights reserved.
* \file lib_aiic.h
* \brief Implementation of a ring buffer for efficient data handling.
* \date 2025-07-30
* \author AstroCeta, Inc.
**/
#ifndef LIB_AIIC_H
#define LIB_AIIC_H

#include <string.h>
#include <stdbool.h>
#include "stdint.h"

#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif

#include "gpio.h"

#define I2C_WR	0	// write
#define I2C_RD	1	// read
#define I2C_HIGH 1
#define I2C_LOW  0
#define I2C_PINREAD   1
#define I2C_PINWRITE  0
#define I2C_SDA   1
#define I2C_SCL  0

#define CycleCount 28

typedef struct {
	GPIO_TypeDef *SCLPort;
	uint16_t SCL_Pin;
	GPIO_TypeDef *SDAPort;
	uint16_t SDA_Pin;
	uint16_t SpeedDelay;
}iic_Node;


extern iic_Node sTCA9535Dev;


uint8_t Drv_IIC_WriteReg(iic_Node *iic,uint8_t addr,uint8_t regster_addr,uint8_t Data);
uint8_t Drv_IIC_Read_Reg( iic_Node *iic,uint8_t addr,uint16_t _usAddress);
uint16_t Drv_IIC_ReadTwoByte( iic_Node *iic,uint8_t addr,uint16_t _usAddress);
uint8_t Drv_IIC_WriteByte(iic_Node *iic,uint8_t addr,uint8_t regster_addr,uint8_t *pBuffer,uint8_t Length);
uint8_t Drv_IIC_ReadByte( iic_Node *iic,uint8_t addr,uint16_t _usAddress,uint8_t *buff,uint8_t _usSize);
void Drv_IIC_Write_Display(iic_Node *iic, uint8_t add, uint8_t value);


#ifdef __cplusplus
}
#endif
#endif  // LIB_AIIC_H
/**************************End of file********************************/

