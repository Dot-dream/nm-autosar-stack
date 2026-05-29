/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       CanNm_Autosar.c
 *  @brief      AUTOSAR NM state machine (AUTOSAR Nm 4.x)
 *
 *  AUTOSAR NM differs fundamentally from OSEK NM:
 *    - No logical ring — all nodes broadcast periodic NM PDUs
 *    - Control Bit Vector (CBV) coordinates sleep/wakeup
 *    - Coordinator node aggregates multi-channel sleep readiness
 *    - Optional Partial Networking (PN) support
 *    - No LimpHome concept — bus errors → direct BUS_SLEEP
 *
 *  State diagram:
 *
 *                    ┌──────────┐
 *                    │  UNINIT  │
 *                    └────┬─────┘
 *                         │ Init
 *                         ▼
 *                    ┌──────────┐
 *         ┌─────────│BUS_SLEEP │◄────────────────────────┐
 *         │         └────┬─────┘                         │
 *         │   NetworkReq │  │ Rx NM PDU                  │
 *         │   /PassiveStartUp (passive wakeup)            │
 *         │              ▼  │                             │
 *         │    ┌────────────────┐                         │
 *         │    │ REPEAT_MESSAGE │                         │
 *         │    │ (NM-xxxx times)│                         │
 *         │    └───────┬────────┘                         │
 *         │            │ repeatCount → 0                  │
 *         │            ▼                                  │
 *         │    ┌───────────────────┐                      │
 *         │    │ NORMAL_OPERATION  │                      │
 *         │    └──┬────────────┬───┘                      │
 *         │       │            │ NetworkRelease            │
 *         │       │            ▼                           │
 *         │       │    ┌──────────────┐                   │
 *         │       │    │  READY_SLEEP │ (non-coordinator) │
 *         │       │    └──────┬───────┘                   │
 *         │       │           │                            │
 *         │       │           │ all nodes ready            │
 *         │       │           │ OR WaitBusSleep timeout    │
 *         │       │           ▼                            │
 *         │       │    ┌──────────────────┐               │
 *         │       │    │ PREPARE_BUS_SLEEP│               │
 *         │       │    └────────┬─────────┘               │
 *         │       │             │ WaitBusSleep timeout     │
 *         │       │             └──────────────────────────┘
 *         │       │
 *         │       │ NetworkRelease (coordinator)
 *         │       ▼
 *         │  ┌──────────────┐
 *         │  │ SYNCHRONIZE  │ → all synced → PREPARE_BUS_SLEEP
 *         │  └──────────────┘
 *         │
 *         │  NetworkRequest (in PREPARE_BUS_SLEEP or READY_SLEEP)
 *         └──[abort sleep]────────────────────────────────────┘
 */
/*============================================================================*/

#include "CanNm.h"
#include "Nm_Internal.h"
#include "Nm_OsIf.h"
#include "Nm_Cbk.h"
#include "Nm_Timer/Nm_Timer.h"

/*=======[ C O N S T A N T S ]===============================================*/

/* Default Repeat Message count when not configured */
#define AUTOSAR_DEFAULT_REPEAT_MSG_COUNT   3U

/* CBV bit positions (AUTOSAR NM 4.x) */
#define CBV_REPEAT_MSG_REQ     0x01U   /* Bit 0: Repeat Message Request */
#define CBV_ACTIVE_WAKEUP      0x02U   /* Bit 1: Active Wakeup */
#define CBV_PNI                0x04U   /* Bit 2: Partial Network Info present */
#define CBV_COORDINATOR_SLEEP  0x08U   /* Bit 3: Coordinator Sleep Indication */
#define CBV_USER_DATA_LEN_MASK 0x30U   /* Bits 4-5: User Data Length */
#define CBV_USER_DATA_LEN_SHIFT 4U

/*=======[ I N T E R N A L   C H A N N E L   S T A T E ]====================*/

typedef struct {
    const Nm_ChannelConfigType* config;
    Nm_ChannelContextType*      nmCtx;

    CanNm_StateType state;
    CanNm_StateType prevState;

    /* AUTOSAR NM timers (Nm_Timer middleware) */
    Nm_TimerHandle hNmTimeout;       /* Periodic NM PDU cycle (NM_TIMEOUT_TIMER) */
    Nm_TimerHandle hNmRepeatMsg;     /* Total Repeat Message duration */
    Nm_TimerHandle hNmWaitBusSleep;  /* Wait before Bus-Sleep */

    /* Control Bit Vector (CBV) — outgoing */
    uint8  cbvRepeatMsgReq;    /* Repeat Message Request bit */
    uint8  cbvActiveWakeup;    /* Active Wakeup bit */
    uint8  cbvPNI;             /* Partial Network Info bit */
    uint8  cbvCoordSleep;      /* Coordinator Sleep Indication */
    uint8  cbvUserDataLen;     /* User Data Length (0-6 bytes) */

    /* Control Bit Vector (CBV) — incoming (from last Rx) */
    uint8  rxRepeatMsgReq;
    uint8  rxActiveWakeup;
    uint8  rxPNI;
    uint8  rxCoordSleep;

    /* Counters */
    uint8  repeatMsgCount;     /* Remaining Repeat Message cycles */
    uint8  txMsgCount;         /* Transmitted NM PDU counter */
    uint8  rxMsgCount;         /* Received NM PDU counter */

    /* Communication control */
    uint8  comEnabled;
    uint8  passiveMode;        /* TRUE = RX only, never TX NM PDUs */

    /* Coordinator */
    uint8  isCoordinator;      /* TRUE = this node coordinates multi-bus sleep */
    uint8  allChannelsSynced;  /* TRUE = all coordinated channels sleep-ready */

    /* Remote node sleep tracking */
    uint8  remoteWakeupRequested;   /* Any remote node has RepeatMsgReq or ActiveWakeup */

    /* RX data cache */
    Nm_PduType lastRxPdu;
    uint8      lastRxNodeId;
    uint8      rxPduAvailable;

    /* TX user data cache */
    uint8  txUserData[NM_USER_DATA_MAX];
    uint8  txUserDataLen;

} CanNmAutosar_ChannelType;

/*=======[ G L O B A L S ]===================================================*/

static CanNmAutosar_ChannelType g_channels[NM_MAX_CHANNELS];

/*=======[ C O N T E X T ]===================================================*/

static CanNmAutosar_ChannelType* Autosar_Ctx(NetworkHandleType ch)
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

/*=======[ C B V   E N C O D E ]=============================================*/

static uint8 Autosar_EncodeCbv(const CanNmAutosar_ChannelType* ctx)
{
    uint8 cbv = 0U;
    if (NULL == ctx) { return 0U; }

    if (ctx->cbvRepeatMsgReq)  { cbv |= CBV_REPEAT_MSG_REQ; }
    if (ctx->cbvActiveWakeup)  { cbv |= CBV_ACTIVE_WAKEUP; }
    if (ctx->cbvPNI)           { cbv |= CBV_PNI; }
    if (ctx->cbvCoordSleep)    { cbv |= CBV_COORDINATOR_SLEEP; }

    /* User Data Length in bits 4-5 (0-6 bytes) */
    {
        uint8 len = ctx->cbvUserDataLen;
        if (len > NM_USER_DATA_MAX) { len = NM_USER_DATA_MAX; }
        cbv |= (uint8)((len & 0x03U) << CBV_USER_DATA_LEN_SHIFT);
    }

    return cbv;
}

/*=======[ C B V   D E C O D E ]=============================================*/

static void Autosar_DecodeCbv(CanNmAutosar_ChannelType* ctx, uint8 cbv)
{
    if (NULL == ctx) { return; }

    ctx->rxRepeatMsgReq = (cbv & CBV_REPEAT_MSG_REQ)     ? 1U : 0U;
    ctx->rxActiveWakeup = (cbv & CBV_ACTIVE_WAKEUP)      ? 1U : 0U;
    ctx->rxPNI          = (cbv & CBV_PNI)                ? 1U : 0U;
    ctx->rxCoordSleep   = (cbv & CBV_COORDINATOR_SLEEP)  ? 1U : 0U;

    /* If any remote node still requests wakeup, flag it */
    if (ctx->rxRepeatMsgReq || ctx->rxActiveWakeup) {
        ctx->remoteWakeupRequested = 1U;
    }
}

/*=======[ C B V   R E S E T ]===============================================*/

static void Autosar_ResetCbvOutgoing(CanNmAutosar_ChannelType* ctx)
{
    if (NULL == ctx) { return; }
    ctx->cbvRepeatMsgReq = 0U;
    ctx->cbvActiveWakeup = 0U;
    ctx->cbvPNI          = 0U;
    ctx->cbvCoordSleep   = 0U;
    ctx->cbvUserDataLen  = 0U;
}

/*=======[ S T A T E   C H A N G E ]=========================================*/

static void Autosar_ChangeState(CanNmAutosar_ChannelType* ctx, CanNm_StateType ns)
{
    Nm_StateType nmNs;
    if (NULL == ctx) { return; }
    ctx->prevState = ctx->state;
    ctx->state     = ns;

    nmNs = Nm_MapCanNmStateToNmState(ns);
    Nm_Core_DispatchStateChange(ctx->config->channelHandle, nmNs);

    switch (ns) {

        case CANNM_AUTOSAR_STATE_REPEAT_MESSAGE:
            Nm_Core_DispatchNetworkStart(ctx->config->channelHandle);
            /* Set Repeat Message Request in outgoing CBV */
            ctx->cbvRepeatMsgReq = 1U;
            ctx->cbvActiveWakeup = 1U;
            /* Start Repeat Message timer (full duration) */
            Nm_Timer_Start(ctx->hNmRepeatMsg);
            /* Start periodic TX timer */
            Nm_Timer_Start(ctx->hNmTimeout);
            /* Send first NM PDU immediately */
            break;

        case CANNM_AUTOSAR_STATE_NORMAL_OPERATION:
            Nm_Core_DispatchNetworkMode(ctx->config->channelHandle);
            /* Clear Repeat Message Request, keep ActiveWakeup */
            ctx->cbvRepeatMsgReq = 0U;
            ctx->cbvActiveWakeup = 1U;
            ctx->cbvCoordSleep   = 0U;
            /* Start periodic TX timer */
            Nm_Timer_Start(ctx->hNmTimeout);
            break;

        case CANNM_AUTOSAR_STATE_READY_SLEEP:
            /* Clear wakeup bits, indicate we're ready to sleep */
            ctx->cbvRepeatMsgReq = 0U;
            ctx->cbvActiveWakeup = 0U;
            ctx->cbvCoordSleep   = 0U;
            /* Start NM_WAIT_BUS_SLEEP_TIMER */
            Nm_Timer_Start(ctx->hNmWaitBusSleep);
            /* Reset remote wakeup tracking */
            ctx->remoteWakeupRequested = 0U;
            break;

        case CANNM_AUTOSAR_STATE_PREPARE_BUS_SLEEP:
            Nm_Core_DispatchPrepareBusSleep(ctx->config->channelHandle);
            /* Stop periodic TX — we're going to sleep */
            ctx->cbvRepeatMsgReq = 0U;
            ctx->cbvActiveWakeup = 0U;
            ctx->cbvCoordSleep   = 0U;
            /* Start final wait timer */
            Nm_Timer_Start(ctx->hNmWaitBusSleep);
            break;

        case CANNM_AUTOSAR_STATE_BUS_SLEEP:
            Nm_Core_DispatchBusSleep(ctx->config->channelHandle);
            /* Stop all timers, disable controller */
            ctx->cbvRepeatMsgReq = 0U;
            ctx->cbvActiveWakeup = 0U;
            CanNm_ControllerDisable(ctx->config->channelHandle);
            break;

        case CANNM_AUTOSAR_STATE_SYNCHRONIZE:
            /* Coordinator: start collecting sleep readiness from all channels */
            ctx->cbvCoordSleep = 1U;
            /* Start sync timeout */
            Nm_Timer_Start(ctx->hNmWaitBusSleep);
            /* Continue sending periodic PDUs with CoordinatorSleep bit */
            Nm_Timer_Start(ctx->hNmTimeout);
            break;

        default: break;
    }
}

/*=======[ P D U   T X ]=====================================================*/

static void Autosar_SendPdu(CanNmAutosar_ChannelType* ctx)
{
    uint8 pdu[NM_PDU_MAX_LENGTH];
    uint8 cbv;
    uint8 i;
    uint8 userLen;

    if (NULL == ctx || 0U == ctx->comEnabled || ctx->passiveMode) { return; }

    for (i = 0; i < NM_PDU_MAX_LENGTH; i++) { pdu[i] = 0x00U; }

    /* Byte 0: Source Node Identifier */
    pdu[0] = ctx->config->nodeId;

    /* Byte 1: Control Bit Vector */
    cbv = Autosar_EncodeCbv(ctx);
    pdu[1] = cbv;

    /* Bytes 2-7: User Data */
    userLen = ctx->cbvUserDataLen;
    if (userLen > NM_USER_DATA_MAX) { userLen = NM_USER_DATA_MAX; }
    for (i = 0; i < userLen && i < (NM_PDU_MAX_LENGTH - 2U); i++) {
        pdu[i + 2U] = ctx->txUserData[i];
    }

    ctx->txMsgCount++;
    (void)CanNm_Transmit(ctx->config->channelHandle, pdu, NM_PDU_MAX_LENGTH);
}

/*=======[ P D U   R X ]=====================================================*/

static void Autosar_ProcessRx(CanNmAutosar_ChannelType* ctx,
                              const uint8* data, uint8 len)
{
    uint8 cbv;
    uint8 i;

    if (NULL == ctx || NULL == data || len < 2U) { return; }

    /* Cache PDU */
    for (i = 0; i < len && i < NM_PDU_MAX_LENGTH; i++) {
        ctx->lastRxPdu.data[i] = data[i];
    }
    ctx->lastRxPdu.length = len;
    ctx->lastRxNodeId     = data[0];   /* Byte 0 = Source Node ID */
    ctx->rxPduAvailable   = 1U;
    ctx->rxMsgCount++;

    /* Parse Control Bit Vector (Byte 1) */
    cbv = data[1];
    Autosar_DecodeCbv(ctx, cbv);

    /* Wake-up from Bus-Sleep on any received NM PDU */
    if (CANNM_AUTOSAR_STATE_BUS_SLEEP == ctx->state) {
        ctx->cbvRepeatMsgReq = 1U;
        ctx->cbvActiveWakeup = 1U;
        Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_REPEAT_MESSAGE);
        return;
    }

    /* If we're in READY_SLEEP and a remote node still needs the network,
     * abort sleep preparation and go back to NORMAL_OPERATION. */
    if (CANNM_AUTOSAR_STATE_READY_SLEEP == ctx->state) {
        if (ctx->rxRepeatMsgReq || ctx->rxActiveWakeup) {
            Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_NORMAL_OPERATION);
            return;
        }
    }

    /* Restart periodic timer on received NM PDU (resets NM_TIMEOUT_TIMER) */
    Nm_Timer_Start(ctx->hNmTimeout);
}

/*=======[ S T A T E   M A C H I N E ]=======================================*/

static void Autosar_FSM(CanNmAutosar_ChannelType* ctx)
{
    if (NULL == ctx) { return; }

    switch (ctx->state) {

        /* ---- UNINIT ---- */
        case CANNM_AUTOSAR_STATE_UNINIT:
            break;

        /* ---- BUS_SLEEP ---- */
        case CANNM_AUTOSAR_STATE_BUS_SLEEP:
            /* Idle — waiting for NetworkRequest or received NM PDU */
            break;

        /* ---- REPEAT_MESSAGE ---- */
        case CANNM_AUTOSAR_STATE_REPEAT_MESSAGE:
            /* Send periodic NM PDUs with RepeatMsgReq bit set.
             * Decrement repeatMsgCount each cycle.
             * When count reaches 0, transition to NORMAL_OPERATION. */

            if (Nm_Timer_IsExpired(ctx->hNmTimeout)) {
                Nm_Timer_Start(ctx->hNmTimeout);
                Autosar_SendPdu(ctx);
            }

            if (Nm_Timer_IsExpired(ctx->hNmRepeatMsg)) {
                /* Repeat Message duration expired → NORMAL_OPERATION */
                Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_NORMAL_OPERATION);
                break;
            }

            /* Immediate NetworkRelease → skip to READY_SLEEP */
            /* (handled in CanNmAutosar_NetworkRelease by checking state) */
            break;

        /* ---- NORMAL_OPERATION ---- */
        case CANNM_AUTOSAR_STATE_NORMAL_OPERATION:
            /* Periodic NM PDU transmission + RX timeout.
             * On each hNmTimeout tick: send PDU, restart timer.
             * If no RX for >2 cycles → all nodes left → BUS_SLEEP. */
            {
                static uint8 no_rx_count[NM_MAX_CHANNELS];
                uint8 idx = ctx->config->channelHandle;
                if (Nm_Timer_IsExpired(ctx->hNmTimeout)) {
                    Autosar_SendPdu(ctx);
                    Nm_Timer_Start(ctx->hNmTimeout);
                    no_rx_count[idx]++;
                    if (no_rx_count[idx] >= 3U) {
                        no_rx_count[idx] = 0U;
                        Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_BUS_SLEEP);
                        break;
                    }
                } else {
                    /* RX happened (timer restarted by Autosar_ProcessRx) */
                    no_rx_count[idx] = 0U;
                }
            }
            break;

        /* ---- READY_SLEEP ---- */
        case CANNM_AUTOSAR_STATE_READY_SLEEP:
            /* Waiting for all remote nodes to indicate sleep readiness.
             * Sends NM PDUs without ActiveWakeup bit.
             * Monitors incoming PDUs for RepeatMsgReq or ActiveWakeup.
             * On NM_WAIT_BUS_SLEEP_TIMER expiry → PREPARE_BUS_SLEEP. */

            if (Nm_Timer_IsExpired(ctx->hNmTimeout)) {
                Nm_Timer_Start(ctx->hNmTimeout);
                Autosar_SendPdu(ctx);
            }

            if (Nm_Timer_IsExpired(ctx->hNmWaitBusSleep)) {
                /* All nodes ready (no wakeup detected) → PREPARE_BUS_SLEEP */
                Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_PREPARE_BUS_SLEEP);
                break;
            }

            /* If remote wakeup detected (handled in Autosar_ProcessRx),
             * we already transitioned to NORMAL_OPERATION. */
            break;

        /* ---- PREPARE_BUS_SLEEP ---- */
        case CANNM_AUTOSAR_STATE_PREPARE_BUS_SLEEP:
            /* Final stage before Bus-Sleep.
             * Send one last NM PDU without wakeup bits.
             * Wait for NM_WAIT_BUS_SLEEP_TIMER → BUS_SLEEP. */

            if (Nm_Timer_IsExpired(ctx->hNmTimeout)) {
                Nm_Timer_Start(ctx->hNmTimeout);
                Autosar_SendPdu(ctx);
            }

            if (Nm_Timer_IsExpired(ctx->hNmWaitBusSleep)) {
                Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_BUS_SLEEP);
            }
            break;

        /* ---- SYNCHRONIZE (Coordinator only) ---- */
        case CANNM_AUTOSAR_STATE_SYNCHRONIZE:
            /* Coordinator sends NM PDUs with CoordinatorSleep bit.
             * Waits for all channels to confirm sleep readiness.
             * On NM_WAIT_BUS_SLEEP_TIMER expiry → PREPARE_BUS_SLEEP. */

            if (Nm_Timer_IsExpired(ctx->hNmTimeout)) {
                Nm_Timer_Start(ctx->hNmTimeout);
                Autosar_SendPdu(ctx);
            }

            if (Nm_Timer_IsExpired(ctx->hNmWaitBusSleep)) {
                /* All channels synced → proceed to PREPARE_BUS_SLEEP */
                Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_PREPARE_BUS_SLEEP);
            }
            break;

        default: break;
    }
}

/*=======[ P U B L I C   A P I   --  v t a b l e   i m p l e m e n t a t i o n ]===*/

/*=======[ I N I T   /   D E I N I T ]=======================================*/

void CanNmAutosar_Init(const Nm_ChannelConfigType* cfg, Nm_ChannelContextType* nmCtx)
{
    uint8 i;
    CanNmAutosar_ChannelType* ctx = NULL;

    if (NULL == cfg || NULL == nmCtx) { return; }
    for (i = 0; i < NM_MAX_CHANNELS; i++) {
        if (g_channels[i].config == NULL) { ctx = &g_channels[i]; break; }
    }
    if (NULL == ctx) { return; }

    ctx->config           = cfg;
    ctx->nmCtx            = nmCtx;
    ctx->state            = CANNM_AUTOSAR_STATE_UNINIT;
    ctx->prevState        = CANNM_AUTOSAR_STATE_UNINIT;
    ctx->comEnabled       = 1U;
    ctx->passiveMode      = 0U;
    ctx->isCoordinator    = (cfg->busSyncEnabled != 0U) ? 1U : 0U;
    ctx->txMsgCount       = 0U;
    ctx->rxMsgCount       = 0U;
    ctx->repeatMsgCount   = (cfg->txCountLimit > 0U) ? cfg->txCountLimit
                                                      : AUTOSAR_DEFAULT_REPEAT_MSG_COUNT;
    ctx->remoteWakeupRequested = 0U;
    ctx->allChannelsSynced     = 0U;

    Autosar_ResetCbvOutgoing(ctx);

    /* Allocate AUTOSAR NM timers */
    ctx->hNmTimeout = Nm_Timer_Alloc(
        (cfg->timerNmMsgCycle > 0U) ? cfg->timerNmMsgCycle : cfg->timerTyp,
        NM_TIMER_MODE_PERIODIC, NULL, NULL);
    ctx->hNmRepeatMsg = Nm_Timer_Alloc(
        (uint32)ctx->repeatMsgCount * ((cfg->timerNmMsgCycle > 0U)
            ? cfg->timerNmMsgCycle : cfg->timerTyp),
        NM_TIMER_MODE_ONESHOT, NULL, NULL);
    ctx->hNmWaitBusSleep = Nm_Timer_Alloc(cfg->timerWaitBusSleep,
        NM_TIMER_MODE_ONESHOT, NULL, NULL);

    Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_BUS_SLEEP);
}

void CanNmAutosar_DeInit(NetworkHandleType ch)
{
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    if (NULL == ctx) { return; }
    Nm_Timer_Free(ctx->hNmTimeout);
    Nm_Timer_Free(ctx->hNmRepeatMsg);
    Nm_Timer_Free(ctx->hNmWaitBusSleep);
    ctx->config = NULL;
}

/*=======[ N E T W O R K   C O N T R O L ]===================================*/

CanNm_ReturnType CanNmAutosar_PassiveStartUp(NetworkHandleType ch)
{
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    if (NULL == ctx) { return CANNM_E_NOT_OK; }

    /* Passive wakeup — received NM PDU while in Bus-Sleep.
     * Do NOT set ActiveWakeup (we were woken, didn't request it). */
    ctx->cbvRepeatMsgReq = 1U;
    ctx->cbvActiveWakeup = 0U;
    ctx->cbvCoordSleep   = 0U;
    ctx->repeatMsgCount  = (ctx->config->txCountLimit > 0U)
                           ? ctx->config->txCountLimit
                           : AUTOSAR_DEFAULT_REPEAT_MSG_COUNT;

    Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_REPEAT_MESSAGE);
    CanNm_ControllerEnable(ch);
    return CANNM_E_OK;
}

CanNm_ReturnType CanNmAutosar_NetworkRequest(NetworkHandleType ch)
{
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    if (NULL == ctx) { return CANNM_E_NOT_OK; }

    ctx->cbvActiveWakeup = 1U;  /* Active request from application */
    ctx->cbvCoordSleep   = 0U;

    switch (ctx->state) {

        case CANNM_AUTOSAR_STATE_BUS_SLEEP:
        case CANNM_AUTOSAR_STATE_PREPARE_BUS_SLEEP:
            /* Cold/warm start → enter REPEAT_MESSAGE */
            ctx->cbvRepeatMsgReq = 1U;
            ctx->repeatMsgCount  = (ctx->config->txCountLimit > 0U)
                                   ? ctx->config->txCountLimit
                                   : AUTOSAR_DEFAULT_REPEAT_MSG_COUNT;
            Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_REPEAT_MESSAGE);
            break;

        case CANNM_AUTOSAR_STATE_READY_SLEEP:
            /* Abort sleep preparation → back to NORMAL_OPERATION */
            Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_NORMAL_OPERATION);
            break;

        case CANNM_AUTOSAR_STATE_SYNCHRONIZE:
            /* Abort coordinator sync → back to NORMAL_OPERATION */
            Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_NORMAL_OPERATION);
            break;

        case CANNM_AUTOSAR_STATE_NORMAL_OPERATION:
        case CANNM_AUTOSAR_STATE_REPEAT_MESSAGE:
            /* Already in network — restart Repeat Message if configured
             * (Immediate Restart). Continue normal operation. */
            ctx->cbvRepeatMsgReq = 1U;
            ctx->repeatMsgCount  = (ctx->config->txCountLimit > 0U)
                                   ? ctx->config->txCountLimit
                                   : AUTOSAR_DEFAULT_REPEAT_MSG_COUNT;
            Nm_Timer_Start(ctx->hNmRepeatMsg);
            Nm_Timer_Start(ctx->hNmTimeout);
            break;

        default:
            break;
    }

    CanNm_ControllerEnable(ch);
    return CANNM_E_OK;
}

CanNm_ReturnType CanNmAutosar_NetworkRelease(NetworkHandleType ch)
{
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    if (NULL == ctx) { return CANNM_E_NOT_OK; }

    ctx->cbvActiveWakeup = 0U;
    ctx->cbvRepeatMsgReq = 0U;

    switch (ctx->state) {

        case CANNM_AUTOSAR_STATE_REPEAT_MESSAGE:
        case CANNM_AUTOSAR_STATE_NORMAL_OPERATION:
            if (ctx->isCoordinator) {
                /* Coordinator: enter SYNCHRONIZE to aggregate channels */
                Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_SYNCHRONIZE);
            } else {
                /* Non-coordinator: enter READY_SLEEP */
                Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_READY_SLEEP);
            }
            break;

        case CANNM_AUTOSAR_STATE_READY_SLEEP:
        case CANNM_AUTOSAR_STATE_SYNCHRONIZE:
        case CANNM_AUTOSAR_STATE_PREPARE_BUS_SLEEP:
            /* Already shutting down — confirm the release */
            break;

        default:
            break;
    }

    return CANNM_E_OK;
}

/*=======[ C O M M U N I C A T I O N   C O N T R O L ]=======================*/

void CanNmAutosar_DisableCommunication(NetworkHandleType ch) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    if (NULL != ctx) { ctx->comEnabled = 0U; }
}
void CanNmAutosar_EnableCommunication(NetworkHandleType ch) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    if (NULL != ctx) { ctx->comEnabled = 1U; }
}

/*=======[ U S E R   D A T A ]===============================================*/

void CanNmAutosar_SetUserData(NetworkHandleType ch, const uint8* d, uint8 n) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    uint8 i;
    if (NULL == ctx || NULL == d) return;
    if (n > NM_USER_DATA_MAX) { n = NM_USER_DATA_MAX; }
    for (i = 0; i < n; i++) { ctx->txUserData[i] = d[i]; }
    ctx->txUserDataLen = n;
    ctx->cbvUserDataLen = n;
}

void CanNmAutosar_GetUserData(NetworkHandleType ch, uint8* d, uint8* nid) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    uint8 i, dl;
    if (NULL == ctx || NULL == d || NULL == nid) return;
    if (0U == ctx->rxPduAvailable) return;
    dl = ctx->lastRxPdu.length;
    if (dl > 2U) { dl -= 2U; } else { dl = 0U; }
    if (dl > NM_USER_DATA_MAX) { dl = NM_USER_DATA_MAX; }
    for (i = 0; i < dl; i++) { d[i] = ctx->lastRxPdu.data[i + 2U]; }
    *nid = ctx->lastRxNodeId;
}

void CanNmAutosar_GetPduData(NetworkHandleType ch, uint8* pdu) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    uint8 i;
    if (NULL == ctx || NULL == pdu) return;
    for (i = 0; i < ctx->lastRxPdu.length && i < NM_PDU_MAX_LENGTH; i++) {
        pdu[i] = ctx->lastRxPdu.data[i];
    }
}

/*=======[ N O D E   I N F O ]===============================================*/

uint8 CanNmAutosar_GetNodeIdentifier(NetworkHandleType ch) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    return ctx ? ctx->lastRxNodeId : NM_INVALID_NODE_ID;
}

uint8 CanNmAutosar_GetLocalNodeIdentifier(NetworkHandleType ch) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    return ctx ? ctx->config->nodeId : NM_INVALID_NODE_ID;
}

boolean CanNmAutosar_CheckRemoteSleepIndication(NetworkHandleType ch) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    if (NULL == ctx) { return FALSE; }
    /* All remote nodes are sleep-ready if no node has requested wakeup
     * and we are in (or past) READY_SLEEP state. */
    if (ctx->remoteWakeupRequested) { return FALSE; }
    return (ctx->state == CANNM_AUTOSAR_STATE_READY_SLEEP
         || ctx->state == CANNM_AUTOSAR_STATE_PREPARE_BUS_SLEEP
         || ctx->state == CANNM_AUTOSAR_STATE_SYNCHRONIZE) ? TRUE : FALSE;
}

/*=======[ S T A T E ]=======================================================*/

void CanNmAutosar_GetState(NetworkHandleType ch, Nm_StateType* s, Nm_ModeType* m) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    if (NULL == ctx) { *s = NM_STATE_UNINIT; *m = NM_MODE_BUS_SLEEP; return; }
    *s = Nm_MapCanNmStateToNmState(ctx->state);
    switch (ctx->state) {
        case CANNM_AUTOSAR_STATE_REPEAT_MESSAGE:
        case CANNM_AUTOSAR_STATE_NORMAL_OPERATION:
            *m = NM_MODE_NETWORK; break;
        case CANNM_AUTOSAR_STATE_SYNCHRONIZE:
            *m = NM_MODE_SYNCHRONIZE; break;
        case CANNM_AUTOSAR_STATE_READY_SLEEP:
        case CANNM_AUTOSAR_STATE_PREPARE_BUS_SLEEP:
            *m = NM_MODE_PREPARE_BUS_SLEEP; break;
        case CANNM_AUTOSAR_STATE_BUS_SLEEP:
            *m = NM_MODE_BUS_SLEEP; break;
        default: *m = NM_MODE_BUS_SLEEP; break;
    }
}

/*=======[ M A I N   F U N C T I O N ]=======================================*/

void CanNmAutosar_MainFunction(NetworkHandleType ch) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    if (NULL != ctx) { Autosar_FSM(ctx); }
}

/*=======[ E V E N T   H A N D L E R S ]=====================================*/

void CanNmAutosar_TxConfirmation(NetworkHandleType ch) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    if (NULL == ctx) return;
    /* In AUTOSAR NM, TxConfirmation is used for flow control.
     * With immediate TxConf mode, this is a no-op.
     * With polling mode, schedule next transmission here. */
}

void CanNmAutosar_RxIndication(NetworkHandleType ch, const uint8* d, uint8 n) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    if (NULL == ctx) return;
    Autosar_ProcessRx(ctx, d, n);
    if (NULL != ctx->config->pduRxCallback) {
        ctx->config->pduRxCallback(ch, &ctx->lastRxPdu);
    }
}

void CanNmAutosar_ControllerBusOff(NetworkHandleType ch) {
    CanNmAutosar_ChannelType* ctx = Autosar_Ctx(ch);
    if (NULL == ctx) return;
    /* AUTOSAR NM: Bus-Off → direct transition to BUS_SLEEP.
     * No LimpHome concept. Recovery via CAN driver layer. */
    Autosar_ChangeState(ctx, CANNM_AUTOSAR_STATE_BUS_SLEEP);
}
