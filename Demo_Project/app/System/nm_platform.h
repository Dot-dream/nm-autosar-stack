#ifndef NM_PLATFORM_H
#define NM_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

/* ===== NM 模块平台适配 ===== */

/* 临界区保护 — Cortex-M0+ 裸机用全局中断屏蔽 */
#define Nm_EnterCritical()    __disable_irq()
#define Nm_ExitCritical()     __enable_irq()

/* 毫秒级系统 Tick — 复用调度器 */
uint32_t Nm_Timer_GetTick(void);

/* CAN 发送 — 适配 Hal_Can_Send */
#include "../../middleware/NM/CanNm/CanNm.h"
CanNm_ReturnType CanNm_Transmit(uint8_t channel, const uint8_t* pduData, uint8_t pduLength);
void CanNm_ControllerEnable(uint8_t channel);
void CanNm_ControllerDisable(uint8_t channel);

#endif
