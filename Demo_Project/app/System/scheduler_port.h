#ifndef SCHEDULER_PORT_H
#define SCHEDULER_PORT_H

// 调度器移植层配置
#define SCHEDULER_USE_SOFTWARE_TIMER 1
#define SCHEDULER_USE_IDLE_TASK 1

// 系统时钟接口（必须在main.c中实现）
void scheduler_increment_tick(void);
uint32_t scheduler_get_tick(void);
uint32_t scheduler_get_tick_ms(void);

// 平台相关配置
#define SCHEDULER_MAX_EVENTS   64
#define SCHEDULER_MAX_TASKS    16

// 编译器相关配置
#ifndef __weak
#define __weak __attribute__((weak))
#endif

#endif // SCHEDULER_PORT_H