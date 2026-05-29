#ifndef LOG_PORT_H
#define LOG_PORT_H

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化日志系统
 * 
 * @param level 默认日志级别 (可选，如使用默认配置则传-1)
 */
void log_init(int level);

/**
 * @brief 获取当前日志级别（移植层维护的级别）
 * 
 * @return int 当前日志级别
 */
int log_port_get_level(void);

#ifdef __cplusplus
}
#endif

#endif /* LOG_PORT_H */