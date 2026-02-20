/******************************************Copyright (C)****************************************
*
* -------------------------------------Company Information-------------------------------------
* Company    : Nanjing Medlander Medical Technology Co. Ltd..
*
* URL		 : http://www.medlander.com/
*
* -------------------------------------C File Descriptions-------------------------------------
* File    	 : user_tim.c
* Version    : Ver 1.0
* Department : Medlander@Hardware
* Author	 : Arthur
* Data		 : 2021.12.07
* Hardware   : GD32F103CBT6.ARM.Cortex M3
* Description: TIM Module Function and varibales define
*
***********************************************************************************************/

/****************************������ͷ�ļ�**************************/
#include "bsp_24C02.h"
#include "bsp_i2c.h"
//#include "bsp_delay.h"
#include "peripheral.h"
#include "bsp_SI5351.h"
/*******************************************************************************
* Function Name  : I2C_Start
* Description    : None
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
u8  UART_freq[8] ={0x5A, 0xA5, 0x05, 0x82, 0x20, 0x20, 0x00, 0x00};

static FunctionalState I2C_Start(void)
{
    SDA2_H;
    SCL2_H;
    I2C_delay();
    if(!SDA2_READ)return DISABLE;	/* SDA��Ϊ�͵�ƽ������æ,�˳� */
    SDA2_L;
    I2C_delay();
    if(SDA2_READ) return DISABLE;	/* SDA��Ϊ�ߵ�ƽ�����߳���,�˳� */
    SDA2_L;
    I2C_delay();
    return ENABLE;
}
/******************************************************************************************
* Function Name  : I2C_WaitAck
* Description    : None
* Input          : None
* Output         : None
* Return         : ����Ϊ:=1��ACK,=0��ACK
* Attention		 : None
*******************************************************************************************/
static FunctionalState I2C_WaitAck(void)
{
    SCL2_L;
    I2C_delay();
    SDA2_H;
    I2C_delay();
    SCL2_H;
    I2C_delay();
    if(SDA2_READ)
    {
        SCL2_L;
        return DISABLE;
    }
    SCL2_L;
    return ENABLE;
}
/******************************************************************************************
* Function Name  : I2C_SendByte
* Description    : ���ݴӸ�λ����λ
* Input          : - SendByte: ���͵�����
* Output         : None
* Return         : None
* Attention		 : None
******************************************************************************************/
static void I2C_SendByte(uint8_t SendByte)
{
    uint8_t i = 8;
    while(i--)
    {
        SCL2_L;
        I2C_delay();
        if(SendByte & 0x80)
            SDA2_H;
        else
            SDA2_L;
        SendByte <<= 1;
        I2C_delay();
        SCL2_H;
        I2C_delay();
    }
    SCL2_L;
}
/*******************************************************************************************
* Function Name  : I2C_Stop
* Description    : None
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
********************************************************************************************/
static void I2C_Stop(void)
{
    SCL2_L;
    I2C_delay();
    SDA2_L;
    I2C_delay();
    SCL2_H;
    I2C_delay();
    SDA2_H;
    I2C_delay();
}
/********************************************************************************************
* Function Name  : I2C_ReceiveByte
* Description    : ���ݴӸ�λ����λ
* Input          : None
* Output         : None
* Return         : I2C���߷��ص�����
* Attention		 : None
*********************************************************************************************/
static uint8_t I2C_ReceiveByte(void)
{
    uint8_t i = 8;
    uint8_t ReceiveByte = 0;

    SDA2_H;
    while(i--)
    {
        ReceiveByte <<= 1;
        SCL2_L;
        I2C_delay();
        SCL2_H;
        I2C_delay();
        if(SDA2_READ)
        {
            ReceiveByte |= 0x01;
        }
    }
    SCL2_L;
    return ReceiveByte;
}
/********************************************************************************************
* Function Name  : I2C_NoAck
* Description    : None
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*********************************************************************************************/
static void I2C_NoAck(void)
{
    SCL2_L;
    I2C_delay();
    SDA2_H;
    I2C_delay();
    SCL2_H;
    I2C_delay();
    SCL2_L;
    I2C_delay();
}
/*******************************************************************************************
* Function Name  : I2C_Ack
* Description    : None
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
********************************************************************************************/
static void I2C_Ack(void)
{
    SCL2_L;
    I2C_delay();
    SDA2_L;
    I2C_delay();
    SCL2_H;
    I2C_delay();
    SCL2_L;
    I2C_delay();
}
/*********************************************************************************************
* Function Name  : I2C_ReadByte
* Description    : ��ȡһ������
* Input          : - pBuffer: ��Ŷ�������
*           	   - length: ����������
*                  - ReadAddress: ��������ַ
*                  - DeviceAddress: ��������(24c16��SD2403)
* Output         : None
* Return         : ����Ϊ:=1�ɹ�����,=0ʧ��
* Attention		 : None
**********************************************************************************************/
FunctionalState I2C_ReadByte(uint8_t* pBuffer,   uint16_t length,   uint16_t ReadAddress,  uint8_t DeviceAddress)
{
    if(!I2C_Start())return DISABLE;
    I2C_SendByte( (((ReadAddress & 0x0700) >> 7) | DeviceAddress) & 0xFFFE); /* ���ø���ʼ��ַ+������ַ */
    if(!I2C_WaitAck()) {
        I2C_Stop();
        return DISABLE;
    }
    I2C_SendByte((uint8_t)(ReadAddress & 0x00FF));   /* ���õ���ʼ��ַ */
    I2C_WaitAck();
    I2C_Start();
    I2C_SendByte(((ReadAddress & 0x0700) >> 7) | DeviceAddress | 0x0001);
    I2C_WaitAck();
    while(length)
    {
        *pBuffer = I2C_ReceiveByte();
        if(length == 1)I2C_NoAck();
        else I2C_Ack();
        pBuffer++;
        length--;
    }
    I2C_Stop();
    return ENABLE;
}
/*********************************************************************************************
* Function Name  : I2C_WriteByte
* Description    : дһ�ֽ�����
* Input          : - SendByte: ��д������
*           	   - WriteAddress: ��д���ַ
*                  - DeviceAddress: ��������(24c16��SD2403)
* Output         : None
* Return         : ����Ϊ:=1�ɹ�д��,=0ʧ��
* Attention		 : None
********************************************************************************************/
FunctionalState I2C_WriteByte(uint8_t SendByte, uint16_t WriteAddress, uint8_t DeviceAddress)
{
    if(!I2C_Start())return DISABLE;
    I2C_SendByte( (((WriteAddress & 0x0700) >> 7) | DeviceAddress) & 0xFFFE); /*���ø���ʼ��ַ+������ַ */
    if(!I2C_WaitAck()) {
        I2C_Stop();
        return DISABLE;
    }
    I2C_SendByte((uint8_t)(WriteAddress & 0x00FF));   /* ���õ���ʼ��ַ */
    I2C_WaitAck();
    I2C_SendByte(SendByte);
    I2C_WaitAck();
    I2C_Stop();
    /* ע�⣺��Ϊ����Ҫ�ȴ�EEPROMд�꣬���Բ��ò�ѯ����ʱ��ʽ(10ms)	*/
    /* Systick_Delay_1ms(10); */
    delay_ms(10);

    return ENABLE;
}
EEPROM_TypeDef St_Eeprom;
static uint8_t Temp_LevelProject = 0;
static uint16_t Temp_TimeProject = 0;
/*************************************************************************************************
* �� �� ����void Eeprom_Para_Init(void)
* ����������eeprom������ʼ��
* �����������
* �����������
* �������ͣ� void
* ��    �ߣ�HardWare_Department@Medlander
* ��    �ڣ�2021.07.13
*************************************************************************************************/
void Eeprom_Para_Init(void)
{
    /*eeprom������ʼ��*/
    memset(&St_Eeprom, 0, sizeof(St_Eeprom));
}
/*************************************************************************************************
* �� �� ����void Eeprom_ReadeData_Handle(const uint8_t blockInfo,uint8_t *rx_buf)
* ������������eeprom�ж�������
* �����������
* �����������
* �������ͣ� void
* ��    �ߣ�HardWare_Department@Medlander
* ��    �ڣ�2021.07.13
*************************************************************************************************/
void Eeprom_ReadeData_Handle(__IO uint8_t blockInfo, uint8_t *rx_buf)
{
    I2C_ReadByte( rx_buf, 8, blockInfo * 8, AT24C02_ADR);
}
/*************************************************************************************************
* �� �� ����void Eeprom_WriteData_Handle(const uint8_t blockInfo,uint8_t *tx_buf)
* ����������д����
* �����������
* �����������
* �������ͣ� void
* ��    �ߣ�HardWare_Department@Medlander
* ��    �ڣ�2021.07.13
*************************************************************************************************/
void Eeprom_WriteData_Handle(const uint8_t blockInfo,uint8_t *tx_buf)
{
    uint8_t i;

    for(i = 0; i < 8; i++)

    {
        I2C_WriteByte(tx_buf[i], blockInfo * 8 + i, AT24C02_ADR);
    }
}
/***********************************************************************************************
* �� �� ����static uint8_t Eeprom_DataHandle(__IO BLOCK_ENUM Block,uint8_t *Tx,__IO uint8_t Val,__IO uint8_t ReadBack_State)
* ����������eeprom�洢���ݴ���
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�HardWare_Department@Medlander
* ��    �ڣ�2020.08.04
***********************************************************************************************/
static uint8_t Eeprom_DataHandle(__IO BLOCK_ENUM Block,uint8_t *Tx,__IO uint8_t Val,__IO uint8_t ReadBack_State)
{
    uint8_t Ret = 0;

    if(Val == 0)
    {
        Ret = Val;
    }
    else
    {
        if(ReadBack_State == 0x01)
        {
            /*St_Eeprom.E_SetState = (SET_ENUM)Val;*/
            switch (Block)
            {
            case E_MODEL:
            {
                if(Val == 0x02)
                {
                    Ret = 0x02;
                    St_Eeprom.IC_Treat_Time = 0;
                    St_Eeprom.IC_Treat_Power = 0;
                    St_Eeprom.E_Model = (MODEL_ENUM)Tx[0];
                }
                else if(Val == 0x01)
                {
                    Ret = 0x03;
                }

                break;
            }

            case E_PROBE_OVERTEMP_THROLD:
            {
                if(Val == 0x02)
                {
                    Ret = 0x04;
                    St_Eeprom.Temperature1 = Tx[0];
                    St_Eeprom.Temperature2 = Tx[1];
                }
                else if(Val == 0x01)
                {
                    Ret = 0x05;
                }

                break;
            }

            case E_VERIFY_EEPROM:
            {
                if(Val == 0x02)
                {
                    Ret = 0x06;
                }
                else if(Val == 0x01)
                {
                    Ret = 0x07;
                }

                break;
            }

            default:
                break;
            }
        }
        else
        {
            Ret = 0x01;
        }
    }

    return Ret;
}
/*************************************************************************************************
* �� �� ����uint8_t EepromWrite_NeedReadBack_Handle(__IO uint8_t Block,uint8_t *tx_buf,__IO uint8_t State,__IO uint8_t TypeState)
* ����������������д��eeprom
* �����������
* �����������
* �������ͣ� void
* ��    �ߣ�HardWare_Department@Medlander
* ��    �ڣ�2021.07.13
*************************************************************************************************/
uint8_t EepromWrite_NeedReadBack_Handle(__IO BLOCK_ENUM Block,uint8_t *tx_buf,__IO uint8_t State,__IO uint8_t TypeState)
{
    static uint8_t Step = 0,Cnt = 0,FirstFlag = 0x01,TempData[32] = {0},Num = 0,ClearSelf_Flag = 0;
    uint8_t Ret = 0,Rx[8] = {0},Tx[4] = {0},i,ErrCnt = 0;

    switch(Step)
    {
    case 0x00:
    {
        if(FirstFlag == 0x01)
        {
            St_Eeprom.E_EepromState = E_EEPROM_BUSY;
            Num = 8;
            if(Block == E_MODEL)
            {
                Num = 16;  /*���ӹ����ɣ��Զ��壩һ������*/
            }
            else if(Block == E_ZHUANGJIZHUCE)
            {
                /*װ��ע��BLOCK���豸���BLOCK����4������һ������*/
                ClearSelf_Flag = 0x01;
            }
            else if(Block == E_DEVIECE_SN)
            {
                Block = E_ZHUANGJIZHUCE;
                Num = 32;
                /*װ��ע��BLOCK���豸���BLOCK����4������һ������*/
            }

            memcpy(TempData,tx_buf,Num);
            for(i = 0; i < Num; i++)
            {
                I2C_WriteByte(TempData[i], Block * 8 + i, AT24C02_ADR);
            }

            FirstFlag = 0;
        }

        Cnt += 1;
        if(Cnt >= 2)
        {
            FirstFlag = 0x01;
            Cnt =0;
            if(State == 0x01)
            {
                Step = 0x02;   /*��ֵ����*/
            }
            else
            {
                if(TypeState != 0)
                {
                    Step = 0x00;
                    Ret = 0x02;
                    St_Eeprom.E_EepromState = E_EEPROM_IDLE;
                }
                else
                {
                    if(ClearSelf_Flag == 0x01)
                    {
                        Step = 0x01;  /*װ��ע������ӹ����ɣ��Զ��壩*/
                        ClearSelf_Flag = 0;
                    }
                    else
                    {
                        Step = 0x00;
                        Ret = 0x02;
                        St_Eeprom.E_EepromState = E_EEPROM_IDLE;
                    }
                }
            }
        }

        break;
    }

    case 0X01:
    {
        if(FirstFlag == 0x01)
        {
            for(i = 0; i < 8; i++)
            {
                I2C_WriteByte(Tx[i], E_SELF_PROJECT * 8 + i, AT24C02_ADR);    // ����ӹ����ɣ��Զ��壩
            }
            FirstFlag = 0;
        }

        Cnt += 1;
        if(Cnt >= 2)
        {
            Step = 0x00;
            FirstFlag = 0x01;
            Cnt = 0;
            Ret = 0x02;
            St_Eeprom.E_EepromState = E_EEPROM_IDLE;
        }

        break;
    }

    case 0x02:
    {
        if(FirstFlag == 0x01)
        {
            FirstFlag = 0;
            I2C_ReadByte( Rx, Num, Block * Num, AT24C02_ADR);     // ����������Ҫ�ȶ�����д��ֵ������ֵ��˰Ѵ˺����ڶ�����θ�ΪNum
        }

        for(i = 0; i< Num; i++)
        {
            if(TempData[i] != Rx[i])
            {
                ErrCnt += 1;
            }
        }

        FirstFlag = 0x01;
        Step =0;
        St_Eeprom.E_EepromState = E_EEPROM_IDLE;
        if(ErrCnt != 0)
        {
            Ret = 0x01;   /*д��ʧ��*/
        }
        else
        {
            Ret = 0x02;   /*д��ɹ�*/
        }

        break;
    }

    default:
        break;
    }
    Ret = Eeprom_DataHandle(Block,tx_buf,Ret,State);

    return Ret;
}
/***********************************************************************************************
* �� �� ����static MODEL_ENUM Get_ModelType(void)
* ������������ȡ�豸�ͺ�����   mld_esu 001A  �C�Cmld_esu 002A    �C�Cmld_esu 003A     mld_esu 004A�C
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/
static MODEL_ENUM Get_ModelType(void)
{
    uint8_t Temp[8] = {0};
    MODEL_ENUM State = E_MLD_ESU_NONE;

    Eeprom_ReadeData_Handle(E_MODEL,Temp);
    if((Temp[0] < 0x01) || (Temp[0] > 0x04))
    {
        Temp[0] = 0;
    }
    State = (MODEL_ENUM)Temp[0];

    return State;
}
/***********************************************************************************************
* �� �� ����static void Get_ProbeTemperature(void)
* ������������ȡ����̽ͷ�¶���ֵ
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/
static void Get_ProbeTemperature(void)
{
    uint8_t Temp[8] = {0};

    Eeprom_ReadeData_Handle(E_PROBE_OVERTEMP_THROLD,Temp);
    St_Eeprom.Temperature1 = Temp[0];
    if((St_Eeprom.Temperature1 < THRESHOLD_TEMP1_DEFAULT) || (St_Eeprom.Temperature1 == 0xFF))
    {
        St_Eeprom.Temperature1 = THRESHOLD_TEMP1_DEFAULT;
    }

    St_Eeprom.Temperature2 = Temp[1];
    if((St_Eeprom.Temperature2 < THRESHOLD_TEMP2_DEFAULT) || (St_Eeprom.Temperature2 == 0xFF))
    {
        St_Eeprom.Temperature2 = THRESHOLD_TEMP2_DEFAULT;
    }
}
/***********************************************************************************************
* �� �� ����static void Get_Diy_Project(void)
* ������������ȡ�ӹ������ǣ��Զ��壩��λ��ʱ��C
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/
static void Get_Diy_Project(void)
{
   uint8_t SelfProject[8] = {0};

    /***********�ӹ����ɵ�λ�Զ������ݻ�ȡ**********************************/
    Eeprom_ReadeData_Handle(E_SELF_PROJECT,SelfProject);
    St_Eeprom.IC_Treat_Power = SelfProject[0];   // һ����4����������6����������Ƕ�����������6ҲҪ����0
    if(St_Eeprom.IC_Treat_Power > 0x08)
    {
        St_Eeprom.IC_Treat_Power = 0;
    }
    St_Eeprom.IC_Treat_Time = (((uint16_t)SelfProject[1]) >> 8) + SelfProject[2];
    if(St_Eeprom.IC_Treat_Time > 0xA8C)
    {
        St_Eeprom.IC_Treat_Time = 0;
    }
}
/***********************************************************************************************
* �� �� ����static void Get_Zhuangjizhuce_Info(void)
* ������������ȡװ��ע�������Ϣ�C
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/
static uint8_t Get_Zhuangjizhuce_Info(void)
{
    uint8_t data[8] = {0},Ret = 0x01;

    Eeprom_ReadeData_Handle(E_ZHUANGJIZHUCE,data);
    if(data[1] >= 0x08 && data[1] <= 0x18)
    {
        St_Eeprom.SnLength = data[1];
    }
    else
    {
        St_Eeprom.SnLength = 0;
    }

    if(data[0] == 0x05)
    {
        Ret = 0x03;
        St_Eeprom.CMD.bit.Flag_StopDisplay = 1;
        St_Eeprom.CMD.bit.Flag_Verify_CalculateTime = 1;
    }
    return Ret;
}
/***********************************************************************************************
* �� �� ����static void Get_Sn_Info(void)
* ������������ȡSN������Ϣ�C
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/
static void Get_Sn_Info(void)
{
    if(St_Eeprom.SnLength > 0)
    {
        I2C_ReadByte( (uint8_t*)St_Eeprom.IC_Verify, St_Eeprom.SnLength, E_DEVIECE_SN * 8, AT24C02_ADR);
    }
}
/***********************************************************************************************
* �� �� ����void Get_DeviceInfo(void)
* ������������ȡ�豸��Ϣ
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/
uint8_t Get_DeviceInfo(void)
{
    uint8_t Ret1 = 0x01,Ret2 = 0;
//    uint8_t Tx[8] = {0};

    St_Eeprom.E_Model = Get_ModelType();
    if(St_Eeprom.E_Model == E_MLD_ESU_NONE)
    {
        Ret1 = 0x05;
    }
    delay_ms(100);
    Get_Diy_Project();
    delay_ms(100);
    Get_ProbeTemperature();
    delay_ms(100);
    Ret2 = Get_Zhuangjizhuce_Info();
    if(Ret2 == 0x03)
    {
        Ret1 = Ret2;
    }
    delay_ms(100);
    Get_Sn_Info();
//		Eeprom_WriteData_Handle(0,Tx);

    return Ret1;
}
/***********************************************************************************************
* �� �� ����void Refresh_Project(__IO uint8_t Level,__IO uint16_t Time,__IO uint8_t State)
* �������������ƹ�����ˢ�����Ʒ����ĵ�λ��ʱ��
* �����������λ��ʱ�䡢��ǰ����ģʽ���û�or���ԣ�����ǰ����ģʽ���û�or���ԣ���һ��ģʽ
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/
void Refresh_Project(__IO uint8_t Level,__IO uint16_t Time,__IO uint8_t State)
{
    if((St_Eeprom.E_Model < E_MLD_ESU_002A) || (State != 0x01))
    {
        return;
    }

    if(St_Eeprom.E_StoreState == E_STORE_DISABLE)
    {
        St_Eeprom.IC_Treat_Power = Level;
        St_Eeprom.IC_Treat_Time = Time;
    }
    else if(St_Eeprom.E_StoreState == E_STORE_ENABLE)
    {
        if(Temp_LevelProject != Level)
        {
            Temp_LevelProject = Level;
        }

        if(Temp_TimeProject != Time)
        {
            Temp_TimeProject = Time;
        }

        if((Temp_LevelProject != St_Eeprom.IC_Treat_Power) || (Temp_TimeProject != St_Eeprom.IC_Treat_Time))
        {
            St_Eeprom.IC_Treat_Power = Temp_LevelProject;
            St_Eeprom.IC_Treat_Time = Temp_TimeProject;
            if(St_Eeprom.E_EepromState == E_EEPROM_IDLE)
            {
                St_Eeprom.CMD.bit.Flag_UpDataEeprom = 1;
                St_Eeprom.Data_Write[0] = St_Eeprom.IC_Treat_Power;
                St_Eeprom.Data_Write[1] = (uint8_t)St_Eeprom.IC_Treat_Power;
                St_Eeprom.Data_Write[2] = St_Eeprom.IC_Treat_Power & 0xFF;
                St_Eeprom.Eeprom_Page_Write = E_SELF_PROJECT;
                St_Eeprom.E_ReadBack_State = E_READ_EE_BACK_NONE;
            }
        }
    }
}


/***********************************************************************************************
* �� �� ����void Get_DeviceInfo(void)
* ������������ʼ����ȡ�豸�Ĺ���Ƶ�� ���ϰ�
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/


void Get_Readfreq_data1(void)
{
    uint8_t SelfProject[2] = {0};

    /***********��ʼ����ȡƵ��**********************************/
    Eeprom_ReadeData_Handle(E_freq_data1_EEPROM,SelfProject);
  //  St_Eeprom.freq_data1 = (((uint16_t)SelfProject[0]) >> 8) + SelfProject[1];
		St_Eeprom.freq_data1 = (SelfProject[0] << 8 | SelfProject[1]);
    if(St_Eeprom.freq_data1 <= 0X578  && St_Eeprom.freq_data1 >= 0X2BC)
    {
      Si5351_Open_CLK1( St_Eeprom.freq_data1 );
			UART_freq[6] =  St_Eeprom.freq_data1 >> 8;;
			UART_freq[7] =  St_Eeprom.freq_data1 & 0XFF;
			Eeprom_WriteData_Handle(E_freq_data1_EEPROM , SelfProject);
			circ_send(UART_freq, sizeof(UART_freq));
    }else{
		  St_Eeprom.freq_data1 = 0x4B0;
			UART_freq[6] =  0x04;
			UART_freq[7] =  0xB0;
			circ_send(UART_freq, sizeof(UART_freq));
			SelfProject[0] =  0x04;
			SelfProject[1] =  0xB0;
			Eeprom_WriteData_Handle(E_freq_data1_EEPROM , SelfProject);
			Si5351_Open_CLK1( St_Eeprom.freq_data1 );
		}
}

//������Ƶ����Ĺ̶�Ƶ��500K
void Get_RFReadfreq_data1(void)
{

      Si5351_Open_CLK1( 500 );

}
/***********************************************************************************************
* �� �� ����void Get_DeviceInfo(void)
* ����������д��ָ���Ĺ���Ƶ�� ���ϰ�  RF
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/
   

void Get_Writefreq_data1(u16 Freq_vary)
{
	  uint8_t newfreq[2] = {0};
	  if(700<= Freq_vary && Freq_vary<= 1400)   //�ж�д���Ƶ���Ƿ��ڷ�Χ
	  {
		  Si5351_Open_CLK1( Freq_vary );
			newfreq[0] =  Freq_vary >> 8;
			newfreq[1] =  Freq_vary & 0XFF;
			Eeprom_WriteData_Handle(E_freq_data1_EEPROM , newfreq);
			Eeprom_ReadeData_Handle(E_freq_data1_EEPROM,newfreq);
			UART_freq[6] =  newfreq[0];
			UART_freq[7] =  newfreq[1];
			circ_send(UART_freq, sizeof(UART_freq));
			US_confi_reply[6] = 0X00 ;
			
		}else
		 {
			US_confi_reply[6] = 0X02 ;
		  UART_freq[6] = 0XFF;
			UART_freq[7] = 0XFF;
			circ_send(UART_freq, sizeof(UART_freq)); 	         //���޷��������
	   }

}

u8  UART_NTC[8] ={0x5A, 0xA5, 0x05, 0x82, 0x20, 0x30, 0x00, 0x00};  //
/***********************************************************************************************
* �� �� ����void Get_DeviceInfo(void)
* ������������ʼ����ȡ�豸���¶����� ���ϰ�
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/
void Get_ReadNTC_data1(void)
{
    uint8_t SelfProject[2] = {0};

    /***********��ʼ����ȡ�¶�����**********************************/
    Eeprom_ReadeData_Handle(E_maxNTC_data1_EEPROM,SelfProject);
    St_Eeprom.maxNTC_data1 = (SelfProject[0] << 8 | SelfProject[1]);
    if(St_Eeprom.maxNTC_data1 <= 480  && St_Eeprom.maxNTC_data1 >= 250)   
    {
      UART_NTC[6] = St_Eeprom.maxNTC_data1>> 8;
	    UART_NTC[7] =St_Eeprom.maxNTC_data1 & 0XFF;
	    circ_send(UART_NTC, sizeof(UART_NTC));  
    }else{
		  St_Eeprom.maxNTC_data1 = 0x190;   //Ĭ������40��
			UART_NTC[6] =  0x01;
			UART_NTC[7] =  0x90;
			circ_send(UART_NTC, sizeof(UART_NTC));
			SelfProject[0] =  0x01;
			SelfProject[1] =  0x90;
			Eeprom_WriteData_Handle(E_maxNTC_data1_EEPROM , SelfProject);
      Eeprom_ReadeData_Handle(E_maxNTC_data1_EEPROM,SelfProject);
      St_Eeprom.maxNTC_data1 = (SelfProject[0] << 8 | SelfProject[1]);
		}
}
/***********************************************************************************************
* �� �� ����void Get_DeviceInfo(void)
* ����������д��ָ�����¶����� ���ϰ�
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/
void Get_WriteNTC_data1(u16 NTC_vary)
{
	  uint8_t newNTC[2] = {0};
	  if(250<= NTC_vary && NTC_vary<= 480)   //�ж�д����¶������Ƿ��ڷ�Χ
	  {
			newNTC[0] =  NTC_vary >> 8;
			newNTC[1] =  NTC_vary & 0XFF;
			Eeprom_WriteData_Handle(E_maxNTC_data1_EEPROM , newNTC);
			UART_NTC[6] =  newNTC[0];
			UART_NTC[7] =  newNTC[1];
			circ_send(UART_NTC, sizeof(UART_NTC));
			US_confi_reply[8] = 0X00 ;
		//	Eeprom_ReadeData_Handle(E_maxNTC_data1_EEPROM,newNTC);
   //   St_Eeprom.maxNTC_data1 = (newNTC[0] << 8 | newNTC[1]);
    // Get_ReadNTC_data1();    //���¶�ȡ��ȷ��д�ɹ�
		}else
		 {
		  UART_NTC[6] = 0XFF;
			UART_NTC[7] = 0XFF;
			circ_send(UART_NTC, sizeof(UART_NTC)); 	 
      US_confi_reply[8] = 0X02 ;			 //���޷��������
	   }
}

u8  UART_U_DCDC[8] ={0x5A, 0xA5, 0x05, 0x82, 0x20, 0xA0, 0x00, 0x00};  //
/***********************************************************************************************
* �� �� ����void Get_DeviceInfo(void)
* ������������ʼ����ȡ�豸�ĵ�ѹ����ֵ ���ϰ�
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/

void RF_Get_U_DCDC_data1(void)
{
    uint8_t SelfProject[2] = {0};

    /***********��ʼ����ȡ��ѹֵ**********************************/
    Eeprom_ReadeData_Handle(E_set_U_data1_EEPROM,SelfProject);
    St_Eeprom.U_DCDC_data1 = (SelfProject[0] << 8 | SelfProject[1]);
    if(St_Eeprom.U_DCDC_data1 <= 2000  && St_Eeprom.U_DCDC_data1 >= 600)
    {
      UART_U_DCDC[6] = St_Eeprom.U_DCDC_data1>> 8;
	    UART_U_DCDC[7] =St_Eeprom.U_DCDC_data1 & 0XFF;
	    circ_send(UART_U_DCDC, sizeof(UART_U_DCDC));  
    }else{
		  St_Eeprom.U_DCDC_data1 = 0x5DC;   //Ĭ������40��
			UART_U_DCDC[6] =  0x05;
			UART_U_DCDC[7] =  0xDC;
			circ_send(UART_U_DCDC, sizeof(UART_U_DCDC));
			SelfProject[0] =  0x05;
			SelfProject[1] =  0xDC;
			Eeprom_WriteData_Handle(E_set_U_data1_EEPROM , SelfProject);

		}
}

void Get_U_DCDC_data1(void)
{
    uint8_t SelfProject[2] = {0};

    /***********��ʼ����ȡ��ѹֵ**********************************/
    Eeprom_ReadeData_Handle(E_set_U_data1_EEPROM,SelfProject);
    St_Eeprom.U_DCDC_data1 = (SelfProject[0] << 8 | SelfProject[1]);
    if(St_Eeprom.U_DCDC_data1 <= 2000  && St_Eeprom.U_DCDC_data1 >= 600)
    {
      UART_U_DCDC[6] = St_Eeprom.U_DCDC_data1>> 8;
	    UART_U_DCDC[7] =St_Eeprom.U_DCDC_data1 & 0XFF;
	    circ_send(UART_U_DCDC, sizeof(UART_U_DCDC));  
    }else{
		  St_Eeprom.U_DCDC_data1 = 0x5DC;   //Ĭ������40��
			UART_U_DCDC[6] =  0x05;
			UART_U_DCDC[7] =  0xDC;
			circ_send(UART_U_DCDC, sizeof(UART_U_DCDC));
			SelfProject[0] =  0x05;
			SelfProject[1] =  0xDC;
			Eeprom_WriteData_Handle(E_set_U_data1_EEPROM , SelfProject);

		}
}
/***********************************************************************************************
* �� �� ����void Get_DeviceInfo(void)
* ����������д��ָ���ĵ�ѹֵ ���ϰ�
* �����������
* �����������
* �������ͣ�void
* ��    �ߣ�Arthur
* ��    �ڣ�2021.12.04
***********************************************************************************************/
void RF_Get_WriteU_DCDC_data1(u16 U_DCDC_vary)
{
	  uint8_t new_DCDC_DATA[2] = {0};
		uint8_t SelfProject[2] = {0};
	  if(600<= U_DCDC_vary && U_DCDC_vary<= 2000)   //�ж�д��ĵ�ѹ�Ƿ��ڷ�Χ
	  {
			new_DCDC_DATA[0] =  U_DCDC_vary >> 8;
			new_DCDC_DATA[1] =  U_DCDC_vary & 0XFF;
			Eeprom_WriteData_Handle(E_set_U_data1_EEPROM , new_DCDC_DATA);
			Eeprom_ReadeData_Handle(E_set_U_data1_EEPROM,SelfProject);
      St_Eeprom.U_DCDC_data1 = (SelfProject[0] << 8 | SelfProject[1]);
			UART_U_DCDC[6] =  SelfProject[0];
			UART_U_DCDC[7] =  SelfProject[1];
			circ_send(UART_U_DCDC, sizeof(UART_U_DCDC));
			US_confi_reply[7] = 0X00 ;
    // Get_ReadNTC_data1();    //���¶�ȡ��ȷ��д�ɹ�
		}else
		 {
		  UART_U_DCDC[6] = 0XFF;
			UART_U_DCDC[7] = 0XFF;
			circ_send(UART_U_DCDC, sizeof(UART_U_DCDC)); 	         //���޷��������
			US_confi_reply[7] = 0X02 ;
	   }

}
//��Ƶ
void Get_RFReadNTC_data1(void)
{
    uint8_t SelfProject[2] = {0};

    /***********��ʼ����ȡ�¶�����**********************************/
    Eeprom_ReadeData_Handle(E_RFmaxNTC_data1_EEPROM,SelfProject);
    St_Eeprom.RFmaxNTC_data1 = (SelfProject[0] << 8 | SelfProject[1]);
    if(St_Eeprom.RFmaxNTC_data1 <= 480  && St_Eeprom.RFmaxNTC_data1 >= 250)   
    {
      UART_NTC[6] = St_Eeprom.RFmaxNTC_data1>> 8;
	    UART_NTC[7] =St_Eeprom.RFmaxNTC_data1 & 0XFF;
	    circ_send(UART_NTC, sizeof(UART_NTC));  
    }else{
		  St_Eeprom.RFmaxNTC_data1 = 0x190;   //Ĭ������40��
			UART_NTC[6] =  0x01;
			UART_NTC[7] =  0x90;
			circ_send(UART_NTC, sizeof(UART_NTC));
			SelfProject[0] =  0x01;
			SelfProject[1] =  0x90;
			Eeprom_WriteData_Handle(E_RFmaxNTC_data1_EEPROM , SelfProject);
      Eeprom_ReadeData_Handle(E_RFmaxNTC_data1_EEPROM,SelfProject);
      St_Eeprom.RFmaxNTC_data1 = (SelfProject[0] << 8 | SelfProject[1]);
		}
}


void Get_RFWriteNTC_data1(u16 NTC_vary)
{
	  uint8_t newNTC[2] = {0};
	  if(250<= NTC_vary && NTC_vary<= 480)   //�ж�д����¶������Ƿ��ڷ�Χ
	  {
			newNTC[0] =  NTC_vary >> 8;
			newNTC[1] =  NTC_vary & 0XFF;
			Eeprom_WriteData_Handle(E_RFmaxNTC_data1_EEPROM , newNTC);
			UART_NTC[6] =  newNTC[0];
			UART_NTC[7] =  newNTC[1];
			circ_send(UART_NTC, sizeof(UART_NTC));
			RF_confi_reply[6] = 0X00 ;
		//	Eeprom_ReadeData_Handle(E_maxNTC_data1_EEPROM,newNTC);
   //   St_Eeprom.maxNTC_data1 = (newNTC[0] << 8 | newNTC[1]);
    // Get_ReadNTC_data1();    //���¶�ȡ��ȷ��д�ɹ�
		}else
		 {
		  UART_NTC[6] = 0XFF;
			UART_NTC[7] = 0XFF;
			circ_send(UART_NTC, sizeof(UART_NTC)); 	 
      RF_confi_reply[6] = 0X02 ;			 //���޷��������
	   }
}

/*************************************************************************************************
*									END OF FILE
*************************************************************************************************/
