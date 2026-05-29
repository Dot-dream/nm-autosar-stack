/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       CanNm_Direct.c
 *  @brief      OSEK Direct NM state machine
 *
 *  States: OFF -> INIT -> INITRESET -> NORMAL -> NORMALPREPSLEEP -> TWBS_NORMAL -> BUSSLEEP
 *                               \-> LIMPHOME -> LIMPHOMEPREPSLEEP -> TWBS_LIMPHOME -> BUSSLEEP
 *
 *  Timer management delegated to Nm_Timer middleware.
 */
/*============================================================================*/

#include "CanNm.h"
#include "Nm_Internal.h"
#include "Nm_OsIf.h"
#include "Nm_Cbk.h"
#include "Nm_Timer/Nm_Timer.h"

/*=======[ I N T E R N A L   C H A N N E L   S T A T E ]====================*/

typedef struct {
    const Nm_ChannelConfigType* config;
    Nm_ChannelContextType*      nmCtx;

    CanNm_StateType state;
    CanNm_StateType prevState;

    /* Timer handles (allocated at Init, freed at DeInit) */
    Nm_TimerHandle hTTyp;
    Nm_TimerHandle hTMax;
    Nm_TimerHandle hTError;
    Nm_TimerHandle hTWbs;
    Nm_TimerHandle hTTx;

    uint8  txCounter;
    uint8  rxCounter;
    uint8  sleepIndication;
    uint8  immediateTxConf;
    uint8  txPending;
    uint8  comEnabled;
    uint8  repeatMsgRequested;
    uint8  busLoadReduction;

    uint8  txUserData[NM_USER_DATA_MAX];
    uint8  txUserDataLen;

    Nm_PduType lastRxPdu;
    uint8      lastRxNodeId;
    uint8      rxPduAvailable;

} CanNmOsekDirect_ChannelType;

/*=======[ G L O B A L S ]===================================================*/

static CanNmOsekDirect_ChannelType g_channels[NM_MAX_CHANNELS];

/*=======[ C O N T E X T ]===================================================*/

static CanNmOsekDirect_ChannelType* Direct_Ctx(NetworkHandleType ch)
{
    uint8 i;
    for (i = 0; i < NM_MAX_CHANNELS; i++) {
        if (g_channels[i].config != NULL
            && g_channels[i].config->channelHandle == ch) {
            return &g_channels[i];
        }
    }
    return NULL;
}

/*=======[ S T A T E   C H A N G E ]=========================================*/

static void Direct_ChangeState(CanNmOsekDirect_ChannelType* ctx, CanNm_StateType ns)
{
    Nm_StateType nmNs;
    if (NULL == ctx) { return; }
    ctx->prevState = ctx->state;
    ctx->state     = ns;

    nmNs = Nm_MapCanNmStateToNmState(ns);
    Nm_Core_DispatchStateChange(ctx->config->channelHandle, nmNs);

    switch (ns) {
        case CANNM_STATE_NORMAL:
            Nm_Core_DispatchNetworkMode(ctx->config->channelHandle);
            break;
        case CANNM_STATE_NORMALPREPSLEEP:
            Nm_Core_DispatchPrepareBusSleep(ctx->config->channelHandle);
            break;
        case CANNM_STATE_BUSSLEEP:
            Nm_Core_DispatchBusSleep(ctx->config->channelHandle);
            break;
        case CANNM_STATE_INITRESET:
            /* Start Alive transmit cycle */
            Nm_Timer_Start(ctx->hTTyp);
            break;
        default: break;
    }
}

/*=======[ P D U   T X ]=====================================================*/

static void Direct_SendPdu(CanNmOsekDirect_ChannelType* ctx)
{
    uint8 pdu[NM_PDU_MAX_LENGTH];
    uint8 i;

    if (NULL == ctx || 0U == ctx->comEnabled) { return; }
    for (i = 0; i < NM_PDU_MAX_LENGTH; i++) { pdu[i] = 0x00U; }

    if (NM_WIRE_FORMAT_CAN_ID == ctx->config->wireConfig.wireFormat) {
        if (ctx->state == CANNM_STATE_NORMAL || ctx->state == CANNM_STATE_TWBS_NORMAL) {
            pdu[0] = ctx->config->wireConfig.pduOpCodeRing;
        } else if (ctx->state >= CANNM_STATE_LIMPHOME) {
            pdu[0] = ctx->config->wireConfig.pduOpCodeLimpHome;
        } else {
            pdu[0] = ctx->config->wireConfig.pduOpCodeAlive;
        }
    } else {
        if (ctx->state == CANNM_STATE_NORMAL || ctx->state == CANNM_STATE_TWBS_NORMAL) {
            pdu[0] = NM_OP_RING_BIT;
            if (ctx->repeatMsgRequested) { pdu[0] |= NM_OP_REPEAT_MSG_BIT; }
        } else if (ctx->state >= CANNM_STATE_LIMPHOME) {
            pdu[0] = NM_OP_LIMPHOME_BIT;
        } else {
            pdu[0] = NM_OP_ALIVE_BIT;
        }
        pdu[0] |= NM_OP_ACTIVE_BIT;
    }

    pdu[1] = ctx->config->nodeId;
    for (i = 0; i < ctx->txUserDataLen && i < (NM_PDU_MAX_LENGTH - 2U); i++) {
        pdu[i + 2U] = ctx->txUserData[i];
    }
    ctx->txPending = 1U;
    ctx->txCounter++;
    (void)CanNm_Transmit(ctx->config->channelHandle, pdu, NM_PDU_MAX_LENGTH);
}

/*=======[ P D U   R X ]=====================================================*/

static void Direct_ProcessRx(CanNmOsekDirect_ChannelType* ctx, const uint8* data, uint8 len)
{
    uint8 i;
    if (NULL == ctx || NULL == data || len < 2U) { return; }

    for (i = 0; i < len && i < NM_PDU_MAX_LENGTH; i++) {
        ctx->lastRxPdu.data[i] = data[i];
    }
    ctx->lastRxPdu.length = len;
    ctx->lastRxNodeId     = data[1];
    ctx->rxPduAvailable   = 1U;
    ctx->rxCounter++;

    /* Any NM message resets TMax */
    Nm_Timer_Start(ctx->hTMax);

    /* Wake-up from Bus-Sleep */
    if (CANNM_STATE_BUSSLEEP == ctx->state) {
            Direct_ChangeState(ctx, CANNM_STATE_INITRESET);
        Nm_Core_DispatchNetworkStart(ctx->config->channelHandle);
    }
}

/*=======[ S T A T E   M A C H I N E ]=======================================*/

static void Direct_FSM(CanNmOsekDirect_ChannelType* ctx)
{
    if (NULL == ctx) { return; }

    switch (ctx->state) {

        case CANNM_STATE_OFF:
            break;

        case CANNM_STATE_INIT:
                Direct_ChangeState(ctx, CANNM_STATE_INITRESET);
            Direct_SendPdu(ctx);   /* Send Alive */
            break;

        case CANNM_STATE_INITRESET:
            if (Nm_Timer_IsExpired(ctx->hTTyp)) {
                Direct_ChangeState(ctx, CANNM_STATE_NORMAL);
                Nm_Timer_Start(ctx->hTTyp);   /* Restart cycle timer */
                Nm_Timer_Start(ctx->hTMax);   /* Start receive watchdog */
                Direct_SendPdu(ctx);          /* Send Ring */
            }
            break;

        case CANNM_STATE_NORMAL:
        case CANNM_STATE_TWBS_NORMAL:
            if (Nm_Timer_IsExpired(ctx->hTMax)) {
                Direct_ChangeState(ctx, CANNM_STATE_LIMPHOME);
                Nm_Timer_Start(ctx->hTError);
                Direct_SendPdu(ctx);
                break;
            }
            if (Nm_Timer_IsExpired(ctx->hTTyp)) {
                Nm_Timer_Start(ctx->hTTyp);
                Direct_SendPdu(ctx);
            }
            break;

        case CANNM_STATE_NORMALPREPSLEEP:
            if (Nm_Timer_IsExpired(ctx->hTWbs)) {
                Direct_ChangeState(ctx, CANNM_STATE_BUSSLEEP);
                CanNm_ControllerDisable(ctx->config->channelHandle);
            }
            break;

        case CANNM_STATE_BUSSLEEP:
            break;

        case CANNM_STATE_LIMPHOME:
        case CANNM_STATE_LIMPHOMEPREPSLEEP:
        case CANNM_STATE_TWBS_LIMPHOME:
            if (Nm_Timer_IsExpired(ctx->hTError)) {
                Nm_Timer_Start(ctx->hTError);
                Direct_SendPdu(ctx);   /* LimpHome message */
            }
            /* Recover if TMax is not expired (= receiving messages) */
            if (!Nm_Timer_IsExpired(ctx->hTMax)) {
                    Direct_ChangeState(ctx, CANNM_STATE_INIT);
            }
            break;

        default: break;
    }
}

/*=======[ P U B L I C   A P I ]=============================================*/

void CanNmOsekDirect_Init(const Nm_ChannelConfigType* cfg, Nm_ChannelContextType* nmCtx)
{
    uint8 i;
    CanNmOsekDirect_ChannelType* ctx = NULL;

    if (NULL == cfg || NULL == nmCtx) { return; }
    for (i = 0; i < NM_MAX_CHANNELS; i++) {
        if (g_channels[i].config == NULL) { ctx = &g_channels[i]; break; }
    }
    if (NULL == ctx) { return; }

    ctx->config   = cfg;
    ctx->nmCtx    = nmCtx;
    ctx->state    = CANNM_STATE_OFF;
    ctx->prevState= CANNM_STATE_OFF;
    ctx->comEnabled = 1U;
    ctx->txCounter  = 0U;
    ctx->rxCounter  = 0U;

    /* Alloc timers from Nm_Timer middleware */
    ctx->hTTyp   = Nm_Timer_Alloc(cfg->timerTyp,          NM_TIMER_MODE_PERIODIC, NULL, NULL);
    ctx->hTMax   = Nm_Timer_Alloc(cfg->timerMax,          NM_TIMER_MODE_ONESHOT,  NULL, NULL);
    ctx->hTError = Nm_Timer_Alloc(cfg->timerError,        NM_TIMER_MODE_PERIODIC, NULL, NULL);
    ctx->hTWbs   = Nm_Timer_Alloc(cfg->timerWaitBusSleep, NM_TIMER_MODE_ONESHOT,  NULL, NULL);
    ctx->hTTx    = Nm_Timer_Alloc(cfg->timerTx,           NM_TIMER_MODE_ONESHOT,  NULL, NULL);

    /* Stay at OFF — NetworkRequest/PassiveStartUp will trigger FSM transition */
    /* Do NOT auto-transition to INIT here, otherwise NM starts without request */
}

void CanNmOsekDirect_DeInit(NetworkHandleType ch)
{
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    if (NULL == ctx) { return; }
    Nm_Timer_Free(ctx->hTTyp);
    Nm_Timer_Free(ctx->hTMax);
    Nm_Timer_Free(ctx->hTError);
    Nm_Timer_Free(ctx->hTWbs);
    Nm_Timer_Free(ctx->hTTx);
    ctx->config = NULL;
}

CanNm_ReturnType CanNmOsekDirect_PassiveStartUp(NetworkHandleType ch)
{
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    if (NULL == ctx) { return CANNM_E_NOT_OK; }
        Direct_ChangeState(ctx, CANNM_STATE_INITRESET);
    CanNm_ControllerEnable(ch);
    return CANNM_E_OK;
}

CanNm_ReturnType CanNmOsekDirect_NetworkRequest(NetworkHandleType ch)
{
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    if (NULL == ctx) { return CANNM_E_NOT_OK; }
        Direct_ChangeState(ctx, CANNM_STATE_INIT);
    CanNm_ControllerEnable(ch);
    return CANNM_E_OK;
}

CanNm_ReturnType CanNmOsekDirect_NetworkRelease(NetworkHandleType ch)
{
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    if (NULL == ctx) { return CANNM_E_NOT_OK; }
    Nm_Timer_Start(ctx->hTWbs);
    Direct_ChangeState(ctx, CANNM_STATE_NORMALPREPSLEEP);
    return CANNM_E_OK;
}

void CanNmOsekDirect_DisableCommunication(NetworkHandleType ch) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    if (NULL != ctx) { ctx->comEnabled = 0U; }
}
void CanNmOsekDirect_EnableCommunication(NetworkHandleType ch) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    if (NULL != ctx) { ctx->comEnabled = 1U; }
}
void CanNmOsekDirect_SetUserData(NetworkHandleType ch, const uint8* d, uint8 n) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    uint8 i; if (NULL == ctx || NULL == d) return;
    if (n > NM_USER_DATA_MAX) n = NM_USER_DATA_MAX;
    for (i = 0; i < n; i++) ctx->txUserData[i] = d[i]; ctx->txUserDataLen = n;
}
void CanNmOsekDirect_GetUserData(NetworkHandleType ch, uint8* d, uint8* nid) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    uint8 i, dl; if (NULL == ctx || NULL == d || NULL == nid) return;
    if (0U == ctx->rxPduAvailable) return;
    dl = ctx->lastRxPdu.length; if (dl > 2U) dl -= 2U; else dl = 0U;
    if (dl > NM_USER_DATA_MAX) dl = NM_USER_DATA_MAX;
    for (i = 0; i < dl; i++) d[i] = ctx->lastRxPdu.data[i + 2U]; *nid = ctx->lastRxNodeId;
}
void CanNmOsekDirect_GetPduData(NetworkHandleType ch, uint8* pdu) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch); uint8 i;
    if (NULL == ctx || NULL == pdu) return;
    for (i = 0; i < ctx->lastRxPdu.length && i < NM_PDU_MAX_LENGTH; i++) pdu[i] = ctx->lastRxPdu.data[i];
}
uint8 CanNmOsekDirect_GetNodeIdentifier(NetworkHandleType ch) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch); return ctx ? ctx->lastRxNodeId : NM_INVALID_NODE_ID;
}
uint8 CanNmOsekDirect_GetLocalNodeIdentifier(NetworkHandleType ch) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch); return ctx ? ctx->config->nodeId : NM_INVALID_NODE_ID;
}
boolean CanNmOsekDirect_CheckRemoteSleepIndication(NetworkHandleType ch) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch); return (ctx && ctx->sleepIndication) ? TRUE : FALSE;
}
void CanNmOsekDirect_GetState(NetworkHandleType ch, Nm_StateType* s, Nm_ModeType* m) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    if (NULL == ctx) { *s = NM_STATE_UNINIT; *m = NM_MODE_BUS_SLEEP; return; }
    *s = Nm_MapCanNmStateToNmState(ctx->state);
    switch (ctx->state) {
        case CANNM_STATE_NORMAL: case CANNM_STATE_TWBS_NORMAL: case CANNM_STATE_LIMPHOME:
            *m = NM_MODE_NETWORK; break;
        case CANNM_STATE_INITRESET: *m = NM_MODE_NETWORK; break;
        case CANNM_STATE_NORMALPREPSLEEP: case CANNM_STATE_LIMPHOMEPREPSLEEP:
        case CANNM_STATE_TWBS_LIMPHOME: *m = NM_MODE_PREPARE_BUS_SLEEP; break;
        case CANNM_STATE_BUSSLEEP: *m = NM_MODE_BUS_SLEEP; break;
        default: *m = NM_MODE_BUS_SLEEP; break;
    }
}
void CanNmOsekDirect_MainFunction(NetworkHandleType ch) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    if (NULL != ctx) { Direct_FSM(ctx); }
}
void CanNmOsekDirect_TxConfirmation(NetworkHandleType ch) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    if (NULL != ctx) { ctx->txPending = 0U; }
}
void CanNmOsekDirect_RxIndication(NetworkHandleType ch, const uint8* d, uint8 n) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    if (NULL == ctx) return;
    Direct_ProcessRx(ctx, d, n);
    if (NULL != ctx->config->pduRxCallback) { ctx->config->pduRxCallback(ch, &ctx->lastRxPdu); }
}
void CanNmOsekDirect_ControllerBusOff(NetworkHandleType ch) {
    CanNmOsekDirect_ChannelType* ctx = Direct_Ctx(ch);
    if (NULL == ctx) return;
    Direct_ChangeState(ctx, CANNM_STATE_LIMPHOME);
    Nm_Timer_Start(ctx->hTError);
}
