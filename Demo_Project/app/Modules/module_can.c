#include "module_can.h"
#include "bsp_can_hal.h"
#include "../System/scheduler.h"
#include "log_port.h"

#define CAN_TX_ID (0x326U)
#define CAN_RX_ID (0x325U)
#define CAN_TX_INTERVAL_MS (500U)

static task_tcb_t can_task_tcb;

/* RX 消息处理：收到 0x325 时打印 */
static void can_rx_handler(uint32_t can_id, const uint8_t *data, uint8_t len)
{
    log_info("CAN RX 0x%03X len=%d: %02X %02X %02X %02X %02X %02X %02X %02X",
             (unsigned int)can_id, (int)len,
             (unsigned int)data[0], (unsigned int)data[1],
             (unsigned int)data[2], (unsigned int)data[3],
             (unsigned int)data[4], (unsigned int)data[5],
             (unsigned int)data[6], (unsigned int)data[7]);
}

void can_module_init(void)
{
    Hal_Can_Init();

    /* 注册 RX 消息 0x325 */
    int idx = Hal_Can_RegisterRx(CAN_RX_ID, can_rx_handler);
    if (idx >= 0)
    {
        log_info("CAN 注册 RX 0x%03X (slot=%d)", (unsigned int)CAN_RX_ID, idx);
    }
    else
    {
        log_error("CAN 注册 RX 0x%03X 失败", (unsigned int)CAN_RX_ID);
    }

    log_info("CAN模块初始化完成");
}

void can_task_function(void *arg)
{
    task_tcb_t *task = (task_tcb_t *)arg;
    static uint8_t tx_counter = 0;

    switch (task->state)
    {
    case 0: /* 启动 */
        task->state = 1;
        break;

    case 1: /* 发送 mock 数据 */
    {
        uint8_t data[8] = {0};
        data[0] = tx_counter++;
        data[1] = 0xAA;
        data[2] = 0xBB;

        Hal_Can_Send(CAN_TX_ID, data, 8);
        log_debug("CAN TX 0x%03X: %02X %02X %02X ...",
                  (unsigned int)CAN_TX_ID,
                  (unsigned int)data[0],
                  (unsigned int)data[1],
                  (unsigned int)data[2]);

        task->time = scheduler_get_tick_ms();
        task->state = 2;
    }
    break;

    case 2: /* 等待 100ms 再发下一条 */
        if (scheduler_get_tick_ms() - task->time >= CAN_TX_INTERVAL_MS)
        {
            task->state = 1;
        }
        break;

    default:
        task->state = 0;
        break;
    }

    /* 每次调用都轮询 RX */
    Hal_Can_Poll();
}

void can_task_init(void)
{
    scheduler_create_task(&can_task_tcb, "CAN_Task", can_task_function);
    log_info("CAN任务初始化完成");
}
