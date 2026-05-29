#include "module_nm_callback.h"
#include "../../middleware/NM/Nm.h"
#include "../../middleware/NM/Nm_Cbk.h"
#include "log_port.h"

/* 状态名映射（便于日志输出） */
static const char* state_name(uint8_t s) {
    switch(s) {
        case NM_STATE_UNINIT:             return "UNINIT";
        case NM_STATE_BUS_SLEEP:          return "BUS_SLEEP";
        case NM_STATE_PREPARE_BUS_SLEEP:  return "PREPARE_BUS_SLEEP";
        case NM_STATE_READY_SLEEP:        return "READY_SLEEP";
        case NM_STATE_NORMAL_OPERATION:   return "NORMAL_OP";
        case NM_STATE_REPEAT_MESSAGE:     return "REPEAT_MSG";
        case NM_STATE_SYNCHRONIZE:        return "SYNC";
        case NM_STATE_LIMPHOME:           return "LIMPHOME";
        case NM_STATE_LIMPHOME_PREPSLEEP: return "LIMPHOME_PS";
        case NM_STATE_TWBS_LIMPHOME:      return "TWBS_LH";
        case NM_STATE_INITRESET:          return "INITRESET";
        case NM_STATE_TWBS_NORMAL:        return "TWBS_NORM";
        default:                          return "?";
    }
}

void nm_callback_init(void)
{
    log_info("NM 回调注册完成");
}

/* ===== 状态通知回调 ===== */
void Nm_NetworkStartIndication(uint8_t ch) {
    log_info("NM [ch%d] << NetworkStart (wakeup)", (int)ch);
}
void Nm_NetworkMode(uint8_t ch) {
    log_info("NM [ch%d] << NetworkMode", (int)ch);
}
void Nm_PrepareBusSleepMode(uint8_t ch) {
    log_info("NM [ch%d] << PrepareBusSleep", (int)ch);
}
void Nm_BusSleepMode(uint8_t ch) {
    log_info("NM [ch%d] << BusSleep", (int)ch);
}
void Nm_StateChangeNotification(uint8_t ch, uint8_t prev, uint8_t cur) {
    log_info("NM [ch%d] State: %s -> %s", (int)ch, state_name(prev), state_name(cur));
}

/* ===== 数据通知回调 ===== */
void Nm_PduRxIndication(uint8_t ch) {
    log_debug("NM [ch%d] PDU Rx", (int)ch);
}
void Nm_RemoteSleepIndication(uint8_t ch) {
    log_info("NM [ch%d] RemoteSleepInd", (int)ch);
}
void Nm_RemoteSleepCancellation(uint8_t ch) {
    log_info("NM [ch%d] RemoteSleepCancel", (int)ch);
}

/* ===== 错误通知回调 ===== */
void Nm_TxTimeoutException(uint8_t ch) {
    log_error("NM [ch%d] TxTimeout!", (int)ch);
}
void Nm_LimpHomeIndication(uint8_t ch) {
    log_info("NM [ch%d] LimpHome!", (int)ch);
}
void Nm_RestartIndication(uint8_t ch) {
    log_info("NM [ch%d] RestartInd", (int)ch);
}
void Nm_CoordReadyToSleep(uint8_t ch) {
    log_info("NM [ch%d] CoordReadyToSleep", (int)ch);
}
