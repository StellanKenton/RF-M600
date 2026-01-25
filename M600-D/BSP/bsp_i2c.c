/************************************************************************************
 * @file     : bsp_i2c.c
 * @brief    : M600 I2C1/I2C2 init - PB6/7, PB10/11, 100kHz, ported from M600 HAL
 ***********************************************************************************/
#include "bsp_i2c.h"

#define BSP_I2C_TIMEOUT  0xFFFFu

static int i2c1_wait_event(I2C_TypeDef *I2Cx, uint32_t event);
static int i2c1_master_tx(I2C_TypeDef *I2Cx, uint8_t devAddr, const uint8_t *buf, uint16_t len);
static int i2c1_master_rx(I2C_TypeDef *I2Cx, uint8_t devAddr, uint8_t *buf, uint16_t len);

void BSP_I2C1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    I2C_InitStructure.I2C_Mode             = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle        = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1      = 0;
    I2C_InitStructure.I2C_Ack              = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed       = 100000;
    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);
}

void BSP_I2C2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    I2C_InitStructure.I2C_Mode             = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle        = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1      = 0;
    I2C_InitStructure.I2C_Ack              = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed       = 100000;
    I2C_Init(I2C2, &I2C_InitStructure);
    I2C_Cmd(I2C2, ENABLE);
}

static int i2c1_wait_event(I2C_TypeDef *I2Cx, uint32_t event)
{
    uint32_t t = BSP_I2C_TIMEOUT;
    while (I2C_CheckEvent(I2Cx, event) != SUCCESS) {
        if (--t == 0)
            return -1;
    }
    return 0;
}

static int i2c1_master_tx(I2C_TypeDef *I2Cx, uint8_t devAddr, const uint8_t *buf, uint16_t len)
{
    uint16_t i;
    if (!buf && len)
        return -1;
    I2C_GenerateSTART(I2Cx, ENABLE);
    if (i2c1_wait_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT) != 0)
        return -1;
    I2C_Send7bitAddress(I2Cx, devAddr, I2C_Direction_Transmitter);
    if (i2c1_wait_event(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != 0) {
        I2C_GenerateSTOP(I2Cx, ENABLE);
        return -1;
    }
    for (i = 0; i < len; i++) {
        I2C_SendData(I2Cx, buf[i]);
        if (i2c1_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED) != 0) {
            I2C_GenerateSTOP(I2Cx, ENABLE);
            return -1;
        }
    }
    I2C_GenerateSTOP(I2Cx, ENABLE);
    return 0;
}

static int i2c1_master_rx(I2C_TypeDef *I2Cx, uint8_t devAddr, uint8_t *buf, uint16_t len)
{
    uint16_t i;
    if (!buf && len)
        return -1;
    if (len == 0)
        return 0;
    I2C_GenerateSTART(I2Cx, ENABLE);
    if (i2c1_wait_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT) != 0)
        return -1;
    I2C_Send7bitAddress(I2Cx, devAddr, I2C_Direction_Receiver);
    if (i2c1_wait_event(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) != 0) {
        I2C_GenerateSTOP(I2Cx, ENABLE);
        return -1;
    }
    if (len == 1) {
        I2C_AcknowledgeConfig(I2Cx, DISABLE);
        I2C_GenerateSTOP(I2Cx, ENABLE);
        if (i2c1_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED) != 0)
            return -1;
        buf[0] = I2C_ReceiveData(I2Cx);
        return 0;
    }
    for (i = 0; i < len; i++) {
        if (i == len - 1) {
            I2C_AcknowledgeConfig(I2Cx, DISABLE);
            I2C_GenerateSTOP(I2Cx, ENABLE);
        }
        if (i2c1_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED) != 0)
            return -1;
        buf[i] = I2C_ReceiveData(I2Cx);
    }
    return 0;
}

int BSP_I2C1_Transmit(uint8_t devAddr, const uint8_t *buf, uint16_t len)
{
    return i2c1_master_tx(I2C1, devAddr, buf, len);
}

int BSP_I2C1_Receive(uint8_t devAddr, uint8_t *buf, uint16_t len)
{
    return i2c1_master_rx(I2C1, devAddr, buf, len);
}

int BSP_I2C2_Transmit(uint8_t devAddr, const uint8_t *buf, uint16_t len)
{
    return i2c1_master_tx(I2C2, devAddr, buf, len);
}

int BSP_I2C2_Receive(uint8_t devAddr, uint8_t *buf, uint16_t len)
{
    return i2c1_master_rx(I2C2, devAddr, buf, len);
}
