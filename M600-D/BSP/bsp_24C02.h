/******************************************Copyright (C)****************************************
*
* -------------------------------------Company Information-------------------------------------
* Company    : Nanjing Medlander Medical Technology Co. Ltd..
* 
* URL		 : http://www.medlander.com/
* 
* -------------------------------------H File Descriptions-------------------------------------
* File    	 : user_stim.h
* Version    : Ver 1.0
* Department : Medlander@Hardware
* Author	 : Arthur
* Data		 : 2021.12.07
* Hardware   : GD32F103CBT6.ARM.Cortex M3
* Description: Magnetic Stim module Function and Variables define file
*
***********************************************************************************************/

#ifndef __BSP_EEPROM__H
#define __BSP_EEPROM__H


/*************************包含的头文件*************************/
#include "stm32f10x.h"
#include "string.h"
#include "peripheral.h"

/***************************宏定义区*****************************/
/***************************外部调用区***************************/

/*************************外部引用函数区************************/
FunctionalState I2C_ReadByte(uint8_t* pBuffer,   uint16_t length,   uint16_t ReadAddress,  uint8_t DeviceAddress);
FunctionalState I2C_WriteByte(uint8_t SendByte, uint16_t WriteAddress, uint8_t DeviceAddress);


#define AT24C02_ADR              0xA0        //AT24C02器件地址

#define EEPROM_MAX_LENGTH		 0x0F    //最大数据长度  刘老板增加2B频率值增加2B最大温度阈值

#define	THRESHOLD_POWER_BREAK	500   //电流反馈最小值 
#define	THRESHOLD_NO_LOAD		3900    //电流反馈最大值

#define THRESHOLD_TEMP1_DEFAULT  45
#define THRESHOLD_TEMP2_DEFAULT  45

#define MODULE_TYPE            0x12  // 模块型号——子宫复旧仪二代自动化
#define MAIN_HARDWARE_VER      0x01  // 主板硬件版本号

#define FIRMWARE_VER			     0x01	 // 固件版本号 0:V1.0  1:V1.0  2:V1.1  3:V1.2  ...  A:V1.9  B:V2.0	0x10:V2.5 0x15:V3.0
#define FIRMWARE_MIN_VER		   0x01	 // 固件小版本号 0-255	eg:	当主版本号为1.2，则0：V1.2.0	1：V1.2.1	2：V1.2.2
#define FIRMWARE_BOOT_VER      0x01  // 固件BOOT版本号

#define BLOCK_MAX              0x1F  // 块0——>块31

typedef enum
{
    E_MLD_ESU_NONE=0,
    E_MLD_ESU_001A,
    E_MLD_ESU_002A,
    E_MLD_ESU_003A,
    E_MLD_ESU_004A,
} MODEL_ENUM;

typedef enum
{
	 E_MODEL = 0,      //  设备不同子版本 01A,02A,03A,04A
	 E_SELF_PROJECT = 1,      // 设备一代，二代的档位，最大治疗时间
	 E_PROBE_OVERTEMP_THROLD = 2,     //  版本号 温度阈值
	 E_ZHUANGJIZHUCE = 3,   //  装机注册信息 
	 E_DEVIECE_SN = 4,     //  SN编码信息 
	 E_VERIFY_EEPROM = 7,
	 E_freq_data1_EEPROM = 8,   //  2B  增加刘老板 保存的频率
	 E_maxNTC_data1_EEPROM = 10,   //  2B  增加刘老板 治疗头保存的温度上限
	 E_maxNTC_data2_EEPROM = 12,   //  2B  增加刘老板 主板保存的温度上限
	 E_set_U_data1_EEPROM = 14,   //  2B  增加刘老板 保存的电压值
	 E_RFmaxNTC_data1_EEPROM = 16,   //  2B  增加刘老板 射频治疗头保存的温度上限
} BLOCK_ENUM;

typedef enum
{
    E_STORE_ENABLE = 0,
    E_STORE_DISABLE = 1,
} SOTRE_DIY_ENUM;

typedef enum
{
    E_Set_NONE = 0,
    E_Set_FAIL = 1,
	  E_Set_SUCCESSED = 2,
} SET_ENUM;

typedef enum
{
	 E_EEPROM_IDLE = 0,
	 E_EEPROM_BUSY = 1,
} EEPROM_OPERATION_ENUM;

typedef enum
{
	 E_READ_EE_BACK_NONE = 0,
	 E_READ_EE_BACK = 1,
} ATC2402C_READ_ENUM;

struct EEPROM_FLAG_BITS {
    __IO uint16_t  FinishReadEeprom           : 1;          // 读取成功标志位
    __IO uint16_t  FinishWriteEeprom          : 1;          // 写入成功标志位
    __IO uint16_t  Flag_UpDataEeprom          : 1;          // 更新EEPROM标志位
    __IO uint16_t  reserved0                  : 1;          // 预留0

    __IO uint16_t  Flag_StopDisplay           : 1;          // 预留4
    __IO uint16_t  Flag_Verify_CalculateTime  : 1;          // 预留5
    __IO uint16_t  Flag_Enter_WriteSn         : 1;          // 预留6
    __IO uint16_t  reserved7                  : 1;          // 预留7

    __IO uint16_t  reserved8                  : 1;          // 预留8
    __IO uint16_t  reserved9                  : 1;          // 预留9
    __IO uint16_t  reserved10                 : 1;          // 预留10
    __IO uint16_t  reserved11                 : 1;          // 预留11

    __IO uint16_t  reserved12                 : 1;          // 预留12
    __IO uint16_t  reserved13                 : 1;          // 预留13
    __IO uint16_t  reserved14                 : 1;          // 预留14
    __IO uint16_t  reserved15                 : 1;          // 预留15
};

union EEPROM_FLAG_UNION {
    uint16_t            		all;
    struct EEPROM_FLAG_BITS		bit;
};
/***************************结构体区***************************/
typedef struct
{
    union EEPROM_FLAG_UNION CMD;
    __IO uint16_t IC_Treat_Time;                    // 保存的治疗时间——子宫复旧仪（自定义）调用
	 __IO uint16_t freq_data1;        // 刘老板 工作频率
	 __IO uint16_t maxNTC_data1;    // 刘老板 治疗头最大温度
	 __IO uint16_t maxNTC_data2;    // 刘老板 主板最大温度
	 __IO uint16_t U_DCDC_data1;    // 刘老板 电压设置值
	 __IO uint16_t RFmaxNTC_data1;    // 刘老板 射频治疗头最大温度
    uint8_t	Data_Read[32];
    uint8_t	Data_Write[32];
    __IO uint8_t  Eeprom_Page_read;                 // 读存储数据的扇区
    __IO uint8_t  Eeprom_Page_Write;                // 写存储数据的扇区
    __IO uint8_t  IC_Treat_Power;                   // 保存的治疗档位——子宫复旧仪（自定义）调用
	  __IO uint8_t  SnLength;                         // 设备编号长度
	  __IO uint8_t  Temperature1;
	  __IO uint8_t  Temperature2;
    MODEL_ENUM    E_Model;                          // 设备型号类型
    SOTRE_DIY_ENUM E_StoreState;                    // 存储子宫复旧（自定义）状态位
	  EEPROM_OPERATION_ENUM E_EepromState;            // EEPROM操作状态位
	  SET_ENUM  E_SetState;                           // 是否写入成功
	  ATC2402C_READ_ENUM E_ReadBack_State;
	  char  IC_Verify[24];                            // 设备编号
} EEPROM_TypeDef;
/**************************枚举定义区**************************/
extern EEPROM_TypeDef St_Eeprom;
/*************************外部引用变量区************************/
void Eeprom_Para_Init(void);
void Eeprom_ReadeData_Handle(__IO uint8_t blockInfo, uint8_t *rx_buf);
void Eeprom_WriteData_Handle(const uint8_t blockInfo,uint8_t *tx_buf);
uint8_t EepromWrite_NeedReadBack_Handle(__IO BLOCK_ENUM Block,uint8_t *tx_buf,__IO uint8_t State,__IO uint8_t TypeState);
uint8_t Get_DeviceInfo(void);
void Refresh_Project(__IO uint8_t Level,__IO uint16_t Time,__IO uint8_t State);
void Get_Readfreq_data1(void);
void Get_Writefreq_data1(u16 Freq_vary);
void Get_ReadNTC_data1(void);
void Get_WriteNTC_data1(u16 Freq_vary);
void Get_U_DCDC_data1(void);
void RF_Get_U_DCDC_data1(void);

void Get_WriteU_DCDC_data1(u16 U_DCDC_vary);
void RF_Get_WriteU_DCDC_data1(u16 U_DCDC_vary);


//射频增加内容
void Get_RFReadfreq_data1(void);
void Get_RFReadNTC_data1(void);
void Get_RFWriteNTC_data1(u16 NTC_vary);

/*************************外部引用函数区************************/
#endif
/*************************************************************************************************
*									END OF FILE
*************************************************************************************************/


/*************************************************************************************************
*									END OF FILE
*************************************************************************************************/

