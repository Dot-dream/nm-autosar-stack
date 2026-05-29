/*
 * Copyright 2020-2026 Yuntu Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * @file pin_mux.c
 * @brief 
 * 
 */



#include "pin_mux.h"

/*! @brief User configuration cfg InitConfigArr0 array */
const pin_settings_config_t g_pin_mux_InitConfigArr0[NUM_OF_CONFIGURED_PINS0] = {
    /* PTE_6-53-GPIO- */
    {
        .base = PCTRLE,
        .pinPortIdx = 6U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_AS_GPIO,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOE,
        .direction = GPIO_OUTPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTC_7-51-CAN1_TX- */
    {
        .base = PCTRLC,
        .pinPortIdx = 7U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_ALT3,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOC,
        .direction = GPIO_INPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTC_6-52-CAN1_RX- */
    {
        .base = PCTRLC,
        .pinPortIdx = 6U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_ALT3,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOC,
        .direction = GPIO_INPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTD_2-46-GPIO- */
    {
        .base = PCTRLD,
        .pinPortIdx = 2U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_AS_GPIO,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOD,
        .direction = GPIO_OUTPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTA_3-47-UART0_TX- */
    {
        .base = PCTRLA,
        .pinPortIdx = 3U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_ALT6,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOA,
        .direction = GPIO_INPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTA_2-48-UART0_RX- */
    {
        .base = PCTRLA,
        .pinPortIdx = 2U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_ALT6,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOA,
        .direction = GPIO_INPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTA_7-37-UART1_RTS- */
    {
        .base = PCTRLA,
        .pinPortIdx = 7U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_ALT5,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOA,
        .direction = GPIO_INPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTC_8-36-UART1_RX- */
    {
        .base = PCTRLC,
        .pinPortIdx = 8U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_ALT2,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOC,
        .direction = GPIO_INPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTC_9-35-UART1_TX- */
    {
        .base = PCTRLC,
        .pinPortIdx = 9U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_ALT2,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOC,
        .direction = GPIO_INPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTD_5-24-GPIO- */
    {
        .base = PCTRLD,
        .pinPortIdx = 5U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_AS_GPIO,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOD,
        .direction = GPIO_OUTPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTD_6-23-GPIO- */
    {
        .base = PCTRLD,
        .pinPortIdx = 6U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_AS_GPIO,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOD,
        .direction = GPIO_OUTPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTD_7-22-GPIO- */
    {
        .base = PCTRLD,
        .pinPortIdx = 7U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_AS_GPIO,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOD,
        .direction = GPIO_OUTPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTA_0-50-MPWM0_CH7- */
    {
        .base = PCTRLA,
        .pinPortIdx = 0U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_ALT2,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOA,
        .direction = GPIO_INPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTE_1-59-GPIO- */
    {
        .base = PCTRLE,
        .pinPortIdx = 1U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_AS_GPIO,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOE,
        .direction = GPIO_INPUT_DIRECTION,
        .initValue = 0,
    },
    /* PTE_0-60-GPIO- */
    {
        .base = PCTRLE,
        .pinPortIdx = 0U,
        .pullConfig = PCTRL_INTERNAL_PULL_NOT_ENABLED,
        .passiveFilter = false,
        .driveSelect = PCTRL_LOW_DRIVE_STRENGTH,
        .mux = PCTRL_MUX_AS_GPIO,
        .intConfig = PCTRL_DMA_INT_DISABLED,
        .clearIntFlag = false,
        .digitalFilter = false,
        .gpioBase = GPIOE,
        .direction = GPIO_INPUT_DIRECTION,
        .initValue = 0,
    },
};



/***********************************************************************************************************************
 * EOF
 **********************************************************************************************************************/

