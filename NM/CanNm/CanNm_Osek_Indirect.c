/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       CanNm_Indirect.c
 *  @brief      OSEK Indirect NM state machine
 *
 *  States: OFF -> INIT -> AWAKE -> NORMAL -> WAITBUSSLEEP -> BUSSLEEP
 *                       \-> LIMPHOME
 *
 *  Timer management delegated to Nm_Timer middleware.
 */
/*============================================================================*/

#include "CanNm.h"
#include "Nm_Internal.h"
#include "Nm_OsIf.h"
#include "Nm_Cbk.h"
#include "Nm_Timer/Nm_Timer.h"

/*=======[ I N T E R N A L ]=================================================*/

typedef struct {
    const Nm_ChannelConfigType* config;
    Nm_ChannelContextType*      nmCtx;

    CanNm_StateType state;
    CanNm_StateType prevState;

    Nm_TimerHandle hToB;      /* App message timeout */
    Nm_TimerHandle hTWbs;     /* Wait bus-sleep */

    uint8  msgReceived;
    uint8  comEnabled;

    Nm_PduType lastRxPdu;
    uint8      lastRxNodeId;
    uint8      rxPduAvailable;

} CanNmOsekIndirect_ChannelType;

static CanNmOsekIndirect_ChannelType g_channels[NM_MAX_CHANNELS];

/*=======[ C O N T E X T ]===================================================*/

static CanNmOsekIndirect_ChannelType* Ind_Ctx(NetworkHandleType ch)
{
    uint8 i;
    for (i = 0; i < NM_MAX_CHANNELS; i++) {
        if (g_channels[i].config != NULL && g_channels[i].config->channelHandle == ch) {
            return &g_channels[i];
        }
    }
    return NULL;
}

/*=======[ S T A T E   C H A N G E ]=========================================*/

static void Ind_ChangeState(CanNmOsekIndirect_ChannelType* ctx, CanNm_StateType ns)
{
    Nm_StateType nmNs;
    if (NULL == ctx) return;
    ctx->prevState = ctx->state;
    ctx->state     = ns;
    nmNs = Nm_MapCanNmStateToNmState(ns);
    Nm_Core_DispatchStateChange(ctx->config->channelHandle, nmNs);
    switch (ns) {
        case CANNM_IND_STATE_NORMAL: Nm_Core_DispatchNetworkMode(ctx->config->channelHandle); break;
        case CANNM_IND_STATE_WAITBUSSLEEP: Nm_Core_DispatchPrepareBusSleep(ctx->config->channelHandle); break;
        case CANNM_IND_STATE_BUSSLEEP: Nm_Core_DispatchBusSleep(ctx->config->channelHandle); break;
        default: break;
    }
}

/*=======[ F S M ]===========================================================*/

static void Ind_FSM(CanNmOsekIndirect_ChannelType* ctx)
{
    if (NULL == ctx) return;

    switch (ctx->state) {
        case CANNM_IND_STATE_OFF: break;
        case CANNM_IND_STATE_INIT:
            Ind_ChangeState(ctx, CANNM_IND_STATE_AWAKE);
            Nm_Timer_Start(ctx->hToB);
            break;
        case CANNM_IND_STATE_AWAKE:
            if (ctx->msgReceived) {
                Ind_ChangeState(ctx, CANNM_IND_STATE_NORMAL);
                Nm_Timer_Start(ctx->hToB);
                ctx->msgReceived = 0U;
            }
            break;
        case CANNM_IND_STATE_NORMAL:
            if (ctx->msgReceived) {
                Nm_Timer_Start(ctx->hToB);
                ctx->msgReceived = 0U;
            } else if (Nm_Timer_IsExpired(ctx->hToB)) {
                Ind_ChangeState(ctx, CANNM_IND_STATE_WAITBUSSLEEP);
                Nm_Timer_Start(ctx->hTWbs);
            }
            break;
        case CANNM_IND_STATE_WAITBUSSLEEP:
            if (ctx->msgReceived) {
                Ind_ChangeState(ctx, CANNM_IND_STATE_NORMAL);
                Nm_Timer_Start(ctx->hToB);
                ctx->msgReceived = 0U;
            } else if (Nm_Timer_IsExpired(ctx->hTWbs)) {
                Ind_ChangeState(ctx, CANNM_IND_STATE_BUSSLEEP);
            }
            break;
        case CANNM_IND_STATE_BUSSLEEP: break;
        case CANNM_IND_STATE_LIMPHOME:
            if (ctx->msgReceived) {
                Ind_ChangeState(ctx, CANNM_IND_STATE_AWAKE);
                Nm_Timer_Start(ctx->hToB);
                ctx->msgReceived = 0U;
            }
            break;
        default: break;
    }
}

/*=======[ P U B L I C   A P I ]=============================================*/

void CanNmOsekIndirect_Init(const Nm_ChannelConfigType* cfg, Nm_ChannelContextType* nmCtx)
{
    uint8 i; CanNmOsekIndirect_ChannelType* ctx = NULL;
    if (NULL == cfg || NULL == nmCtx) return;
    for (i = 0; i < NM_MAX_CHANNELS; i++) {
        if (g_channels[i].config == NULL) { ctx = &g_channels[i]; break; }
    }
    if (NULL == ctx) return;
    ctx->config = cfg; ctx->nmCtx = nmCtx;
    ctx->state = CANNM_IND_STATE_OFF; ctx->prevState = CANNM_IND_STATE_OFF;
    ctx->comEnabled = 1U; ctx->msgReceived = 0U;
    ctx->hToB  = Nm_Timer_Alloc(cfg->timerMax, NM_TIMER_MODE_ONESHOT, NULL, NULL);
    ctx->hTWbs = Nm_Timer_Alloc(cfg->timerWaitBusSleep, NM_TIMER_MODE_ONESHOT, NULL, NULL);
        /* Stay at OFF - NetworkRequest triggers FSM */
}

void CanNmOsekIndirect_DeInit(NetworkHandleType ch) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch);
    if (NULL == ctx) return;
    Nm_Timer_Free(ctx->hToB); Nm_Timer_Free(ctx->hTWbs); ctx->config = NULL;
}
CanNm_ReturnType CanNmOsekIndirect_PassiveStartUp(NetworkHandleType ch) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch); if (NULL == ctx) return CANNM_E_NOT_OK;
    Ind_ChangeState(ctx, CANNM_IND_STATE_AWAKE); return CANNM_E_OK;
}
CanNm_ReturnType CanNmOsekIndirect_NetworkRequest(NetworkHandleType ch) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch); if (NULL == ctx) return CANNM_E_NOT_OK;
    Ind_ChangeState(ctx, CANNM_IND_STATE_AWAKE); return CANNM_E_OK;
}
CanNm_ReturnType CanNmOsekIndirect_NetworkRelease(NetworkHandleType ch) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch); if (NULL == ctx) return CANNM_E_NOT_OK;
    Nm_Timer_Start(ctx->hTWbs); Ind_ChangeState(ctx, CANNM_IND_STATE_WAITBUSSLEEP); return CANNM_E_OK;
}
void CanNmOsekIndirect_DisableCommunication(NetworkHandleType ch) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch); if (NULL != ctx) ctx->comEnabled = 0U;
}
void CanNmOsekIndirect_EnableCommunication(NetworkHandleType ch) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch); if (NULL != ctx) ctx->comEnabled = 1U;
}
void CanNmOsekIndirect_SetUserData(NetworkHandleType ch, const uint8* d, uint8 n) { (void)ch;(void)d;(void)n; }
void CanNmOsekIndirect_GetUserData(NetworkHandleType ch, uint8* d, uint8* nid) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch); uint8 i, dl;
    if (NULL == ctx || NULL == d || NULL == nid) return;
    dl = ctx->lastRxPdu.length; if (dl > 2U) dl -= 2U; else dl = 0U;
    if (dl > NM_USER_DATA_MAX) dl = NM_USER_DATA_MAX;
    for (i = 0; i < dl; i++) d[i] = ctx->lastRxPdu.data[i + 2U]; *nid = ctx->lastRxNodeId;
}
void CanNmOsekIndirect_GetPduData(NetworkHandleType ch, uint8* pdu) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch); uint8 i;
    if (NULL == ctx || NULL == pdu) return;
    for (i = 0; i < ctx->lastRxPdu.length && i < NM_PDU_MAX_LENGTH; i++) pdu[i] = ctx->lastRxPdu.data[i];
}
uint8 CanNmOsekIndirect_GetNodeIdentifier(NetworkHandleType ch) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch); return ctx ? ctx->lastRxNodeId : NM_INVALID_NODE_ID;
}
uint8 CanNmOsekIndirect_GetLocalNodeIdentifier(NetworkHandleType ch) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch); return ctx ? ctx->config->nodeId : NM_INVALID_NODE_ID;
}
boolean CanNmOsekIndirect_CheckRemoteSleepIndication(NetworkHandleType ch) { (void)ch; return FALSE; }
void CanNmOsekIndirect_GetState(NetworkHandleType ch, Nm_StateType* s, Nm_ModeType* m) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch);
    if (NULL == ctx) { *s = NM_STATE_UNINIT; *m = NM_MODE_BUS_SLEEP; return; }
    *s = Nm_MapCanNmStateToNmState(ctx->state);
    switch (ctx->state) {
        case CANNM_IND_STATE_NORMAL: case CANNM_IND_STATE_AWAKE: *m = NM_MODE_NETWORK; break;
        case CANNM_IND_STATE_WAITBUSSLEEP: *m = NM_MODE_PREPARE_BUS_SLEEP; break;
        case CANNM_IND_STATE_BUSSLEEP: *m = NM_MODE_BUS_SLEEP; break;
        default: *m = NM_MODE_BUS_SLEEP; break;
    }
}
void CanNmOsekIndirect_MainFunction(NetworkHandleType ch) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch); if (NULL != ctx) Ind_FSM(ctx);
}
void CanNmOsekIndirect_TxConfirmation(NetworkHandleType ch) { (void)ch; }
void CanNmOsekIndirect_RxIndication(NetworkHandleType ch, const uint8* d, uint8 n) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch); uint8 i;
    if (NULL == ctx || NULL == d || 0U == n) return;
    for (i = 0; i < n && i < NM_PDU_MAX_LENGTH; i++) ctx->lastRxPdu.data[i] = d[i];
    ctx->lastRxPdu.length = n; ctx->lastRxNodeId = (n >= 2U) ? d[1] : 0U;
    ctx->rxPduAvailable = 1U; ctx->msgReceived = 1U;
    if (CANNM_IND_STATE_BUSSLEEP == ctx->state) {
        Ind_ChangeState(ctx, CANNM_IND_STATE_AWAKE); Nm_Core_DispatchNetworkStart(ch);
    }
}
void CanNmOsekIndirect_ControllerBusOff(NetworkHandleType ch) {
    CanNmOsekIndirect_ChannelType* ctx = Ind_Ctx(ch); if (NULL == ctx) return;
    Ind_ChangeState(ctx, CANNM_IND_STATE_LIMPHOME);
}
