#include "module_log.h"
#include "../../../middleware/utility_print/log/log.h"
#include "../../../middleware/utility_print/log_port.h"
#include "../System/scheduler.h"

// 定时日志任务控制块
static task_tcb_t log_task_tcb;

// 日志模块初始化
void log_module_init(void)
{
    // 初始化日志系统
    log_init(LOG_INFO);
    log_info("日志模块初始化完成");
}

// 定期日志任务初始化
void log_periodic_task_init(void)
{
    scheduler_create_task(&log_task_tcb, "LOG_Task", log_periodic_task_function);
    log_info("日志任务初始化完成");
}

// 定期日志任务实现
void log_periodic_task_function(void *arg)
{
    task_tcb_t *task = (task_tcb_t *)arg;
    
    while (1) {
        // 简单延时实现
        if (task->state == 0) {
            task->time = scheduler_get_tick_ms();
            task->state = 1;
            return;
        }
        if (scheduler_get_tick_ms() - task->time < LOG_PERIODIC_INTERVAL_MS) {
            return;
        }
        task->state = 0;
        
        //log_debug("定期任务执行中...");
        //log_debug("系统tick: %u", scheduler_get_tick());
    }
}