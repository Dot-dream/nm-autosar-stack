#ifndef MODULE_LED_H
#define MODULE_LED_H

#include <stdint.h>
#include <stdbool.h>

// LED引脚定义
typedef enum {
    LED_D5 = 0,
    LED_D6,
    LED_D7,
    LED_COUNT
} led_id_t;

// LED状态
typedef enum {
    LED_OFF = 0,
    LED_ON = 1
} led_state_t;

// LED控制接口
void led_init(void);
void led_set(led_id_t led, led_state_t state);
void led_toggle(led_id_t led);
void led_all_set(led_state_t state);
void led_test_all(void);

// LED任务接口（使用调度器）
void led_task_init(void);
extern void led_task_function(void *arg);

#endif // MODULE_LED_H