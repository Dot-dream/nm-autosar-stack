#include "scheduler.h"
#include <string.h>

// 全局变量
static task_tcb_t *task_list = NULL;
static task_tcb_t *task_head = NULL;
static task_tcb_t *event_map[SCHEDULER_MAX_EVENTS];

// 内部宏
#define GET_TASK_FROM_PTR(ptr, member) \
    ((task_tcb_t *)((char *)(ptr) - (uintptr_t)(&((task_tcb_t *)0)->member)))

// 调度器初始化
void scheduler_init(void)
{
    task_list = NULL;
    task_head = NULL;
    memset(event_map, 0, sizeof(event_map));
}

// 创建任务
bool scheduler_create_task(task_tcb_t *task, const char *name, task_func_t func)
{
    if (task == NULL || func == NULL) {
        return false;
    }
    
    // 添加到任务链表
    if (task_head == NULL) {
        task_head = task;
        task_list = task;
    } else {
        task_list->next = task;
        task_list = task;
    }
    
    // 形成循环链表
    task->next = task_head;
    task->name = (char *)name;
    task->func = func;
    task->arg = task;
    task->state = TASK_STATE_READY;
    task->events = EVENT_NONE;
    task->event_next = NULL;
    
    return true;
}

// 挂起任务
void scheduler_suspend_task(task_tcb_t *task)
{
    if (task != NULL) {
        task->state = TASK_STATE_SUSPENDED;
    }
}

// 恢复任务
void scheduler_resume_task(task_tcb_t *task)
{
    if (task != NULL) {
        task->state = TASK_STATE_READY;
    }
}

// 运行一次调度
void scheduler_run_once(void)
{
    if (task_head == NULL) {
        return;
    }
    
    task_tcb_t *current = task_head;
    do {
        if ((current->func != NULL) && (current->state != TASK_STATE_SUSPENDED)) {
            current->func(current->arg);
        }
        current = current->next;
    } while (current != task_head);
}

// 启动调度器
void scheduler_start(void)
{
    // 调度器在main循环中运行，这里只是一个标记
}

// 停止调度器
void scheduler_stop(void)
{
    scheduler_init();
}

// 注册事件
void scheduler_register_event(task_tcb_t *task, event_t events)
{
    if (task == NULL || events == EVENT_NONE) {
        return;
    }
    
    for (int i = 0; i < SCHEDULER_MAX_EVENTS; i++) {
        event_t mask = (event_t)1u << i;
        if (events & mask) {
            task_tcb_t **node = &event_map[i];
            
            // 检查是否已注册
            task_tcb_t *current = *node;
            while (current != NULL) {
                if (current == task) {
                    break;
                }
                current = current->event_next;
            }
            
            // 添加到事件链表头部
            if (current == NULL) {
                task->event_next = *node;
                *node = task;
            }
        }
    }
}

// 通知事件
void scheduler_notify_event(event_t events)
{
    if (events == EVENT_NONE) {
        return;
    }
    
    for (int i = 0; i < SCHEDULER_MAX_EVENTS; i++) {
        event_t mask = (event_t)1u << i;
        if (events & mask) {
            task_tcb_t *current = event_map[i];
            while (current != NULL) {
                current->events |= mask;
                current = current->event_next;
            }
        }
    }
}

// 清除事件
void scheduler_clear_event(task_tcb_t *task, event_t events)
{
    if (task != NULL) {
        task->events &= ~(events);
    }
}

// 简单的任务延时函数
void task_delay_ms(task_tcb_t *task, uint32_t ms)
{
    if (task == NULL) {
        return;
    }
    
    if (task->state == 0) {
        task->time = scheduler_get_tick_ms();
        task->state = 1;
        return;
    }
    
    if (scheduler_get_tick_ms() - task->time >= ms) {
        task->state = 0;
    }
}

// 简单的任务事件等待函数
void task_wait_event(task_tcb_t *task, event_t events, uint32_t timeout)
{
    if (task == NULL) {
        return;
    }
    
    // 注册事件
    scheduler_register_event(task, events);
    
    // 检查事件是否已经就绪
    if (task->events & events) {
        // 事件已就绪
        return;
    }
    
    if (task->state == 0) {
        task->time = scheduler_get_tick_ms();
        task->state = 1;
        return;
    }
    
    // 检查超时
    if (timeout != SCHEDULER_WAIT_FOREVER) {
        if (scheduler_get_tick_ms() - task->time >= timeout) {
            task->state = 0;
            return;
        }
    }
    
    // 检查事件
    if (task->events & events) {
        task->state = 0;
    }
}

// 系统时钟接口（由移植层实现）
__attribute__((weak)) void scheduler_increment_tick(void) {}
__attribute__((weak)) uint32_t scheduler_get_tick(void) { return 0; }
__attribute__((weak)) uint32_t scheduler_get_tick_ms(void) { return 0; }