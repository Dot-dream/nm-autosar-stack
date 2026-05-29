#ifndef NM_LCFG_H
#define NM_LCFG_H

#include <stdint.h>

/* ===== 测试模式选择 =====
 * 0 = OSEK 直接 NM (Alive/Ring/LimpHome)
 * 1 = OSEK 间接 NM (应用消息监控)
 * 2 = AUTOSAR NM    (CBV 广播 + 协调器)
 */
#define NM_TEST_MODE  2

#include "Nm_ConfigTypes.h"

extern const Nm_ConfigType NmConfig;
extern void Nm_Lcfg_PrintInfo(void);

#endif
