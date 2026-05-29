#ifndef _BSP_GPIO_HAL_H_
#define _BSP_GPIO_HAL_H_

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * 1. 标准类型定义
 * ========================================================================= */

/* 逻辑电平 */
typedef enum {
    HAL_LOW  = 0U,
    HAL_HIGH = 1U
} Hal_Gpio_Level_t;

/* 引脚方向 */
typedef enum {
    HAL_INPUT  = 0U,
    HAL_OUTPUT = 1U
} Hal_Gpio_Dir_t;

/* 
 * 逻辑引脚 ID 
 * 应用层只认这些 ID，完全不知道物理引脚是 PA3 还是 PD2
 */
typedef enum {
    HAL_PIN_GPIO_E6 = 0,
    HAL_PIN_CAN1_TX,
    HAL_PIN_CAN1_RX,
    HAL_PIN_GPIO_D2,
    HAL_PIN_UART0_TX,
    HAL_PIN_UART0_RX,
    HAL_PIN_UART1_RTS,
    HAL_PIN_UART1_RX,
    HAL_PIN_UART1_TX,
    HAL_PIN_GPIO_D5,
    HAL_PIN_GPIO_D6,
    HAL_PIN_GPIO_D7,
    HAL_PIN_PWM_A0,
    HAL_PIN_GPIO_E1,
    HAL_PIN_GPIO_E0,
    
    HAL_PIN_MAX /* 哨兵，用于检查边界 */
} Hal_Pin_Id_t;

/* =========================================================================
 * 2. 标准 API 接口 (供 APP 调用)
 * ========================================================================= */

void Hal_Gpio_Init(void);
void Hal_Gpio_Write(Hal_Pin_Id_t pinId, Hal_Gpio_Level_t level);
void Hal_Gpio_Toggle(Hal_Pin_Id_t pinId);
Hal_Gpio_Level_t Hal_Gpio_Read(Hal_Pin_Id_t pinId);
void Hal_Gpio_SetDir(Hal_Pin_Id_t pinId, Hal_Gpio_Dir_t dir);

#endif /* _BSP_GPIO_HAL_H_ */