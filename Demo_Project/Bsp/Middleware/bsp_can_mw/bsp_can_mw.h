#ifndef _BSP_CAN_MW_H_
#define _BSP_CAN_MW_H_

#include "bsp_can_hal.h"

/* Middleware 内部接口（仅 HAL 层调用） */
void Mw_Can_Init(void);
int  Mw_Can_RegisterRx(uint32_t can_id, Hal_Can_RxCallback_t cb);
bool Mw_Can_Send(uint32_t can_id, const uint8_t *data, uint8_t len);
void Mw_Can_Poll(void);

#endif /* _BSP_CAN_MW_H_ */
