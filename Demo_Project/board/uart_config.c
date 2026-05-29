/*
 * Copyright 2020-2026 Yuntu Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * @file uart_config.c
 * @brief 
 * 
 */



#include "uart_config.h"


/*uart_config1*/
uart_state_t uart_config1_State;
uart_user_config_t uart_config1 = {
    .baudRate=115200U,
    .parityMode=UART_PARITY_DISABLED,
    .stopBitCount=UART_ONE_STOP_BIT,
    .bitCountPerChar=UART_8_BITS_PER_CHAR,
    .transferType=UART_USING_DMA,
    .rxDMAChannel=0,
    .txDMAChannel=1,
    .idleErrorIntEnable=false,
};

