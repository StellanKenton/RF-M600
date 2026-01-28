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
#include "app_ultrasound.h"
#include "app_radiofreq.h"
#include "app_shockwave.h"
#include "app_negprsheat.h"

App_Comm_Info_t s_AppCommInfo;
static Protocol_Frame_t RxFrame;



static void App_Comm_RecvDataHandle(const Protocol_Frame_t *pRxFrame);
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
    uint16_t Crc16 = Crc16Compute(UartRxData+6, RxFrame.data_len);
    uint16_t Crc16_recv = UartRxData[RxFrame.data_len+6] | UartRxData[RxFrame.data_len+7] << 8;
    RxFrame.crc16 = Crc16_recv;
    if(Crc16_recv != Crc16){
        CBuff_Pop(pRxBuffer, UartRxData, 2);
        return;
    }
    RxFrame.data = UartRxData + 6;
    App_Comm_RecvDataHandle(&RxFrame);
    CBuff_Pop(pRxBuffer, UartRxData, RxFrame.data_len+8);
}



static void App_Comm_RecvDataHandle(const Protocol_Frame_t *pRxFrame)
{
    if(pRxFrame == NULL || pRxFrame->data == NULL){
        return;
    }
    const uint8_t *pData = pRxFrame->data;

    switch(pRxFrame->module)
    {
        case PROTOCOL_MODULE_ULTRASOUND:
            switch(pRxFrame->cmd) 
            {
                case PROTOCOL_CMD_GET_STATUS:
                    s_AppCommInfo.US.flag.bits.Rely_Status = 1;
                    break;
                case PROTOCOL_CMD_SET_WORK_STATE:
                    if(pRxFrame->data_len < 4){
                        break;
                    }
                    s_AppCommInfo.US.RxWorkState.work_state = pData[0];
                    s_AppCommInfo.US.RxWorkState.work_time = pData[1]  | (uint16_t)pData[2] << 8;
                    s_AppCommInfo.US.RxWorkState.work_level = pData[3];
                    s_AppCommInfo.US.RxValidFlag[PROTOCOL_CMD_SET_WORK_STATE] = true;
                    break; 
                case PROTOCOL_CMD_SET_CONFIG:
                    if(pRxFrame->data_len < 6){
                        break;
                    }
                    s_AppCommInfo.US.RxConfig.frequency = pData[0] | (uint16_t)pData[1] << 8;
                    s_AppCommInfo.US.RxConfig.voltage = pData[2] | (uint16_t)pData[3] << 8;
                    s_AppCommInfo.US.RxConfig.temp_limit = pData[4] | (uint16_t)pData[5] << 8;
                    s_AppCommInfo.US.flag.bits.Rely_Config = 1;
                    s_AppCommInfo.US.RxValidFlag[PROTOCOL_CMD_SET_CONFIG] = true;
                    break;
            }
            break;
        case PROTOCOL_MODULE_RADIO_FREQ:
            switch(pRxFrame->cmd) 
            {
                case PROTOCOL_CMD_GET_STATUS:
                    s_AppCommInfo.RF.flag.bits.Rely_Status = 1;
                    break;
                case PROTOCOL_CMD_SET_WORK_STATE:
                    if(pRxFrame->data_len < 4){
                        break;
                    }
                    s_AppCommInfo.RF.RxWorkState.work_state = pData[0];
                    s_AppCommInfo.RF.RxWorkState.work_time = pData[1] | (uint16_t)pData[2] << 8;
                    s_AppCommInfo.RF.RxWorkState.work_level = pData[3];
                    break; 
                case PROTOCOL_CMD_SET_CONFIG:
                    if(pRxFrame->data_len < 2){
                        break;
                    }
                    s_AppCommInfo.RF.RxConfig.temp_limit = pData[0] | (uint16_t)pData[1] << 8;
                    s_AppCommInfo.RF.flag.bits.Rely_Config = 1;
                    break;
            }
            break;
        case PROTOCOL_MODULE_SHOCKWAVE:
            switch(pRxFrame->cmd) 
            {
                case PROTOCOL_CMD_GET_STATUS:
                    s_AppCommInfo.SW.flag.bits.Rely_Status = 1;
                    break;
                case PROTOCOL_CMD_SET_WORK_STATE:
                    if(pRxFrame->data_len < 5){
                        break;
                    }
                    s_AppCommInfo.SW.RxWorkState.work_state = pData[0];
                    s_AppCommInfo.SW.RxWorkState.work_time = pData[1] | (uint16_t)pData[2] << 8;
                    s_AppCommInfo.SW.RxWorkState.work_level = pData[3];
                    s_AppCommInfo.SW.RxWorkState.frequency = pData[4];
                    break; 
            }
            break;
        case PROTOCOL_MODULE_HEAT:
            switch(pRxFrame->cmd) 
            {
                case PROTOCOL_CMD_GET_STATUS:
                    s_AppCommInfo.Heat.flag.bits.Rely_Status = 1;
                    break;
                case PROTOCOL_CMD_SET_WORK_STATE:
                    if(pRxFrame->data_len < 10){
                        break;
                    }
                    s_AppCommInfo.Heat.RxWorkState.work_state = pData[0];
                    s_AppCommInfo.Heat.RxWorkState.work_time = pData[1] | (uint16_t)pData[2] << 8;
                    s_AppCommInfo.Heat.RxWorkState.pressure = pData[3];
                    s_AppCommInfo.Heat.RxWorkState.suck_time = pData[4] | (uint16_t)pData[5] << 8;
                    s_AppCommInfo.Heat.RxWorkState.release_time = pData[6] | (uint16_t)pData[7] << 8;
                    s_AppCommInfo.Heat.RxWorkState.temp_limit = pData[8] | (uint16_t)pData[9] << 8;
                    break; 
                case PROTOCOL_CMD_SET_CONFIG:
                    if(pRxFrame->data_len < 5){
                        break;
                    }
                    s_AppCommInfo.Heat.RxPreheat.preheat_state = pData[0];
                    s_AppCommInfo.Heat.RxPreheat.work_time = pData[1] | (uint16_t)pData[2] << 8;
                    s_AppCommInfo.Heat.RxPreheat.temp_limit = pData[3] | (uint16_t)pData[4] << 8;
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


void App_Comm_CreateAndSend(uint8_t module, uint8_t cmd, void *pData, uint16_t data_len)
{
    uint16_t CaculateCrc;
    s_AppCommInfo.TxData[0] = PROTOCOL_HEADER_0;
    s_AppCommInfo.TxData[1] = PROTOCOL_HEADER_1;
    s_AppCommInfo.TxData[2] = PROTOCOL_DIR_DEV_TO_HOST;
    s_AppCommInfo.TxData[3] = module;
    s_AppCommInfo.TxData[4] = cmd;
    s_AppCommInfo.TxData[5] = data_len;
    memcpy(s_AppCommInfo.TxData+6, pData, data_len);
    CaculateCrc = Crc16Compute(s_AppCommInfo.TxData+6, data_len);
    s_AppCommInfo.TxData[data_len+6] = CaculateCrc & 0xFF;
    s_AppCommInfo.TxData[data_len+7] = CaculateCrc >> 8;
    Drv_USART1_Send(s_AppCommInfo.TxData, data_len+8);
}

static void App_Comm_ReplyUSStatus(void)
{
    US_GetStatus_Reply_t *pStatus = App_UltraSound_GetStatus();
    uint8_t TxData[128];
    uint8_t DataLen = 0;
    if(pStatus == NULL){
        return;
    }
    s_AppCommInfo.US.TxStatus = *pStatus;
    TxData[DataLen++] = s_AppCommInfo.US.TxStatus.work_state;
    TxData[DataLen++] = s_AppCommInfo.US.TxStatus.frequency & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.US.TxStatus.frequency >> 8;
    TxData[DataLen++] = s_AppCommInfo.US.TxStatus.temp_limit & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.US.TxStatus.temp_limit >> 8;
    TxData[DataLen++] = s_AppCommInfo.US.TxStatus.remain_time & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.US.TxStatus.remain_time >> 8;
    TxData[DataLen++] = s_AppCommInfo.US.TxStatus.work_level;
    TxData[DataLen++] = s_AppCommInfo.US.TxStatus.head_temp & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.US.TxStatus.head_temp >> 8;
    TxData[DataLen++] = s_AppCommInfo.US.TxStatus.conn_state;
    TxData[DataLen++] = s_AppCommInfo.US.TxStatus.error_code;
    App_Comm_CreateAndSend(PROTOCOL_MODULE_ULTRASOUND, PROTOCOL_CMD_GET_STATUS, TxData, DataLen);
}

static void App_Comm_ReplyRFStatus(void)
{
    RF_GetStatus_Reply_t *pStatus = App_RadioFreq_GetStatus();
    uint8_t TxData[128];
    uint8_t DataLen = 0;
    if(pStatus == NULL){
        return;
    }
    s_AppCommInfo.RF.TxStatus = *pStatus;
    TxData[DataLen++] = s_AppCommInfo.RF.TxStatus.work_state;
    TxData[DataLen++] = s_AppCommInfo.RF.TxStatus.temp_limit & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.RF.TxStatus.temp_limit >> 8;
    TxData[DataLen++] = s_AppCommInfo.RF.TxStatus.remain_time & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.RF.TxStatus.remain_time >> 8;
    TxData[DataLen++] = s_AppCommInfo.RF.TxStatus.work_level;
    TxData[DataLen++] = s_AppCommInfo.RF.TxStatus.head_temp & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.RF.TxStatus.head_temp >> 8;
    TxData[DataLen++] = s_AppCommInfo.RF.TxStatus.conn_state;
    TxData[DataLen++] = s_AppCommInfo.RF.TxStatus.error_code;
    App_Comm_CreateAndSend(PROTOCOL_MODULE_RADIO_FREQ, PROTOCOL_CMD_GET_STATUS, TxData, DataLen);
}

static void App_Comm_ReplySWStatus(void)
{
    SW_GetStatus_Reply_t *pStatus = App_Shockwave_GetStatus();
    uint8_t TxData[128];
    uint8_t DataLen = 0;
    if(pStatus == NULL){
        return;
    }
    s_AppCommInfo.SW.TxStatus = *pStatus;
    TxData[DataLen++] = s_AppCommInfo.SW.TxStatus.work_state;
    TxData[DataLen++] = s_AppCommInfo.SW.TxStatus.frequency;
    TxData[DataLen++] = s_AppCommInfo.SW.TxStatus.remain_time & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.SW.TxStatus.remain_time >> 8;
    TxData[DataLen++] = s_AppCommInfo.SW.TxStatus.work_level;
    TxData[DataLen++] = s_AppCommInfo.SW.TxStatus.head_temp & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.SW.TxStatus.head_temp >> 8;
    TxData[DataLen++] = s_AppCommInfo.SW.TxStatus.conn_state;
    TxData[DataLen++] = s_AppCommInfo.SW.TxStatus.error_code;
    App_Comm_CreateAndSend(PROTOCOL_MODULE_SHOCKWAVE, PROTOCOL_CMD_GET_STATUS, TxData, DataLen);
}

static void App_Comm_ReplyHeatStatus(void)
{
    Heat_GetStatus_Reply_t *pStatus = App_NegPrsHeat_GetStatus();
    uint8_t TxData[128];
    uint8_t DataLen = 0;
    if(pStatus == NULL){
        return;
    }
    s_AppCommInfo.Heat.TxStatus = *pStatus;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.work_state;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.temp_limit & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.temp_limit >> 8;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.remain_heat_time & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.remain_heat_time >> 8;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.suck_time & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.suck_time >> 8;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.release_time & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.release_time >> 8;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.pressure;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.head_temp & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.head_temp >> 8;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.preheat_state;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.preheat_temp_limit & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.preheat_temp_limit >> 8;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.remain_preheat_time & 0xFF;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.remain_preheat_time >> 8;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.conn_state;
    TxData[DataLen++] = s_AppCommInfo.Heat.TxStatus.error_code;
    App_Comm_CreateAndSend(PROTOCOL_MODULE_HEAT, PROTOCOL_CMD_GET_STATUS, TxData, DataLen);
}

void App_Comm_SendData(void)
{
    if(Drv_GetUSART1_DMA_SendStatus()){
        return;
    }

    if(s_AppCommInfo.US.flag.bits.Rely_Status){
        s_AppCommInfo.US.flag.bits.Rely_Status = 0;
        App_Comm_ReplyUSStatus();
    }

    if(s_AppCommInfo.RF.flag.bits.Rely_Status){
        s_AppCommInfo.RF.flag.bits.Rely_Status = 0;
        App_Comm_ReplyRFStatus();
    }

    if(s_AppCommInfo.SW.flag.bits.Rely_Status){
        s_AppCommInfo.SW.flag.bits.Rely_Status = 0;
        App_Comm_ReplySWStatus();
    }

    if(s_AppCommInfo.Heat.flag.bits.Rely_Status){
        s_AppCommInfo.Heat.flag.bits.Rely_Status = 0;
        App_Comm_ReplyHeatStatus();
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
