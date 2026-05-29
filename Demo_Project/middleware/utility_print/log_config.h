#ifndef LOG_CONFIG_H
#define LOG_CONFIG_H

// 日志系统配置选项

// 启用颜色输出
#define LOG_USE_COLOR

// 启用时间戳功能 (0-禁用, 1-启用)
// 在裸机平台建议禁用，避免标准库依赖
#define LOG_ENABLE_TIMESTAMP  0

// 默认日志级别
#define LOG_DEFAULT_LEVEL     LOG_INFO

// 注意：移除了时间戳格式配置，因为不使用标准库strftime

#endif /* LOG_CONFIG_H */