#include "bsp_gpio_mw.h"

/* 引入厂商驱动头文件 */
#include "pin_mux.h"
#include "pins_driver.h"

/* 
 * 物理引脚映射表结构体 
 * 这是 Middleware 私有的，HAL 不需要知道
 */
typedef struct {
    GPIO_Type * portBase;
    uint32_t    pinNum;
} Mw_Gpio_Map_t;

/* 
 * 映射表实现
 * 顺序必须严格对应 Hal_Pin_Id_t 枚举
 */
static const Mw_Gpio_Map_t g_mw_gpio_map[HAL_PIN_MAX] = {
    {GPIOE, 6U}, /* HAL_PIN_GPIO_E6      */
    {GPIOC, 7U}, /* HAL_PIN_CAN1_TX      */
    {GPIOC, 6U}, /* HAL_PIN_CAN1_RX      */
    {GPIOD, 2U}, /* HAL_PIN_GPIO_D2      */
    {GPIOA, 3U}, /* HAL_PIN_UART0_TX     */
    {GPIOA, 2U}, /* HAL_PIN_UART0_RX     */
    {GPIOA, 7U}, /* HAL_PIN_UART1_RTS    */
    {GPIOC, 8U}, /* HAL_PIN_UART1_RX     */
    {GPIOC, 9U}, /* HAL_PIN_UART1_TX     */
    {GPIOD, 5U}, /* HAL_PIN_GPIO_D5      */
    {GPIOD, 6U}, /* HAL_PIN_GPIO_D6      */
    {GPIOD, 7U}, /* HAL_PIN_GPIO_D7      */
    {GPIOA, 0U}, /* HAL_PIN_PWM_A0       */
    {GPIOE, 1U}, /* HAL_PIN_GPIO_E1      */
    {GPIOE, 0U}, /* HAL_PIN_GPIO_E0      */
};

/* =========================================================================
 * 接口实现
 * ========================================================================= */

void Mw_Gpio_Hardware_Init(void)
{
    /* 调用 Yuntu SDK 提供的初始化接口 */
    PINS_DRV_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);
}

void Mw_Gpio_Write_Impl(Hal_Pin_Id_t pinId, Hal_Gpio_Level_t level)
{
    const Mw_Gpio_Map_t *map = &g_mw_gpio_map[pinId];
    
    /* 转换 HAL 电平到 Yuntu SDK 电平 */
    pins_level_type_t hw_level = (level == HAL_HIGH) ? 1U : 0U;
    
    PINS_DRV_WritePin(map->portBase, map->pinNum, hw_level);
}

void Mw_Gpio_Toggle_Impl(Hal_Pin_Id_t pinId)
{
    const Mw_Gpio_Map_t *map = &g_mw_gpio_map[pinId];
    
    /* Yuntu SDK Toggle 需要引脚掩码 */
    uint32_t pin_mask = (1UL << map->pinNum);
    
    PINS_DRV_TogglePins(map->portBase, pin_mask);
}

Hal_Gpio_Level_t Mw_Gpio_Read_Impl(Hal_Pin_Id_t pinId)
{
    const Mw_Gpio_Map_t *map = &g_mw_gpio_map[pinId];
    
    pins_level_type_t val = PINS_DRV_ReadPin(map->portBase, map->pinNum);
    
    return (val != 0U) ? HAL_HIGH : HAL_LOW;
}

void Mw_Gpio_SetDir_Impl(Hal_Pin_Id_t pinId, Hal_Gpio_Dir_t dir)
{
    const Mw_Gpio_Map_t *map = &g_mw_gpio_map[pinId];
    
    pin_direction_t hw_dir = (dir == HAL_OUTPUT) ? GPIO_OUTPUT_DIRECTION : GPIO_INPUT_DIRECTION;
    
    PINS_DRV_SetPinDirection(map->portBase, map->pinNum, hw_dir);
}