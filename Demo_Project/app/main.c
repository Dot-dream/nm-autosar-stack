/* USER CODE BEGIN Header */
/* you can remove the copyright */
/*
 * Copyright 2020-2025 Yuntu Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * @file main.c
 * @brief 主程序入口
 * 
 */

/* USER CODE END Header */
#include "sdk_project_config.h"
/* Includes ------------------------------------------------------------------*/
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// BSP层
#include "log_port.h"
#include "bsp_gpio_hal.h"
#include "lptmr_driver.h"

// 系统模块
#include "scheduler.h"
#include "scheduler_port.h"
#include "module_led.h"
#include "module_log.h"
#include "module_event.h"
#include "module_nm.h"
#include "nm_platform.h"

// 配置文件
#include "task_config.h"

// 标准库头文件
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LPTMR_INST 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
volatile uint32_t g_tick_count = 0;
static uint32_t last_event_time = 0;
/* USER CODE END PV */

/* Private function declare --------------------------------------------------*/
/* USER CODE BEGIN PFDC */
static void Board_Init(void);

void scheduler_increment_tick(void)
{
}

uint32_t scheduler_get_tick(void)
{
    return g_tick_count;
}

uint32_t scheduler_get_tick_ms(void)
{
    return g_tick_count;
}
/* USER CODE END PFDC */
static void Board_Init(void);

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void lpTMR0_IRQHandler(void)
{
    lpTMR_DRV_ClearCompareFlag(0);
    ++g_tick_count;
    
#if TASK_CONFIG_ENABLE_SCHEDULER
    scheduler_increment_tick();
#endif
}
/* USER CODE END 0 */


/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */ 
    Board_Init();
    /* USER CODE BEGIN 2 */

    // 初始化硬件定时器
    INT_SYS_EnableIRQ(lpTMR0_IRQn);
    lpTMR_DRV_StartCounter(LPTMR_INST);

    // 初始化日志系统
    log_init(LOG_INFO);
    log_info("=== MC03 项目启动 ===");
    
    log_set_level(LOG_DEBUG);
    
    int current_level = log_port_get_level();
    log_info("当前日志级别: %d", current_level);
    
#if TASK_CONFIG_ENABLE_SCHEDULER
    scheduler_init();
    log_info("调度器初始化完成");
    
    log_module_init();
    led_task_init();
    log_periodic_task_init();
    event_demo_task_init();
    nm_module_init();
    nm_task_init();
    
    scheduler_start();
    log_info("所有任务模块初始化完成，开始调度");
    
    last_event_time = scheduler_get_tick_ms();
#else
    log_info("使用原始系统（无任务调度器）");
#endif
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */
        /* USER CODE BEGIN 3 */
        
#if TASK_CONFIG_ENABLE_SCHEDULER
        scheduler_run_once();
        
        if (scheduler_get_tick_ms() - last_event_time > 5000) {
            event_trigger_button_press();
            last_event_time = scheduler_get_tick_ms();
        }
        
        OSIF_TimeDelay(1);
#else
        OSIF_TimeDelay(100);
        static bool flag = true;
        if(flag) {
            flag = false;
            Hal_Gpio_Write(HAL_PIN_GPIO_D5,HAL_HIGH);
            Hal_Gpio_Write(HAL_PIN_GPIO_D6,HAL_HIGH);
            Hal_Gpio_Write(HAL_PIN_GPIO_D7,HAL_HIGH);
        } else {
            flag = true;
            Hal_Gpio_Write(HAL_PIN_GPIO_D5,HAL_LOW);
            Hal_Gpio_Write(HAL_PIN_GPIO_D6,HAL_LOW);
            Hal_Gpio_Write(HAL_PIN_GPIO_D7,HAL_LOW);
        }
        log_debug("Test debug log function");
        log_error("Test error log function");
        log_info("Test info log function");
#endif
        /* USER CODE END 3 */
    }
    /* USER CODE END WHILE */
        /* USER CODE BEGIN 3 */
        
#if TASK_CONFIG_ENABLE_SCHEDULER
        scheduler_run_once();
        
        if (scheduler_get_tick_ms() - last_event_time > 5000) {
            event_trigger_button_press();
            last_event_time = scheduler_get_tick_ms();
        }
        
        OSIF_TimeDelay(1);
#else
        OSIF_TimeDelay(100);
        static bool flag = true;
        if(flag) {
            flag = false;
            Hal_Gpio_Write(HAL_PIN_GPIO_D5,HAL_HIGH);
            Hal_Gpio_Write(HAL_PIN_GPIO_D6,HAL_HIGH);
            Hal_Gpio_Write(HAL_PIN_GPIO_D7,HAL_HIGH);
        } else {
            flag = true;
            Hal_Gpio_Write(HAL_PIN_GPIO_D5,HAL_LOW);
            Hal_Gpio_Write(HAL_PIN_GPIO_D6,HAL_LOW);
            Hal_Gpio_Write(HAL_PIN_GPIO_D7,HAL_LOW);
        }
        log_debug("Test debug log function");
        log_error("Test error log function");
        log_info("Test info log function");
#endif
        /* USER CODE END 3 */
}

static void Board_Init(void)
{
    CLOCK_SYS_Init(g_clockManConfigsArr,CLOCK_MANAGER_CONFIG_CNT,g_clockManCallbacksArr,CLOCK_MANAGER_CALLBACK_CNT);
    CLOCK_SYS_UpdateConfiguration(CLOCK_MANAGER_ACTIVE_INDEX,CLOCK_MANAGER_POLICY_AGREEMENT);
    PINS_DRV_Init(NUM_OF_CONFIGURED_PINS0,g_pin_mux_InitConfigArr0);
    DMA_DRV_Init(&dmaState,&dmaController_InitConfig,dmaChnState,dmaChnConfigArray,NUM_OF_CONFIGURED_DMA_CHANNEL);
    UTILITY_PRINT_Init();
    LIN_DRV_Init(0,&lin_config0,&lin_config0_State);
    MPWM_DRV_Init(0,&MPWM_State0);
    MPWM_DRV_SetClkSrc(0,MPWM_BUS_CLOCK);
    lpTMR_DRV_Init(0,&LPTMR_Config,false);
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */
