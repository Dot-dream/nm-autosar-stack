#include "module_event.h"
#include "../../../middleware/utility_print/log/log.h"
#include "../../../middleware/utility_print/log_port.h"
#include "../System/scheduler.h"
#include "log_port.h"

// 事件演示任务控制块
static task_tcb_t event_task_tcb;

// 事件模块初始化
void event_module_init(void)
{
    log_info("事件模块初始化完成");
}

// 事件演示任务初始化
void event_demo_task_init(void)
{
    scheduler_create_task(&event_task_tcb, "EVENT_Task", event_demo_task_function);
    //log_info("事件任务初始化完成");
}

// 触发按键事件
void event_trigger_button_press(void)
{
    scheduler_notify_event(EVENT_BUTTON_PRESS);
    //log_debug("触发按键事件");
}

// 触发定时器超时事件
void event_trigger_timer_timeout(void)
{
    scheduler_notify_event(EVENT_TIMER_TIMEOUT);
    //log_debug("触发定时器超时事件");
}

// task_delay_ms 和 task_wait_event 函数在 scheduler.c 中实现

// 事件演示任务实现
void event_demo_task_function(void *arg)
{
    task_tcb_t *task = (task_tcb_t *)arg;
    
    // 注册感兴趣的事件
    scheduler_register_event(task, EVENT_BUTTON_PRESS);
    scheduler_register_event(task, EVENT_TIMER_TIMEOUT);
    
    while (1) {
        // 等待按键事件
        task_wait_event(task, EVENT_BUTTON_PRESS, SCHEDULER_WAIT_FOREVER);
        if (task->state != 0) return;
        
        log_debug("收到按键事件，处理中...");
        scheduler_clear_event(task, EVENT_BUTTON_PRESS);
        
        // 模拟处理时间
        task_delay_ms(task, 1000);
        if (task->state != 0) return;
        
        // 触发定时器事件
        event_trigger_timer_timeout();
        
        // 等待定时器事件（带超时）
        task_wait_event(task, EVENT_TIMER_TIMEOUT, 3000);
        if (task->state != 0) return;
        
        if (task->events & EVENT_TIMER_TIMEOUT) {
            log_debug("收到定时器事件，继续执行");
            scheduler_clear_event(task, EVENT_TIMER_TIMEOUT);
        } else {
            //log_debug("等待定时器事件超时");
        }
        
        task_delay_ms(task, 2000);
        if (task->state != 0) return;
    }
}