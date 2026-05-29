#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

// 任务系统配置
#define TASK_CONFIG_ENABLE_SCHEDULER    1
#define TASK_CONFIG_MAX_TASKS           16
#define TASK_CONFIG_MAX_EVENTS          64
#define TASK_CONFIG_USE_EVENTS          1

// 系统时钟配置
#define TASK_CONFIG_TICK_FREQ_HZ        1000    // 1kHz
#define TASK_CONFIG_TICK_PERIOD_MS      1       // 1ms

// 任务优先级配置
#define TASK_PRIORITY_HIGH               3
#define TASK_PRIORITY_NORMAL             2
#define TASK_PRIORITY_LOW                1

// 调试配置
#define TASK_CONFIG_DEBUG_ENABLED        1
#define TASK_CONFIG_PROFILE_ENABLED      0

#endif // TASK_CONFIG_H