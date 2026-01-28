/***********************************************************************************
* @file     : app_comm.c
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "app_comm.h"
#include "lib_ringbuffer.h"
#include "drv_usart.h"

App_Comm_Info_t s_AppCommInfo;
static Protocol_Frame_t RxFrame;



void App_Comm_RecvDataHandle(uint8_t *Data);
uint16_t Crc16Compute(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0x0000;
    
    while (length--) {
        uint8_t b = *data++;
        
        // 杈撳叆鍙嶈浆锛堜娇鐢ㄥ惊鐜�锛岃妭鐪佷唬鐮佺┖闂达級
        uint8_t r = 0;
        for (uint8_t i = 0; i < 8; i++) {
            r = (r << 1) | (b & 0x01);
            b >>= 1;
        }
        
        crc ^= (uint16_t)r << 8;
        
        // 澶勭悊8浣�
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x8005;
            } else {
                crc <<= 1;
            }
        }
    }
    
    // 杈撳嚭鍙嶈浆锛堜娇鐢ㄥ惊鐜�锛�
    uint16_t result = 0;
    for (uint8_t i = 0; i < 16; i++) {
        result = (result << 1) | (crc & 0x01);
        crc >>= 1;
    }
    
    return result;
}
/* =============================================================================
 * Private Functions
 * ============================================================================= */

void App_Comm_RecvData(void)
{
    static uint8_t UartRxData[APP_COMM_RX_BUFFER_SIZE];
    static uint16_t OverTime = 0;
    CBuff* pRxBuffer = Drv_GetUsart1RingPtr();
    if(pRxBuffer == NULL){
        return;
    }
    if(CBuff_GetLength(pRxBuffer) < 6){
        return;
    }
    
    CBuff_Read(pRxBuffer, UartRxData, 6);
    if(UartRxData[0] != PROTOCOL_HEADER_0 || UartRxData[1] != PROTOCOL_HEADER_1 || UartRxData[2] != PROTOCOL_DIR_HOST_TO_DEV){
        CBuff_Pop(pRxBuffer, UartRxData, 1);
        return;
    }
    RxFrame.header[0] = UartRxData[0];
    RxFrame.header[1] = UartRxData[1];
    RxFrame.direction = UartRxData[2];
    RxFrame.module = UartRxData[3];
    RxFrame.cmd = UartRxData[4];
    RxFrame.data_len = UartRxData[5];

    if(CBuff_GetLength(pRxBuffer) < RxFrame.data_len+8){
        OverTime += APP_COMM_RUN_INTERVAL;
        if(OverTime >= 200){
            OverTime = 0;
            CBuff_Pop(pRxBuffer, UartRxData, 1);
            return;
        }
        return;
    }
    OverTime = 0;
    CBuff_Read(pRxBuffer, UartRxData, RxFrame.data_len+8);
    uint16_t Crc16 = Crc16Compute(UartRxData, RxFrame.data_len+6);
    uint16_t Crc16_recv = UartRxData[RxFrame.data_len+6] | UartRxData[RxFrame.data_len+7] << 8;
    RxFrame.crc16 = Crc16_recv;
    if(Crc16_recv != Crc16){
        CBuff_Pop(pRxBuffer, UartRxData, 2);
        return;
    }
    RxFrame.data = UartRxData + 6;
    App_Comm_RecvDataHandle(RxFrame.data);
    CBuff_Pop(pRxBuffer, UartRxData, RxFrame.data_len+8);
}



void App_Comm_RecvDataHandle(uint8_t *Data)
{
    if(Data == NULL){
        return;
    }

    switch(Data[3])
    {
        case PROTOCOL_MODULE_ULTRASOUND:
            switch(Data[4]) 
            {
                case PROTOCOL_CMD_GET_STATUS:
                    s_AppCommInfo.US.flag.bits.Rely_Status = 1;
                    break;
                case PROTOCOL_CMD_SET_WORK_STATE:
                    s_AppCommInfo.US.RxWorkState.work_state = Data[6];
                    s_AppCommInfo.US.RxWorkState.work_time = Data[7]  | Data[8] << 8;
                    s_AppCommInfo.US.RxWorkState.work_level = Data[9];
                    s_AppCommInfo.US.RxValidFlag[PROTOCOL_CMD_SET_WORK_STATE] = true;
                    break; 
                case PROTOCOL_CMD_SET_CONFIG:
                    s_AppCommInfo.US.RxConfig.frequency = Data[6] | Data[7] << 8;
                    s_AppCommInfo.US.RxConfig.voltage = Data[8] | Data[9] << 8;
                    s_AppCommInfo.US.RxConfig.temp_limit = Data[10] | Data[11] << 8;
                    s_AppCommInfo.US.flag.bits.Rely_Config = 1;
                    s_AppCommInfo.US.RxValidFlag[PROTOCOL_CMD_SET_CONFIG] = true;
                    break;
            }
            break;
        case PROTOCOL_MODULE_RADIO_FREQ:
            switch(Data[4]) 
            {
                case PROTOCOL_CMD_GET_STATUS:
                    s_AppCommInfo.RF.flag.bits.Rely_Status = 1;
                    break;
                case PROTOCOL_CMD_SET_WORK_STATE:
                    s_AppCommInfo.RF.RxWorkState.work_state = Data[6];
                    s_AppCommInfo.RF.RxWorkState.work_time = Data[7] | Data[8] << 8;
                    s_AppCommInfo.RF.RxWorkState.work_level = Data[9];
                    break; 
                case PROTOCOL_CMD_SET_CONFIG:
                    s_AppCommInfo.RF.RxConfig.temp_limit = Data[6] | Data[7] << 8;
                    s_AppCommInfo.RF.flag.bits.Rely_Config = 1;
                    break;
            }
            break;
        case PROTOCOL_MODULE_SHOCKWAVE:
            switch(Data[4]) 
            {
                case PROTOCOL_CMD_GET_STATUS:
                    s_AppCommInfo.SW.flag.bits.Rely_Status = 1;
                    break;
                case PROTOCOL_CMD_SET_WORK_STATE:
                    s_AppCommInfo.SW.RxWorkState.work_state = Data[6];
                    s_AppCommInfo.SW.RxWorkState.work_time = Data[7] | Data[8] << 8;
                    s_AppCommInfo.SW.RxWorkState.work_level = Data[9];
                    s_AppCommInfo.SW.RxWorkState.frequency = Data[10];
                    break; 
            }
            break;
        case PROTOCOL_MODULE_HEAT:
            switch(Data[4]) 
            {
                case PROTOCOL_CMD_GET_STATUS:
                    s_AppCommInfo.Heat.flag.bits.Rely_Status = 1;
                    break;
                case PROTOCOL_CMD_SET_WORK_STATE:
                    s_AppCommInfo.Heat.RxWorkState.work_state = Data[6];
                    s_AppCommInfo.Heat.RxWorkState.work_time = Data[7] | Data[8] << 8;
                    s_AppCommInfo.Heat.RxWorkState.pressure = Data[9];
                    s_AppCommInfo.Heat.RxWorkState.suck_time = Data[10] | Data[11] << 8;
                    s_AppCommInfo.Heat.RxWorkState.release_time = Data[12] | Data[13] << 8;
                    s_AppCommInfo.Heat.RxWorkState.temp_limit = Data[14] | Data[15] << 8;
                    break; 
                case PROTOCOL_CMD_SET_CONFIG:
                    s_AppCommInfo.Heat.RxPreheat.preheat_state = Data[6];
                    s_AppCommInfo.Heat.RxPreheat.work_time = Data[7] | Data[8] << 8;
                    s_AppCommInfo.Heat.RxPreheat.temp_limit = Data[9] | Data[10] << 8;
                    s_AppCommInfo.Heat.flag.bits.Rely_Config = 1;
                    break;
            }
            break;
        default:
            break;
    }
}


UltraSound_TransData_t *App_Comm_GetUSTransData(void)
{
    return &s_AppCommInfo.US;
}

RF_TransData_t *App_Comm_GetRFTransData(void)
{
    return &s_AppCommInfo.RF;
}

SW_TransData_t *App_Comm_GetSWTransData(void)
{
    return &s_AppCommInfo.SW;
}

Heat_TransData_t *App_Comm_GetHeatTransData(void)
{
    return &s_AppCommInfo.Heat;
}

void App_Comm_SendData(void)
{
    if(Drv_GetUSART1_DMA_SendStatus()){
        return;
    }

    if(s_AppCommInfo.US.flag.bits.Rely_Status){
        s_AppCommInfo.US.flag.bits.Rely_Status = 0;
        
    }
}

/* =============================================================================
 * Public Functions
 * ============================================================================= */

void App_Comm_Init(void)
{
    memset(&s_AppCommInfo, 0, sizeof(App_Comm_Info_t));
}

void App_Comm_Process(void)
{
    App_Comm_RecvData();
    App_Comm_SendData();
}

/**************************End of file********************************/
