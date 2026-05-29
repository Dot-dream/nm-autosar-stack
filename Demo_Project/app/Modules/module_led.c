
#include "module_led.h"
#include "../../Bsp/Hal/bsp_gpio_hal/bsp_gpio_hal.h"
#include "../../../middleware/utility_print/log/log.h"
#include "../../../middleware/utility_print/log_port.h"
#include "../System/scheduler.h"
#include "log_port.h"

// LED任务控制块
static task_tcb_t led_task_tcb;

// LED延时时间宏定义
#define LED_DELAY_MS    50UL

// LED引脚映射
static const struct {
    uint32_t pin;
    const char *name;
} led_config[LED_COUNT] = {
    {HAL_PIN_GPIO_D5, "D5"},
    {HAL_PIN_GPIO_D6, "D6"},
    {HAL_PIN_GPIO_D7, "D7"}
};

// LED硬件初始化
void led_init(void)
{
    // LED引脚在Board_Init()中已初始化
    // 这里只需要设置初始状态
    led_all_set(LED_OFF);
}

// 设置单个LED
void led_set(led_id_t led, led_state_t state)
{
    if (led >= LED_COUNT) {
        return;
    }
    
    Hal_Gpio_Write(led_config[led].pin, 
                   state == LED_ON ? HAL_HIGH : HAL_LOW);
}

// 切换LED状态
void led_toggle(led_id_t led)
{
    if (led >= LED_COUNT) {
        return;
    }
    
    // 读取当前状态并切换
    uint32_t current = Hal_Gpio_Read(led_config[led].pin);
    Hal_Gpio_Write(led_config[led].pin, 
                   current == HAL_HIGH ? HAL_LOW : HAL_HIGH);
}

// 设置所有LED
void led_all_set(led_state_t state)
{
    for (int i = 0; i < LED_COUNT; i++) {
        led_set(i, state);
    }
}

// LED测试（逐个点亮）
void led_test_all(void)
{
    for (int i = 0; i < LED_COUNT; i++) {
        led_set(i, LED_ON);
        ////log_debug("LED %s ON", led_config[i].name);
        // 调用者控制延时
    }
}

// LED任务初始化
void led_task_init(void)
{
    led_init();
    scheduler_create_task(&led_task_tcb, "LED_Task", led_task_function);
    ////log_debug("LED任务初始化完成");
}

// LED任务实现
void led_task_function(void *arg)
{
    task_tcb_t *task = (task_tcb_t *)arg;
    
    // 使用静态变量作为状态计数器
    static uint32_t led_state = 0;
    
    switch (task->state) {
        case 0: // 初始状态，开始第一个LED状态
            led_state = 0;
            task->state = 1;
            break;
            
        case 1: // D5亮，其他灭
            led_set(LED_D5, LED_ON);
            led_set(LED_D6, LED_OFF);
            led_set(LED_D7, LED_OFF);
            ////log_debug("LED D5 ON, D6 OFF, D7 OFF");
            
            // 设置延时
            task->time = scheduler_get_tick_ms();
            task->state = 2;
            break;
            
        case 2: // 等待D5延时结束
            if (scheduler_get_tick_ms() - task->time >= LED_DELAY_MS) {
                task->state = 3;
            }
            break;
            
        case 3: // D6亮，其他灭
            led_set(LED_D5, LED_OFF);
            led_set(LED_D6, LED_ON);
            led_set(LED_D7, LED_OFF);
            ////log_debug("LED D5 OFF, D6 ON, D7 OFF");
            
            // 设置延时
            task->time = scheduler_get_tick_ms();
            task->state = 4;
            break;
            
        case 4: // 等待D6延时结束
            if (scheduler_get_tick_ms() - task->time >= LED_DELAY_MS) {
                task->state = 5;
            }
            break;
            
        case 5: // D7亮，其他灭
            led_set(LED_D5, LED_OFF);
            led_set(LED_D6, LED_OFF);
            led_set(LED_D7, LED_ON);
            ////log_debug("LED D5 OFF, D6 OFF, D7 ON");
            
            // 设置延时
            task->time = scheduler_get_tick_ms();
            task->state = 6;
            break;
            
        case 6: // 等待D7延时结束
            if (scheduler_get_tick_ms() - task->time >= LED_DELAY_MS) {
                task->state = 7;
            }
            break;
            
        case 7: // 全灭
            led_all_set(LED_OFF);
            //log_debug("All LEDs OFF");
            
            // 设置延时
            task->time = scheduler_get_tick_ms();
            task->state = 8;
            break;
            
        case 8: // 等待全灭延时结束，重新开始
            if (scheduler_get_tick_ms() - task->time >= LED_DELAY_MS) {
                task->state = 1; // 回到状态1重新开始循环
            }
            break;
            
        default:
            task->state = 0; // 重置状态
            break;
    }
}