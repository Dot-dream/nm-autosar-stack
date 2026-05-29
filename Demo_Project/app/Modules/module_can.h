#ifndef MODULE_CAN_H
#define MODULE_CAN_H

#include <stdint.h>
#include <stdbool.h>

/* CAN 模块初始化（硬件初始化 + 注册默认 RX handler） */
void can_module_init(void);

/* 注册 CAN 调度器任务 */
void can_task_init(void);

/* CAN 调度器任务函数 */
extern void can_task_function(void *arg);

#endif /* MODULE_CAN_H */
