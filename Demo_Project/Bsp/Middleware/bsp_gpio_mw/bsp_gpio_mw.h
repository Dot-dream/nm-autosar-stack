#ifndef _BSP_GPIO_MW_H_
#define _BSP_GPIO_MW_H_

#include "bsp_gpio_hal.h"

/* 
 * Middleware 必须实现的接口声明 
 * 这些函数是 HAL 层调用的，应用层不可见
 */
void Mw_Gpio_Hardware_Init(void);
void Mw_Gpio_Write_Impl(Hal_Pin_Id_t pinId, Hal_Gpio_Level_t level);
void Mw_Gpio_Toggle_Impl(Hal_Pin_Id_t pinId);
Hal_Gpio_Level_t Mw_Gpio_Read_Impl(Hal_Pin_Id_t pinId);
void Mw_Gpio_SetDir_Impl(Hal_Pin_Id_t pinId, Hal_Gpio_Dir_t dir);

#endif /* _BSP_GPIO_MW_H_ */