#include "nm_platform.h"
#include "../Modules/module_nm.h"
#include "scheduler.h"
#include "../../Bsp/Hal/bsp_can_hal/bsp_can_hal.h"
#include "log_port.h"

/* NM 通道 → CAN ID 映射表 */
#define NM_CH0_CAN_TX_ID  0x402U   /* NM 消息发送 CAN ID */
#define NM_CH0_CAN_RX_ID0 0x402U   /* NM 消息接收 CAN ID */
#define NM_CH0_CAN_RX_ID1 0x403U

/* ===== 毫秒 Tick ===== */
uint32_t Nm_Timer_GetTick(void)
{
    return scheduler_get_tick_ms();
}

/* ===== CAN 发送 ===== */
CanNm_ReturnType CanNm_Transmit(uint8_t channel, const uint8_t* pduData, uint8_t pduLength)
{
    bool ok;
    (void)channel;
    log_info("NM: TX CAN ID=0x%03X len=%d data=%02X %02X %02X %02X %02X %02X %02X %02X",
             (unsigned int)NM_CH0_CAN_TX_ID, (int)pduLength,
             (unsigned int)pduData[0], (unsigned int)pduData[1],
             (unsigned int)pduData[2], (unsigned int)pduData[3],
             (unsigned int)pduData[4], (unsigned int)pduData[5],
             (unsigned int)pduData[6], (unsigned int)pduData[7]);
    ok = Hal_Can_Send(NM_CH0_CAN_TX_ID, pduData, pduLength);
    if (!ok) {
        log_error("NM: TX FAILED!");
    }
    return ok ? CANNM_E_OK : CANNM_E_NOT_OK;
}

/* ===== CAN 控制器使能/禁能 ===== */
void CanNm_ControllerEnable(uint8_t channel)
{
    (void)channel;
    log_debug("NM: CAN Controller Enable");
}

void CanNm_ControllerDisable(uint8_t channel)
{
    (void)channel;
    log_debug("NM: CAN Controller Disable");
}
