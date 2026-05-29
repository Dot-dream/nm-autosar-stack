#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

// 调度器配置
#define SCHEDULER_MAX_TASKS    16
#define SCHEDULER_MAX_EVENTS   64
#define SCHEDULER_WAIT_FOREVER 0xFFFFFFFFUL

// 调度器性能统计（可选）
typedef struct {
    uint32_t total_tasks;
    uint32_t active_tasks;
    uint32_t events_processed;
    uint32_t max_events_in_queue;
} scheduler_stats_t;

// 任务状态定义
typedef int32_t task_state_t;

// 任务状态常量（保持向后兼容）
#define TASK_STATE_READY     0
#define TASK_STATE_SUSPENDED -1
#define TASK_STATE_WAITING   1

// 任务函数类型
typedef void (*task_func_t)(void *arg);

// 任务控制块
typedef struct task_tcb {
    char *name;
    task_func_t func;
    void *arg;
    task_state_t state;
    uint32_t time;
    uint64_t events;
    struct task_tcb *next;
    struct task_tcb *event_next;
} task_tcb_t;

// 事件定义
typedef uint64_t event_t;

// 事件宏定义
#define EVENT_NONE     ((event_t)0)
#define EVENT_1        ((event_t)1u<<0)
#define EVENT_2        ((event_t)1u<<1)
#define EVENT_3        ((event_t)1u<<2)
#define EVENT_4        ((event_t)1u<<3)
// ... 可根据需要扩展

// 调度器接口
void scheduler_init(void);
bool scheduler_create_task(task_tcb_t *task, const char *name, task_func_t func);
void scheduler_start(void);
void scheduler_stop(void);
void scheduler_suspend_task(task_tcb_t *task);
void scheduler_resume_task(task_tcb_t *task);
void scheduler_run_once(void);

// 事件接口
void scheduler_register_event(task_tcb_t *task, event_t events);
void scheduler_notify_event(event_t events);
void scheduler_clear_event(task_tcb_t *task, event_t events);

// 最简单的任务宏定义
#define TASK_BEGIN \
    task_tcb_t *task = (task_tcb_t *)arg;

#define TASK_END /* 空实现 */

// 临时禁用复杂宏，使用简单函数
#define TASK_DELAY_MS(ms) \
    /* 临时禁用 - 将后续用函数实现 */

#define TASK_WAIT_EVENT(events, timeout) \
    /* 临时禁用 - 将后续用函数实现 */

#define TASK_REGISTER_EVENT(events) scheduler_register_event(task, events)
#define TASK_CLEAR_EVENT(events) scheduler_clear_event(task, events)

// 简单的任务接口函数
void task_delay_ms(task_tcb_t *task, uint32_t ms);
void task_wait_event(task_tcb_t *task, event_t events, uint32_t timeout);

// 系统时钟接口（由移植层实现）
void scheduler_increment_tick(void);
uint32_t scheduler_get_tick(void);
uint32_t scheduler_get_tick_ms(void);

#endif // SCHEDULER_H