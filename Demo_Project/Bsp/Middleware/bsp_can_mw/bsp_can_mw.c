#include "bsp_can_mw.h"
#include "flexcan_driver.h"
#include "can_config.h"

#define MW_CAN_INST         (flexcanInitConfig1_INST)
#define MW_CAN_TX_MB        (16U)
#define MW_CAN_RX_MB_BASE   (17U)
#define MW_CAN_MAX_RX       (4U)

/* 注册槽位 */
typedef struct {
    uint32_t              can_id;
    uint8_t               mb_idx;
    Hal_Can_RxCallback_t  callback;
    bool                  active;
} mw_can_rx_slot_t;

static mw_can_rx_slot_t mw_rx_slots[MW_CAN_MAX_RX];
static uint8_t mw_rx_slot_count = 0;
static flexcan_msgbuff_t mw_rx_buffer;
static bool mw_initialized = false;

/* 标准 CAN 帧数据格式 (8 字节，非 FD) */
static const flexcan_data_info_t mw_can_std_info = {
    .msg_id_type = FLEXCAN_MSG_ID_STD,
    .data_length = 8U,
    .fd_enable   = false,
    .fd_padding  = 0U,
    .enable_brs  = false,
    .is_remote   = false,
};

void Mw_Can_Init(void)
{
    /* 使用 board 生成的 CAN 配置初始化 */
    FLEXCAN_DRV_Init(MW_CAN_INST, &flexcanInitConfig1_State, &flexcanInitConfig1);

    /* 配置 TX mailbox */
    FLEXCAN_DRV_ConfigTxMb(MW_CAN_INST, MW_CAN_TX_MB, &mw_can_std_info, 0x326U);

    /* 清空槽位 */
    for (int i = 0; i < MW_CAN_MAX_RX; i++) {
        mw_rx_slots[i].active   = false;
        mw_rx_slots[i].callback = NULL;
    }
    mw_rx_slot_count = 0;
    mw_initialized = true;
}

int Mw_Can_RegisterRx(uint32_t can_id, Hal_Can_RxCallback_t cb)
{
    if (!mw_initialized || mw_rx_slot_count >= MW_CAN_MAX_RX || cb == NULL) {
        return -1;
    }

    uint8_t idx = mw_rx_slot_count;
    uint8_t mb  = MW_CAN_RX_MB_BASE + idx;

    /* 配置 mailbox 为 RX */
    if (FLEXCAN_DRV_ConfigRxMb(MW_CAN_INST, mb, &mw_can_std_info, can_id) != STATUS_SUCCESS) {
        return -1;
    }

    /* 启动接收 */
    if (FLEXCAN_DRV_Receive(MW_CAN_INST, mb, &mw_rx_buffer) != STATUS_SUCCESS) {
        return -1;
    }

    mw_rx_slots[idx].can_id   = can_id;
    mw_rx_slots[idx].mb_idx   = mb;
    mw_rx_slots[idx].callback = cb;
    mw_rx_slots[idx].active   = true;
    mw_rx_slot_count++;

    return (int)idx;
}

bool Mw_Can_Send(uint32_t can_id, const uint8_t *data, uint8_t len)
{
    (void)len;
    if (!mw_initialized) {
        return false;
    }
    return (FLEXCAN_DRV_Send(MW_CAN_INST, MW_CAN_TX_MB, &mw_can_std_info, can_id, data) == STATUS_SUCCESS);
}

void Mw_Can_Poll(void)
{
    if (!mw_initialized) {
        return;
    }

    for (int i = 0; i < mw_rx_slot_count; i++) {
        if (!mw_rx_slots[i].active) {
            continue;
        }

        uint8_t mb = mw_rx_slots[i].mb_idx;

        if (FLEXCAN_DRV_GetTransferStatus(MW_CAN_INST, mb) == STATUS_SUCCESS) {
            /* 消息就绪，调用回调 */
            mw_rx_slots[i].callback(mw_rx_slots[i].can_id,
                                    mw_rx_buffer.data,
                                    mw_rx_buffer.dataLen);
            /* 重新启动接收 */
            FLEXCAN_DRV_Receive(MW_CAN_INST, mb, &mw_rx_buffer);
        }
    }
}
