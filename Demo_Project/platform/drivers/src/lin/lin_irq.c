/*
 * Copyright 2020-2022 Yuntu Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*!
 * @file lin_irq.c
 * @version 1.4.0
 */

/*!
 * @page misra_violations MISRA-C:2012 violations list
 *
 * PRQA S 1533 Rule 8.9: The object 'g_linUartIsrs' is only referenced by function 'LIN_UART_DRV_Init'.
 *
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "lin_driver.h"

/*******************************************************************************
 * Code
 ******************************************************************************/
#if (UART_INSTANCE_COUNT > 0U)
static void LIN_UART0_RxTx_IRQHandler(void)
{
    LIN_DRV_IRQHandler(0U);
}

#ifdef UART_ERR_IRQS
static void LIN_UART0_ERR_IRQHandler(void)
{
    LIN_DRV_IRQHandler(0U);
}
#endif
#endif /* if (UART_INSTANCE_COUNT > 0U) */

#if (UART_INSTANCE_COUNT > 1U)
static void LIN_UART1_RxTx_IRQHandler(void)
{
    LIN_DRV_IRQHandler(1U);
}

#ifdef UART_ERR_IRQS
static void LIN_UART1_ERR_IRQHandler(void)
{
    LIN_DRV_IRQHandler(1U);
}
#endif
#endif /* if (UART_INSTANCE_COUNT > 1U) */

#if (UART_INSTANCE_COUNT > 2U)
static void LIN_UART2_RxTx_IRQHandler(void)
{
    LIN_DRV_IRQHandler(2U);
}

#ifdef UART_ERR_IRQS
static void LIN_UART2_ERR_IRQHandler(void)
{
    LIN_DRV_IRQHandler(2U);
}
#endif
#endif /* if (UART_INSTANCE_COUNT > 2U) */

#if (UART_INSTANCE_COUNT > 3U)
static void LIN_UART3_RxTx_IRQHandler(void)
{
    LIN_DRV_IRQHandler(3U);
}

#ifdef UART_ERR_IRQS
static void LIN_UART3_ERR_IRQHandler(void)
{
    LIN_DRV_IRQHandler(3U);
}
#endif
#endif /* if (UART_INSTANCE_COUNT > 3U) */

#if (UART_INSTANCE_COUNT > 4U)
static void LIN_UART4_RxTx_IRQHandler(void)
{
    LIN_DRV_IRQHandler(4U);
}

#ifdef UART_ERR_IRQS
static void LIN_UART4_ERR_IRQHandler(void)
{
    LIN_DRV_IRQHandler(4U);
}
#endif
#endif /* if (UART_INSTANCE_COUNT > 4U) */

#if (UART_INSTANCE_COUNT > 5U)
static void LIN_UART5_RxTx_IRQHandler(void)
{
    LIN_DRV_IRQHandler(5U);
}

#ifdef UART_ERR_IRQS
static void LIN_UART5_ERR_IRQHandler(void)
{
    LIN_DRV_IRQHandler(5U);
}
#endif
#endif /* if (UART_INSTANCE_COUNT > 2U) */

isr_t g_linUartIsrs[UART_INSTANCE_COUNT] = /*PRQA S 1533*/
    {
#if (UART_INSTANCE_COUNT > 0U)
        LIN_UART0_RxTx_IRQHandler,
#endif
#if (UART_INSTANCE_COUNT > 1U)
        LIN_UART1_RxTx_IRQHandler,
#endif
#if (UART_INSTANCE_COUNT > 2U)
        LIN_UART2_RxTx_IRQHandler,
#endif
#if (UART_INSTANCE_COUNT > 3U)
        LIN_UART3_RxTx_IRQHandler,
#endif
#if (UART_INSTANCE_COUNT > 4U)
        LIN_UART4_RxTx_IRQHandler,
#endif
#if (UART_INSTANCE_COUNT > 5U)
        LIN_UART5_RxTx_IRQHandler,
#endif
    };

#ifdef UART_ERR_IRQS
isr_t g_linUartErrIsrs[UART_INSTANCE_COUNT] =
{
#if (UART_INSTANCE_COUNT > 0U)
    LIN_UART0_ERR_IRQHandler,
#endif
#if (UART_INSTANCE_COUNT > 1U)
    LIN_UART1_ERR_IRQHandler,
#endif
#if (UART_INSTANCE_COUNT > 2U)
    LIN_UART2_ERR_IRQHandler,
#endif
#if (UART_INSTANCE_COUNT > 3U)
    LIN_UART3_ERR_IRQHandler,
#endif
#if (UART_INSTANCE_COUNT > 4U)
    LIN_UART4_ERR_IRQHandler,
#endif
#if (UART_INSTANCE_COUNT > 5U)
    LIN_UART5_ERR_IRQHandler,
#endif
};
#endif
/******************************************************************************/
/* EOF */
/******************************************************************************/
