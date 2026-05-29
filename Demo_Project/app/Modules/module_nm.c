#include "module_nm.h"
#include "module_nm_callback.h"
#include "../Config/Nm_Lcfg.h"
#include "../../middleware/NM/Nm.h"
#include "../System/scheduler.h"
#include "../../Bsp/Hal/bsp_can_hal/bsp_can_hal.h"
#include "log_port.h"

static task_tcb_t nm_task_tcb;

/* CAN 命令 ID */
#define NM_CMD_CAN_ID       0x7E0U
#define NM_CMD_NETWORK_REQ  0x01U
#define NM_CMD_NETWORK_REL  0x02U
#define NM_CMD_DISABLE_COMM 0x03U
#define NM_CMD_ENABLE_COMM  0x04U
#define NM_CMD_BUSOFF       0x05U
#define NM_CMD_IG_ON        0x06U
#define NM_CMD_IG_OFF       0x07U

/* RX 回调：将 CAN 消息转发给 NM 模块，或解析控制命令 */
static void nm_can_rx_handler(uint32_t can_id, const uint8_t *data, uint8_t len)
{
    /* NM 控制命令 (CAN ID 0x7E0) */
    if (can_id == NM_CMD_CAN_ID && len >= 1U) {
        switch (data[0]) {
            case NM_CMD_NETWORK_REQ:
                log_info("NM: CAN CMD NetworkRequest");
                Nm_NetworkRequest(0);
                break;
            case NM_CMD_NETWORK_REL:
                log_info("NM: CAN CMD NetworkRelease");
                Nm_NetworkRelease(0);
                break;
            case NM_CMD_DISABLE_COMM:
                log_info("NM: CAN CMD DisableCommunication");
                Nm_DisableCommunication(0);
                break;
            case NM_CMD_ENABLE_COMM:
                log_info("NM: CAN CMD EnableCommunication");
                Nm_EnableCommunication(0);
                break;
            case NM_CMD_BUSOFF:
                log_info("NM: CAN CMD ControllerBusOff");
                Nm_ControllerBusOff(0);
                break;
            case NM_CMD_IG_ON:
                log_info("NM: CAN CMD IG_ON -> NetworkRequest");
                Nm_NetworkRequest(0);
                break;
            case NM_CMD_IG_OFF:
                log_info("NM: CAN CMD IG_OFF -> NetworkRelease");
                Nm_NetworkRelease(0);
                break;
            default:
                break;
        }
        return;
    }

    /* NM PDU (CAN ID 0x400-0x4FF) */
    if (can_id >= 0x400U && can_id <= 0x4FFU) {
        Nm_RxIndication(0, data, len);
    } else {
        log_debug("NM: non-NM CAN RX 0x%03X ignored", (unsigned int)can_id);
    }
}

/* ===== 初始化 ===== */
void nm_module_init(void)
{
    Hal_Can_Init();

    /* 注册 NM PDU 接收 */
    Hal_Can_RegisterRx(0x402U, nm_can_rx_handler);
    Hal_Can_RegisterRx(0x403U, nm_can_rx_handler);
    /* 注册 NM 控制命令接收 */
    Hal_Can_RegisterRx(NM_CMD_CAN_ID, nm_can_rx_handler);

    nm_callback_init();
    log_info("NM 模块初始化完成");
}

/* ===== 调度器任务 ===== */
void nm_task_init(void)
{
    Nm_Init(&NmConfig);
    Nm_Lcfg_PrintInfo();
    scheduler_create_task(&nm_task_tcb, "NM_Task", nm_task_function);
    log_info("NM 任务注册完成");
}

void nm_task_function(void *arg)
{
    task_tcb_t *task = (task_tcb_t *)arg;

    switch (task->state) {

        case 0: /* 启动：等待初始化完成 */
            task->time = scheduler_get_tick_ms();
            task->state = 1;
            break;

        case 1: /* 等待 2s，确保硬件稳定，之后进入待命状态 */
            if (scheduler_get_tick_ms() - task->time >= 2000U) {
                log_info("NM: 就绪，等待 CAN 命令");
                task->state = 2;
            }
            break;

        case 2: /* 正常运行：周期性调用 Nm_MainFunction */
            break;

        default:
            task->state = 2;
            break;
    }

    /* 每 5ms 调用一次 NM 主循环 */
    Nm_MainFunction();

    /* 轮询 CAN 接收 */
    Hal_Can_Poll();

    /* 状态变化时输出日志（非周期） */
    {
        static uint8_t last_state = 0xFF;
        uint8_t state, mode;
        Nm_GetState(0, &state, &mode);
        if (state != last_state) {
            log_info("NM: state %d->%d mode=%d tick=%lu",
                     (int)last_state, (int)state, (int)mode,
                     (unsigned long)scheduler_get_tick_ms());
            last_state = state;
        }
    }

    /* 5ms 后再次调度 */
    if (task->state == 2) {
        task->time = scheduler_get_tick_ms();
        task->state = 3;
    } else if (task->state == 3) {
        if (scheduler_get_tick_ms() - task->time >= 5U) {
            task->state = 2;
        }
    }
}

/* ===== 测试接口 ===== */
void nm_request_network(void)
{
    log_info("NM: 手动 NetworkRequest");
    Nm_NetworkRequest(0);
}

void nm_release_network(void)
{
    log_info("NM: 手动 NetworkRelease");
    Nm_NetworkRelease(0);
}

uint8_t nm_get_state(void)
{
    uint8_t state, mode;
    Nm_GetState(0, &state, &mode);
    return state;
}
