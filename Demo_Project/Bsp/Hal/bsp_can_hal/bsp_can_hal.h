#ifndef _BSP_CAN_HAL_H_
#define _BSP_CAN_HAL_H_

#include <stdint.h>
#include <stdbool.h>

/* CAN 消息接收回调 */
typedef void (*Hal_Can_RxCallback_t)(uint32_t can_id, const uint8_t *data, uint8_t len);

/* CAN 初始化 */
void Hal_Can_Init(void);

/* 注册 RX 消息回调，返回索引，失败返回 -1 */
int Hal_Can_RegisterRx(uint32_t can_id, Hal_Can_RxCallback_t cb);

/* 发送 CAN 消息 */
bool Hal_Can_Send(uint32_t can_id, const uint8_t *data, uint8_t len);

/* 轮询所有 RX mailbox（由调度器任务周期性调用） */
void Hal_Can_Poll(void);

#endif /* _BSP_CAN_HAL_H_ */
