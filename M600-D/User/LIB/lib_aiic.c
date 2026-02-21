/**
* Copyright (c) 2023, AstroCeta, Inc. All rights reserved.
* \file lib_aiic.h
* \brief Implementation of a ring buffer for efficient data handling.
* \date 2025-07-30
* \author AstroCeta, Inc.
**/
#include "lib_aiic.h"


uint8_t Drv_PIN_CTRL(iic_Node *iic,uint8_t pin,uint8_t rw,uint8_t state)
{
	uint8_t lbret = 0;
	if(pin == I2C_SCL)
	{
		if(rw == I2C_PINWRITE)
		{
			HAL_GPIO_WritePin(iic->SCLPort, iic->SCL_Pin, (GPIO_PinState)state);
			lbret = 1;
		}
	}
	else if(pin == I2C_SDA)
	{
		if(rw == I2C_PINWRITE)
		{
			HAL_GPIO_WritePin(iic->SDAPort, iic->SDA_Pin, (GPIO_PinState)state);
			lbret = 1;
		}
		else if(rw == I2C_PINREAD)
		{
			lbret = HAL_GPIO_ReadPin(iic->SDAPort, iic->SDA_Pin);
		}
	}
	return lbret;
}



/**
* @brief iic delay function
* @param 
**/
static void i2c_Delay(iic_Node *iic)
{
	for (uint8_t i = 0; i < iic->SpeedDelay; i++);
}

/**
* @brief 
**/
void i2c_Start(iic_Node *iic)
{
	Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINWRITE,I2C_HIGH);
	Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_HIGH);
	i2c_Delay(iic);
	Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINWRITE,I2C_LOW);
	i2c_Delay(iic);
	Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_LOW);
	i2c_Delay(iic);
}

void iO_Start(iic_Node *iic)
{
	Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINWRITE,I2C_LOW);
	Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_LOW);
}


void i2c_Stop(iic_Node *iic)
{
	Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINWRITE,I2C_LOW);
	Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_HIGH);
	i2c_Delay(iic);
	Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINWRITE,I2C_HIGH);
}


void i2c_SendByte(iic_Node *iic,uint8_t ucByte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		if (ucByte & 0x80)
		{
			Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINWRITE,I2C_HIGH);
		}else{
			Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINWRITE,I2C_LOW);
		}
		i2c_Delay(iic);
		Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_HIGH);
		i2c_Delay(iic);
		Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_LOW);
		if (i == 7)
		{
			Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINWRITE,I2C_HIGH); 
		}
		ucByte <<= 1;	
		i2c_Delay(iic);
	}
}

uint8_t i2c_WaitAck(iic_Node *iic)
{
	uint8_t re;
	Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINWRITE,I2C_HIGH);	
	i2c_Delay(iic);
	Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_HIGH);	
	i2c_Delay(iic);
	if(Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINREAD,I2C_LOW))
	{	
		re = 1;
	}
	else
	{
		re = 0;
	}
	Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_LOW);
	i2c_Delay(iic);
	return re;
}


uint8_t i2c_ReadByte(iic_Node *iic)
{
	uint8_t i;
	uint8_t value;
	value = 0;
	for (i = 0; i < 8; i++){
		value <<= 1;
		Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_HIGH);
		i2c_Delay(iic);
		
		if(Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINREAD,I2C_LOW)){
			value++;
		}
		Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_LOW);
		i2c_Delay(iic);
	}
	return value;
}


void i2c_Ack(iic_Node *iic)
{
	Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINWRITE,I2C_LOW);
	i2c_Delay(iic);
	Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_HIGH);
	i2c_Delay(iic);
	Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_LOW);
	i2c_Delay(iic);
	Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINWRITE,I2C_HIGH);
}


void i2c_NAck(iic_Node *iic)
{
	Drv_PIN_CTRL(iic,I2C_SDA,I2C_PINWRITE,I2C_HIGH);
	i2c_Delay(iic);
	Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_HIGH);
	i2c_Delay(iic);
	Drv_PIN_CTRL(iic,I2C_SCL,I2C_PINWRITE,I2C_LOW);
	i2c_Delay(iic);
}
 

uint8_t i2c_CheckDevice(iic_Node *iic,uint8_t _Address)
{
	uint8_t ucAck = 1;
	i2c_Start(iic);		
	i2c_SendByte(iic,_Address | I2C_WR);
	ucAck = i2c_WaitAck(iic);
 
	i2c_Stop(iic);		
 
	return ucAck;
}


uint8_t iic_Device_CheckOK(iic_Node *iic,uint8_t addr)
{
	if (i2c_CheckDevice(iic,addr) == 0){
		return 1;
	}else{
		i2c_Stop(iic);
		return 0;
	}
}

void iic_Write_Byte(iic_Node *iic,uint8_t addr,uint8_t regster_addr,uint8_t value)
{
	i2c_Start(iic);
	i2c_SendByte(iic,addr | I2C_WR);
	i2c_WaitAck(iic);
	i2c_SendByte(iic,regster_addr);
	i2c_WaitAck(iic);
	i2c_SendByte(iic,value);
	i2c_WaitAck(iic);
	i2c_Stop(iic);
}

uint8_t Drv_IIC_WriteByte(iic_Node *iic,uint8_t addr,uint8_t regster_addr,uint8_t *pBuffer,uint8_t Length)
{
	if(Length > 8)
	{
	}
	i2c_Start(iic);
	addr = addr << 1;
	i2c_SendByte(iic,addr | I2C_WR);
	if (i2c_WaitAck(iic) != 0)
	{
		goto cmd_fail;	
	}
	i2c_SendByte(iic,regster_addr);
	if (i2c_WaitAck(iic) != 0)
	{
		goto cmd_fail;	
	}
	
	for(uint8_t i=0; i<Length; i++)
	{
		i2c_SendByte(iic,pBuffer[i]);
		if (i2c_WaitAck(iic) != 0)
		{
			goto cmd_fail;	
		}
	}
	i2c_Stop(iic);
cmd_fail:
	i2c_Stop(iic);
	return 0;
}

uint8_t Drv_IIC_WriteReg(iic_Node *iic,uint8_t addr,uint8_t regster_addr,uint8_t Data)
{
	return Drv_IIC_WriteByte(iic,addr,regster_addr,&Data,1);
}

uint8_t Drv_IIC_ReadByte( iic_Node *iic,uint8_t addr,uint16_t _usAddress,uint8_t *buff,uint8_t _usSize)
{
	uint8_t i = 0;
 
	i2c_Start(iic);	
	addr = addr << 1;
	i2c_SendByte(iic,addr | I2C_WR);
	if (i2c_WaitAck(iic) != 0)
	{
		goto cmd_fail;	
	}
	i2c_SendByte(iic,(uint8_t)_usAddress);
	if (i2c_WaitAck(iic) != 0)
	{
		goto cmd_fail;
	}
	i2c_Delay(iic);
	i2c_Delay(iic);
	i2c_Start(iic);
	i2c_SendByte(iic,addr | I2C_RD);

	if (i2c_WaitAck(iic) != 0)
	{
		goto cmd_fail;
	}
	for (i = 0; i < _usSize; i++)
	{
		buff[i] = i2c_ReadByte(iic);
 
		if (i != _usSize - 1)
		{
			i2c_Ack(iic);
		}
		else
		{
			i2c_NAck(iic);
		}
	}
	i2c_Stop(iic);
	return 1;
cmd_fail:
	i2c_Stop(iic);
	return 0;
}
 


uint16_t Drv_IIC_ReadTwoByte( iic_Node *iic,uint8_t addr,uint16_t _usAddress)
{
	uint16_t temp = 0;
	i2c_Start(iic);	
  addr = addr << 1;
	i2c_SendByte(iic,addr | I2C_WR);
	if (i2c_WaitAck(iic) != 0)
	{
		goto cmd_fail;
	}
	i2c_SendByte(iic,(uint8_t)_usAddress);
	if (i2c_WaitAck(iic) != 0)
	{
		goto cmd_fail;
	}
	i2c_Start(iic);	
	i2c_SendByte(iic,addr | I2C_RD);
	if (i2c_WaitAck(iic) != 0)
	{
		goto cmd_fail;
	}
	temp =(i2c_ReadByte(iic) << 8);
	i2c_Ack(iic);
	temp |=(i2c_ReadByte(iic) & 0xff);
	i2c_NAck(iic);
	i2c_Stop(iic);
	return temp;
cmd_fail: 
	i2c_Stop(iic);
	return 0;
}
 


uint8_t Drv_IIC_Read_Reg( iic_Node *iic,uint8_t addr,uint16_t _usAddress)
{
	uint8_t temp = 0;
	i2c_Start(iic);	
	addr = addr << 1;
	i2c_SendByte(iic,addr | I2C_WR);
	if (i2c_WaitAck(iic) != 0)
	{
		goto cmd_fail;
	}
	i2c_SendByte(iic,(uint8_t)_usAddress);
	if (i2c_WaitAck(iic) != 0)
	{
		goto cmd_fail;
	}
	i2c_Start(iic);
	i2c_SendByte(iic,addr | I2C_RD);
	if (i2c_WaitAck(iic) != 0)
	{
		goto cmd_fail;
	}
	temp =i2c_ReadByte(iic)&0xff;
	i2c_NAck(iic);
	i2c_Stop(iic);
	return temp;
cmd_fail:
	i2c_Stop(iic);
	return 0;
}

/**************************End of file********************************/


