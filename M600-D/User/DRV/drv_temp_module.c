/************************************************************************************
 * @file     : drv_temp_module.c
 * @brief    : GY-MCU90614 temperature module driver on USART2 (PA2/PA3)
 ***********************************************************************************/
#include "drv_temp_module.h"
#include "drv_iodevice.h"
#include "drv_usart.h"
#include "drv_delay.h"
#include "stm32f10x.h"
#include "lib_ringbuffer.h"
#include <string.h>

#define TEMP_FRAME_HEADER            0x5Au
#define TEMP_RING_HANDCOMM_HEADER_1  0xA5u
#define TEMP_FRAME_TYPE_TEMPERATURE  0x45u
#define TEMP_FRAME_DATA_LEN          0x04u

#define TEMP_PROCESS_INTERVAL_MS     5u
#define TEMP_CONFIG_INTERVAL_MS      200u
#define TEMP_COMM_TIMEOUT_MS         2000u
#define TEMP_TIMEOUT_OBJECT_TEMP     20000  /* 200.00 deg C in centi-C */

static const uint8_t s_tempCmdAutoOutput[] = {0xA5u, 0x51u, 0xF6u}; // {0xA5u, 0x45u, 0xEAu};//

static Drv_TempModule_Data_t s_tempData;
static bool     s_tempDataValid;
static uint16_t s_commTimeoutMs;

/* ---------------------------------------------------------------------------
 * Check whether the head of the shared UART2 ring buffer is a HandComm frame
 * (header 0x5A 0xA5).  If so the temp module must yield and let HandComm
 * consume it.
 * --------------------------------------------------------------------------- */
static bool Drv_TempModule_IsHandCommFrame(CBuff *pRxBuffer)
{
    uint8_t header[2];
    if (pRxBuffer == NULL || CBuff_GetLength(pRxBuffer) < 2u) {
        return false;
    }
    if (CBuff_Read(pRxBuffer, header, 2u) == false) {
        return false;
    }
    return (header[0] == TEMP_FRAME_HEADER) && (header[1] == TEMP_RING_HANDCOMM_HEADER_1);
}

/* ---------------------------------------------------------------------------
 * Parse one temperature frame from the UART2 ring buffer.
 *
 * Frame format (e.g. dataLen = 4, type = 0x45):
 *   Byte0  : 0x5A          header
 *   Byte1  : 0x5A          header
 *   Byte2  : type          0x45=temp, 0x25=emissivity, 0x35=temp offset
 *   Byte3  : dataLen       number of data bytes that follow
 *   Byte4  : Data1 High
 *   Byte5  : Data1 Low     -> object temperature (signed, *100)
 *   Byte6  : Data2 High
 *   Byte7  : Data2 Low     -> ambient temperature (signed, *100)
 *   Byte8  : checksum      (Byte0..Byte7 sum, low 8 bits)
 *
 * Total frame length = dataLen + 5.
 * --------------------------------------------------------------------------- */
static void Drv_TempModule_RecvData(void)
{
    static uint8_t  rxBuf[32];
    static uint16_t overTime = 0;

    CBuff *pRxBuffer = Drv_GetUsart2RingPtr();
    if (pRxBuffer == NULL) {
        return;
    }

    /* If the head of the buffer belongs to HandComm, do not touch it. */
    if (Drv_TempModule_IsHandCommFrame(pRxBuffer)) {
        return;
    }

    /* Need at least header(2) + type(1) + dataLen(1) = 4 bytes to begin. */
    if (CBuff_GetLength(pRxBuffer) < 4u) {
        return;
    }

    CBuff_Read(pRxBuffer, rxBuf, 4u);

    /* Verify frame header 0x5A 0x5A */
    if (rxBuf[0] != TEMP_FRAME_HEADER || rxBuf[1] != TEMP_FRAME_HEADER) {
        CBuff_Pop(pRxBuffer, rxBuf, 1u);
        return;
    }

    /* Verify data type: 0x45=temperature, 0x25=emissivity, 0x35=temp offset */
    uint8_t frameType = rxBuf[2];
    if (frameType != TEMP_FRAME_TYPE_TEMPERATURE &&
        frameType != 0x25u && frameType != 0x35u) {
        CBuff_Pop(pRxBuffer, rxBuf, 1u);
        return;
    }

    uint8_t  dataLen  = rxBuf[3];
    uint16_t frameLen = (uint16_t)dataLen + 5u;   /* header(2)+type(1)+len(1)+data(N)+chk(1) */

    /* Sanity ¨C prevent local buffer overflow */
    if (frameLen > sizeof(rxBuf)) {
        CBuff_Pop(pRxBuffer, rxBuf, 2u);
        return;
    }

    /* Wait until the full frame has arrived */
    if (CBuff_GetLength(pRxBuffer) < frameLen) {
        overTime += TEMP_PROCESS_INTERVAL_MS;
        if (overTime >= 200u) {
            overTime = 0;
            CBuff_Pop(pRxBuffer, rxBuf, 1u);
        }
        return;
    }
    overTime = 0;

    /* Peek the complete frame */
    CBuff_Read(pRxBuffer, rxBuf, frameLen);

    /* Checksum: low-8-bit sum of Byte0 ¡­ Byte(3+dataLen) */
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < (uint16_t)(4u + dataLen); i++) {
        checksum += rxBuf[i];
    }
    if (checksum != rxBuf[4u + dataLen]) {
        CBuff_Pop(pRxBuffer, rxBuf, 2u);
        return;
    }

    /* Only extract temperatures from type 0x45 frames */
    if (frameType == TEMP_FRAME_TYPE_TEMPERATURE && dataLen >= TEMP_FRAME_DATA_LEN) {
        s_tempData.object_temp_centi_c  = (int16_t)((uint16_t)rxBuf[4] << 8 | rxBuf[5]);
        s_tempData.ambient_temp_centi_c = (int16_t)((uint16_t)rxBuf[6] << 8 | rxBuf[7]);
        s_tempDataValid  = true;
    }
    s_commTimeoutMs = 0;

    /* Consume the whole frame */
    CBuff_Pop(pRxBuffer, rxBuf, frameLen);
}

/* =============================================================================
 * Public Functions
 * ============================================================================= */

void Drv_TempModule_Init(void)
{
    memset(&s_tempData, 0, sizeof(s_tempData));
    s_tempDataValid  = false;
    s_commTimeoutMs  = 0;
    Drv_USART2_Send(s_tempCmdAutoOutput, sizeof(s_tempCmdAutoOutput));
}

void Drv_TempModule_Process(void)
{
    static Drv_Timer_t tempTimer;
    static uint16_t configResendMs = 0;

    if (Drv_Timer_Tick(&tempTimer, TEMP_PROCESS_INTERVAL_MS) == false) {
        return;
    }

    uint16_t prevTimeout = s_commTimeoutMs;

    Drv_TempModule_RecvData();

    /* RecvData resets s_commTimeoutMs to 0 on successful frame parse.
     * If it was NOT reset, increment the offline counter only when the
     * current probe is RF ¨C other modes don't rely on this sensor.  */
    if (s_commTimeoutMs == prevTimeout) {
        if (Drv_IODevice_GetProbeStatus() == E_IODEVICE_MODE_RADIO_FREQUENCY) {
            s_commTimeoutMs += TEMP_PROCESS_INTERVAL_MS;
        }
    }

    /* RF offline timeout: 2 s with no temp data ¡ú force 200.00 ¡ãC */
    if (s_commTimeoutMs >= TEMP_COMM_TIMEOUT_MS) {
        s_commTimeoutMs = TEMP_COMM_TIMEOUT_MS;   /* clamp */
        s_tempData.object_temp_centi_c = TEMP_TIMEOUT_OBJECT_TEMP;
        s_tempDataValid = false;
    }

    /* Before first valid data or during timeout, resend auto-output cmd every 200ms */
    if (!s_tempDataValid || s_commTimeoutMs >= TEMP_COMM_TIMEOUT_MS) {
        configResendMs += TEMP_PROCESS_INTERVAL_MS;
        if (configResendMs >= TEMP_CONFIG_INTERVAL_MS) {
            configResendMs = 0;
            Drv_USART2_Send(s_tempCmdAutoOutput, sizeof(s_tempCmdAutoOutput));
        }
    } else {
        configResendMs = 0;
    }
}

bool Drv_TempModule_GetLatest(Drv_TempModule_Data_t *pOut)
{
    if (pOut == NULL) {
        return false;
    }
    *pOut = s_tempData;
    return true;
}

