#include "log.h"
#include "printf.h"
#include "log_port.h"
#include "log_config.h"
#include <stdarg.h>

// 当前日志级别（移植层维护）
static int current_log_level = LOG_DEFAULT_LEVEL;

// 颜色定义 (与log.c中的保持一致)
#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
    "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

// 极简时间戳实现（避免使用标准库）
#if LOG_ENABLE_TIMESTAMP
static void get_simple_time(char *buf, size_t size)
{
    // 在裸机平台，你需要实现自己的时间获取
    // 这里提供一个简单的占位实现
    static unsigned int counter = 0;
    counter++;
    snprintf_(buf, size, "%08u", counter); // 使用计数作为简单时间戳
}
#endif

// 带颜色和时间戳的完整格式回调
static void color_printf_callback(log_Event *ev)
{
    const char *level_strings[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

#if LOG_ENABLE_TIMESTAMP
    char time_buf[LOG_TIMESTAMP_BUFSIZE];
    get_simple_time(time_buf, sizeof(time_buf));
    printf_("%s ", time_buf);
#endif

#ifdef LOG_USE_COLOR
    printf_("%s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
            level_colors[ev->level], level_strings[ev->level], ev->file, ev->line);
#else
    printf_("%-5s %s:%d: ", level_strings[ev->level], ev->file, ev->line);
#endif

    // 使用自定义vprintf
    va_list ap_copy;
    va_copy(ap_copy, ev->ap);
    vprintf_(ev->fmt, ap_copy);
    va_end(ap_copy);

    // printf_("\n");
    printf_("\r\n");
}

// 简化格式回调（无颜色）
static void simple_printf_callback(log_Event *ev)
{
    const char *level_strings[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

#if LOG_ENABLE_TIMESTAMP
    char time_buf[LOG_TIMESTAMP_BUFSIZE];
    get_simple_time(time_buf, sizeof(time_buf));
    printf_("%s ", time_buf);
#endif

    printf_("%-5s %s:%d: ", level_strings[ev->level], ev->file, ev->line);

    // 使用自定义vprintf
    va_list ap_copy;
    va_copy(ap_copy, ev->ap);
    vprintf_(ev->fmt, ap_copy);
    va_end(ap_copy);

    printf_("\r\n");
    // printf_("\n");
}

// 自定义初始化事件函数
void custom_init_event(log_Event *ev, void *udata)
{
    // 在裸机平台，我们不使用标准库的时间功能
    ev->time = NULL; // 设置为NULL，避免标准库调用
    ev->udata = udata;
}

void log_init(int level)
{
    // 设置日志级别
    if (level >= LOG_TRACE && level <= LOG_FATAL)
    {
        current_log_level = level;
    }
    else
    {
        current_log_level = LOG_DEFAULT_LEVEL;
    }

    // 调用log库的级别设置函数
    log_set_level(current_log_level);

    // 禁用log库的默认输出（避免使用fprintf）
    log_set_quiet(true);

    // 添加基于printf的回调函数
    // 根据配置选择回调函数
#ifdef LOG_USE_COLOR
    log_add_callback(color_printf_callback, NULL, LOG_TRACE);
#else
    log_add_callback(simple_printf_callback, NULL, LOG_TRACE);
#endif

    // 输出初始化信息（使用log_info，因为此时日志系统已初始化）
    log_info("日志系统初始化完成");
    log_info("日志级别: %s", log_level_string(current_log_level));
#if LOG_ENABLE_TIMESTAMP
    log_info("时间戳: 已启用(简化版)");
#else
    log_info("时间戳: 已禁用");
#endif
#ifdef LOG_USE_COLOR
    log_info("颜色输出: 已启用");
#else
    log_info("颜色输出: 已禁用");
#endif
}

int log_port_get_level(void)
{
    return current_log_level;
}