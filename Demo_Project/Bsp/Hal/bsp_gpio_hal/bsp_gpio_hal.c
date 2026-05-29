#include "bsp_gpio_hal.h"
#include "bsp_gpio_mw.h" /* 引用 Middleware 接口 */

void Hal_Gpio_Init(void)
{
    /* 调用中间层初始化 */
    Mw_Gpio_Hardware_Init();
}

void Hal_Gpio_Write(Hal_Pin_Id_t pinId, Hal_Gpio_Level_t level)
{
    if (pinId < HAL_PIN_MAX)
    {
        /* 透传给中间层 */
        Mw_Gpio_Write_Impl(pinId, level);
    }
}

void Hal_Gpio_Toggle(Hal_Pin_Id_t pinId)
{
    if (pinId < HAL_PIN_MAX)
    {
        Mw_Gpio_Toggle_Impl(pinId);
    }
}

Hal_Gpio_Level_t Hal_Gpio_Read(Hal_Pin_Id_t pinId)
{
    if (pinId < HAL_PIN_MAX)
    {
        return Mw_Gpio_Read_Impl(pinId);
    }
    return HAL_LOW;
}

void Hal_Gpio_SetDir(Hal_Pin_Id_t pinId, Hal_Gpio_Dir_t dir)
{
    if (pinId < HAL_PIN_MAX)
    {
        Mw_Gpio_SetDir_Impl(pinId, dir);
    }
}