#include "bsp_can_hal.h"
#include "bsp_can_mw.h"

void Hal_Can_Init(void)
{
    Mw_Can_Init();
}

int Hal_Can_RegisterRx(uint32_t can_id, Hal_Can_RxCallback_t cb)
{
    return Mw_Can_RegisterRx(can_id, cb);
}

bool Hal_Can_Send(uint32_t can_id, const uint8_t *data, uint8_t len)
{
    return Mw_Can_Send(can_id, data, len);
}

void Hal_Can_Poll(void)
{
    Mw_Can_Poll();
}
