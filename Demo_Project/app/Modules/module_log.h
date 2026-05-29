#ifndef MODULE_LOG_H
#define MODULE_LOG_H

#include <stdint.h>
#include <stdbool.h>

// 日志模块接口
void log_module_init(void);
void log_periodic_task_init(void);
extern void log_periodic_task_function(void *arg);

// 日志配置
#define LOG_PERIODIC_INTERVAL_MS  2000

#endif // MODULE_LOG_H