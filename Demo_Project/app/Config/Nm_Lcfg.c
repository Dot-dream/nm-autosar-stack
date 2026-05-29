#include "Nm_Lcfg.h"
#include "../../middleware/NM/Nm.h"
#include "../../middleware/NM/Nm_ConfigTypes.h"
#include "log_port.h"
#include <string.h>

/* ===== 单通道配置 ===== */
static Nm_ChannelConfigType NmChannel = {
    .channelHandle        = 0,
    .nodeId               = 0x0A,
    .busType              = NM_BUS_CAN,
#if NM_TEST_MODE == 0
    .nmMode               = NM_MODE_DIRECT,
#elif NM_TEST_MODE == 1
    .nmMode               = NM_MODE_INDIRECT,
#elif NM_TEST_MODE == 2
    .nmMode               = NM_MODE_AUTOSAR,
#endif

    .wireConfig = {
        .wireFormat         = NM_WIRE_FORMAT_OPCODE,
        .singleCanId        = 0x402,        /* NM 报文 CAN ID (与平台适配一致) */
        .pduOpCodeAlive     = 0x01,
        .pduOpCodeRing      = 0x02,
        .pduOpCodeLimpHome  = 0x04,
        .canIdBase          = 0x400,
        .canIdMax           = 0x4FF,
    },

    /* 定时器 (ms) */
    .timerTyp            = 1000,    /* TTyp: 1s */
    .timerMax            = 2000,    /* TMax: 2s */
    .timerError          = 1000,    /* TError: 1s */
    .timerWaitBusSleep   = 5000,    /* TWbs: 5s */
    .timerTx             = 100,     /* TTx: 100ms */
    .timerNmMsgCycle     = 1000,    /* AUTOSAR NM cycle */

    /* 重试限制 */
    .rxCountLimit        = 4,
    .txCountLimit        = 8,

    /* 可选功能 */
    .busLoadReductionActive = 0,
    .immediateTxConfEnabled = 0,
    .busSyncEnabled       = 0,

    /* CFMOTO 扩展 */
    .hasCfmotoExtensions    = 0,
    .cfmotoTSleepRequestMin = 0,
    .cfmotoTLimpSleepReqMin = 0,
    .cfmotoTLimpHomeDTC     = 0,

    /* 回调 */
    .stateChangeCallback = NULL,
    .pduRxCallback       = NULL,
};

/* ===== 全局配置 ===== */
const Nm_ConfigType NmConfig = {
    .numChannels          = 1,
    .channels             = &NmChannel,
    .mainFunctionPeriodMs = 5,
};

void Nm_Lcfg_PrintInfo(void)
{
    log_info("NM Config: mode=%d nodeId=0x%02X TTyp=%dms TMax=%dms TWbs=%dms",
#if NM_TEST_MODE == 0
             (int)NM_TEST_MODE,
#elif NM_TEST_MODE == 1
             (int)NM_TEST_MODE,
#else
             (int)NM_TEST_MODE,
#endif
             (unsigned int)NmChannel.nodeId,
             (int)NmChannel.timerTyp,
             (int)NmChannel.timerMax,
             (int)NmChannel.timerWaitBusSleep);
}
