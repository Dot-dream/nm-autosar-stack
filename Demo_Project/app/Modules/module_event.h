#ifndef MODULE_EVENT_H
#define MODULE_EVENT_H

#include "../System/scheduler.h"
#include <stdint.h>

// 事件定义
#define EVENT_BUTTON_PRESS    ((event_t)1u<<0)
#define EVENT_TIMER_TIMEOUT   ((event_t)1u<<1)
#define EVENT_CAN_RX          ((event_t)1u<<2)

// 事件模块接口
void event_module_init(void);
void event_demo_task_init(void);
extern void event_demo_task_function(void *arg);

// 事件控制接口
void event_trigger_button_press(void);
void event_trigger_timer_timeout(void);

#endif // MODULE_EVENT_H