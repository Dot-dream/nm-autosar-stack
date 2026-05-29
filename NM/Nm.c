/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       Nm.c
 *  @brief      Nm Core implementation — routing, state coordination, PDU management
 *
 *  This is the AUTOSAR-compatible Nm abstraction layer.
 *  It routes Nm_*() API calls to the appropriate bus-specific NM
 *  implementation (CanNm, LinNm, etc.) and coordinates multi-channel
 *  operation.
 *
 *  Dependencies:
 *    - Nm_OsIf.h:  Nm_EnterCritical() / Nm_ExitCritical() / Nm_GetSystemTickMs()
 *    - CanNm.h:    bus-specific NM implementation
 */
/*============================================================================*/

#include "Nm.h"
#include "Nm_Internal.h"
#include "Nm_Timer/Nm_Timer.h"
#include "Nm_Cbk.h"

/*=======[ G L O B A L   S T A T E ]=========================================*/

Nm_CoreType Nm_Core;

/*=======[ F O R W A R D   D E C L A R A T I O N S ]=========================*/

static Nm_ChannelContextType* Nm_GetChannelContext(NetworkHandleType channel);
static Nm_ReturnType Nm_ValidateChannel(NetworkHandleType channel);

/*=======[ S T A T E   M A P P I N G ]=======================================*/

Nm_StateType Nm_MapCanNmStateToNmState(CanNm_StateType canNmState)
{
    switch (canNmState) {
        /* Direct NM states */
        case CANNM_STATE_OFF:              return NM_STATE_UNINIT;
        case CANNM_STATE_INIT:             return NM_STATE_UNINIT;
        case CANNM_STATE_INITRESET:        return NM_STATE_INITRESET;
        case CANNM_STATE_NORMAL:           return NM_STATE_NORMAL_OPERATION;
        case CANNM_STATE_NORMALPREPSLEEP:  return NM_STATE_PREPARE_BUS_SLEEP;
        case CANNM_STATE_TWBS_NORMAL:      return NM_STATE_TWBS_NORMAL;
        case CANNM_STATE_BUSSLEEP:         return NM_STATE_BUS_SLEEP;
        case CANNM_STATE_LIMPHOME:         return NM_STATE_LIMPHOME;
        case CANNM_STATE_LIMPHOMEPREPSLEEP:return NM_STATE_LIMPHOME_PREPSLEEP;
        case CANNM_STATE_TWBS_LIMPHOME:    return NM_STATE_TWBS_LIMPHOME;
        case CANNM_STATE_ON:               return NM_STATE_ON;

        /* Indirect NM states (CANNM_IND_STATE_OFF==0x00, same as CANNM_STATE_OFF) */
        case CANNM_IND_STATE_INIT:         return NM_STATE_UNINIT;
        case CANNM_IND_STATE_AWAKE:        return NM_STATE_NORMAL_OPERATION;
        case CANNM_IND_STATE_BUSSLEEP:     return NM_STATE_BUS_SLEEP;
        case CANNM_IND_STATE_NORMAL:       return NM_STATE_NORMAL_OPERATION;
        case CANNM_IND_STATE_WAITBUSSLEEP: return NM_STATE_PREPARE_BUS_SLEEP;
        case CANNM_IND_STATE_LIMPHOME:     return NM_STATE_LIMPHOME;
        case CANNM_IND_STATE_ON:           return NM_STATE_ON;

        /* AUTOSAR NM states */
        case CANNM_AUTOSAR_STATE_UNINIT:             return NM_STATE_UNINIT;
        case CANNM_AUTOSAR_STATE_BUS_SLEEP:          return NM_STATE_BUS_SLEEP;
        case CANNM_AUTOSAR_STATE_REPEAT_MESSAGE:     return NM_STATE_REPEAT_MESSAGE;
        case CANNM_AUTOSAR_STATE_NORMAL_OPERATION:   return NM_STATE_NORMAL_OPERATION;
        case CANNM_AUTOSAR_STATE_READY_SLEEP:        return NM_STATE_READY_SLEEP;
        case CANNM_AUTOSAR_STATE_PREPARE_BUS_SLEEP:  return NM_STATE_PREPARE_BUS_SLEEP;
        case CANNM_AUTOSAR_STATE_SYNCHRONIZE:        return NM_STATE_SYNCHRONIZE;

        default:                           return NM_STATE_UNINIT;
    }
}

CanNm_StateType Nm_MapNmStateToCanNmState(Nm_StateType nmState)
{
    switch (nmState) {
        case NM_STATE_UNINIT:              return CANNM_STATE_OFF;
        case NM_STATE_INITRESET:           return CANNM_STATE_INITRESET;
        case NM_STATE_NORMAL_OPERATION:    return CANNM_STATE_NORMAL;
        case NM_STATE_REPEAT_MESSAGE:      return CANNM_AUTOSAR_STATE_REPEAT_MESSAGE;
        case NM_STATE_PREPARE_BUS_SLEEP:   return CANNM_STATE_NORMALPREPSLEEP;
        case NM_STATE_TWBS_NORMAL:         return CANNM_STATE_TWBS_NORMAL;
        case NM_STATE_BUS_SLEEP:           return CANNM_STATE_BUSSLEEP;
        case NM_STATE_SYNCHRONIZE:         return CANNM_AUTOSAR_STATE_SYNCHRONIZE;
        case NM_STATE_LIMPHOME:            return CANNM_STATE_LIMPHOME;
        case NM_STATE_LIMPHOME_PREPSLEEP:  return CANNM_STATE_LIMPHOMEPREPSLEEP;
        case NM_STATE_TWBS_LIMPHOME:       return CANNM_STATE_TWBS_LIMPHOME;
        case NM_STATE_ON:                  return CANNM_STATE_ON;
        /* AUTOSAR-specific forward mappings */
        case NM_STATE_READY_SLEEP:         return CANNM_AUTOSAR_STATE_READY_SLEEP;
        default:                           return CANNM_STATE_OFF;
    }
}

/*=======[ I N T E R N A L   H E L P E R S ]=================================*/

static Nm_ChannelContextType* Nm_GetChannelContext(NetworkHandleType channel)
{
    uint8 i;
    for (i = 0; i < NM_MAX_CHANNELS; i++) {
        if (Nm_Core.channels[i].config != NULL
            && Nm_Core.channels[i].handle == channel) {
            return &Nm_Core.channels[i];
        }
    }
    return NULL;
}

static Nm_ReturnType Nm_ValidateChannel(NetworkHandleType channel)
{
    if (0U == Nm_Core.initialized) {
        return NM_E_NOT_OK;
    }
    if (NULL == Nm_GetChannelContext(channel)) {
        return NM_E_NOT_OK;
    }
    return NM_E_OK;
}

/*=======[ P U B L I C   A P I   I M P L E M E N T A T I O N ]==============*/

/*****************************************************************************/
void Nm_Init(const Nm_ConfigType* configPtr)
{
    uint8 i;

    if (NULL == configPtr) {
        return;
    }

    Nm_Timer_Init();

    Nm_EnterCritical();

    /* Zero-initialize all channel contexts */
    for (i = 0; i < NM_MAX_CHANNELS; i++) {
        Nm_Core.channels[i].config    = NULL;
        Nm_Core.channels[i].handle    = NM_INVALID_HANDLE;
        Nm_Core.channels[i].state     = NM_STATE_UNINIT;
        Nm_Core.channels[i].mode      = NM_MODE_BUS_SLEEP;
    }

    Nm_Core.config      = configPtr;
    Nm_Core.initialized = 0U;

    /* Initialize each channel */
    for (i = 0; i < configPtr->numChannels && i < NM_MAX_CHANNELS; i++) {
        const Nm_ChannelConfigType* chCfg = &configPtr->channels[i];
        Nm_ChannelContextType*      ctx   = &Nm_Core.channels[i];

        ctx->config    = chCfg;
        ctx->handle    = chCfg->channelHandle;
        ctx->nodeId    = chCfg->nodeId;
        ctx->busType   = chCfg->busType;
        ctx->nmMode    = chCfg->nmMode;
        ctx->commEnabled = 1U;

        /* Initialize bus-specific NM layer */
        CanNm_Init(chCfg);

        /* Set Nm-level state AFTER CanNm init (CanNm DispatchStateChange may
         * set intermediate state values during init FSM transitions). */
        ctx->state     = NM_STATE_BUS_SLEEP;
        ctx->mode      = NM_MODE_BUS_SLEEP;
    }

    Nm_Core.initialized = 1U;

    Nm_ExitCritical();
}

/*****************************************************************************/
#if (NM_DEINIT_API == STD_ON)
void Nm_DeInit(void)
{
    uint8 i;

    Nm_EnterCritical();

    for (i = 0; i < NM_MAX_CHANNELS; i++) {
        if (Nm_Core.channels[i].config != NULL) {
            CanNm_DeInit(Nm_Core.channels[i].handle);
            Nm_Core.channels[i].config = NULL;
        }
    }

    Nm_Core.initialized = 0U;

    Nm_ExitCritical();
}
#endif

/*****************************************************************************/
Nm_ReturnType Nm_PassiveStartUp(NetworkHandleType nmChannelHandle)
{
    CanNm_ReturnType result;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }

    result = CanNm_PassiveStartUp(nmChannelHandle);
    /* State will be updated via DispatchStateChange callback when FSM advances */
    return (CANNM_E_OK == result) ? NM_E_OK : NM_E_NOT_OK;
}

/*****************************************************************************/
Nm_ReturnType Nm_NetworkRequest(NetworkHandleType nmChannelHandle)
{
    CanNm_ReturnType result;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }

    result = CanNm_NetworkRequest(nmChannelHandle);
    /* State will be updated via DispatchStateChange callback when FSM advances */
    return (CANNM_E_OK == result) ? NM_E_OK : NM_E_NOT_OK;
}

/*****************************************************************************/
Nm_ReturnType Nm_NetworkRelease(NetworkHandleType nmChannelHandle)
{
    CanNm_ReturnType result;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }

    result = CanNm_NetworkRelease(nmChannelHandle);
    /* State will be updated via DispatchStateChange callback when FSM advances */
    return (CANNM_E_OK == result) ? NM_E_OK : NM_E_NOT_OK;
}

/*****************************************************************************/
#if (NM_COM_CONTROL_ENABLED == STD_ON)
Nm_ReturnType Nm_DisableCommunication(NetworkHandleType nmChannelHandle)
{
    Nm_ChannelContextType* ctx;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }

    ctx = Nm_GetChannelContext(nmChannelHandle);
    ctx->commEnabled = 0U;
    CanNm_DisableCommunication(nmChannelHandle);

    return NM_E_OK;
}
#endif

/*****************************************************************************/
#if (NM_COM_CONTROL_ENABLED == STD_ON)
Nm_ReturnType Nm_EnableCommunication(NetworkHandleType nmChannelHandle)
{
    Nm_ChannelContextType* ctx;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }

    ctx = Nm_GetChannelContext(nmChannelHandle);
    ctx->commEnabled = 1U;
    CanNm_EnableCommunication(nmChannelHandle);

    return NM_E_OK;
}
#endif

/*****************************************************************************/
#if (NM_USER_DATA_ENABLED == STD_ON)
Nm_ReturnType Nm_SetUserData(
    NetworkHandleType nmChannelHandle,
    const uint8* nmUserDataPtr,
    uint8 nmUserDataLength)
{
    Nm_ChannelContextType* ctx;
    uint8 i;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }
    if (NULL == nmUserDataPtr || nmUserDataLength > NM_USER_DATA_MAX) {
        return NM_E_NOT_OK;
    }

    ctx = Nm_GetChannelContext(nmChannelHandle);

    for (i = 0; i < nmUserDataLength; i++) {
        ctx->txUserData[i] = nmUserDataPtr[i];
    }
    ctx->txUserDataLength = nmUserDataLength;

    CanNm_SetUserData(nmChannelHandle, nmUserDataPtr, nmUserDataLength);

    return NM_E_OK;
}
#endif

/*****************************************************************************/
#if (NM_USER_DATA_ENABLED == STD_ON)
Nm_ReturnType Nm_GetUserData(
    NetworkHandleType nmChannelHandle,
    uint8* nmUserDataPtr,
    uint8* nmNodeIdPtr)
{
    Nm_ChannelContextType* ctx;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }
    if (NULL == nmUserDataPtr || NULL == nmNodeIdPtr) {
        return NM_E_NOT_OK;
    }

    ctx = Nm_GetChannelContext(nmChannelHandle);

    if (0U == ctx->rxPduAvailable) {
        return NM_E_NOT_OK;
    }

    /* Copy user data from PDU (skip first byte = OpCode, second = NodeID) */
    {
        uint8 i;
        uint8 dataLen = ctx->lastRxPdu.length;
        if (dataLen > 2U) {
            dataLen -= 2U;
            if (dataLen > NM_USER_DATA_MAX) {
                dataLen = NM_USER_DATA_MAX;
            }
            for (i = 0; i < dataLen; i++) {
                nmUserDataPtr[i] = ctx->lastRxPdu.data[i + 2U];
            }
        }
    }

    /* CanNm stores nodeId separately -- get it from internal state */
    *nmNodeIdPtr = ctx->lastRxNodeId;

    return NM_E_OK;
}
#endif

/*****************************************************************************/
#if (NM_USER_DATA_ENABLED == STD_ON) || (NM_NODE_ID_ENABLED == STD_ON) \
    || (NM_NODE_DETECTION_ENABLED == STD_ON)
Nm_ReturnType Nm_GetPduData(
    NetworkHandleType nmChannelHandle,
    uint8* nmPduData)
{
    Nm_ChannelContextType* ctx;
    uint8 i;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }
    if (NULL == nmPduData) {
        return NM_E_NOT_OK;
    }

    ctx = Nm_GetChannelContext(nmChannelHandle);

    if (0U == ctx->rxPduAvailable) {
        return NM_E_NOT_OK;
    }

    for (i = 0; i < ctx->lastRxPdu.length && i < NM_PDU_MAX_LENGTH; i++) {
        nmPduData[i] = ctx->lastRxPdu.data[i];
    }

    return NM_E_OK;
}
#endif

/*****************************************************************************/
#if (NM_NODE_DETECTION_ENABLED == STD_ON)
Nm_ReturnType Nm_RepeatMessageRequest(NetworkHandleType nmChannelHandle)
{
    Nm_ChannelContextType* ctx;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }

    ctx = Nm_GetChannelContext(nmChannelHandle);
    ctx->repeatMsgRequested = 1U;

    return NM_E_OK;
}
#endif

/*****************************************************************************/
#if (NM_NODE_ID_ENABLED == STD_ON)
Nm_ReturnType Nm_GetNodeIdentifier(
    NetworkHandleType nmChannelHandle,
    uint8* nmNodeIdPtr)
{
    Nm_ChannelContextType* ctx;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }
    if (NULL == nmNodeIdPtr) {
        return NM_E_NOT_OK;
    }

    ctx = Nm_GetChannelContext(nmChannelHandle);
    *nmNodeIdPtr = CanNm_GetNodeIdentifier(nmChannelHandle);

    return NM_E_OK;
}
#endif

/*****************************************************************************/
#if (NM_NODE_ID_ENABLED == STD_ON)
Nm_ReturnType Nm_GetLocalNodeIdentifier(
    NetworkHandleType nmChannelHandle,
    uint8* nmNodeIdPtr)
{
    Nm_ChannelContextType* ctx;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }
    if (NULL == nmNodeIdPtr) {
        return NM_E_NOT_OK;
    }

    ctx = Nm_GetChannelContext(nmChannelHandle);
    *nmNodeIdPtr = CanNm_GetLocalNodeIdentifier(nmChannelHandle);

    return NM_E_OK;
}
#endif

/*****************************************************************************/
#if (NM_REMOTE_SLEEP_IND_ENABLED == STD_ON)
Nm_ReturnType Nm_CheckRemoteSleepIndication(
    NetworkHandleType nmChannelHandle,
    boolean* nmRemoteSleepIndPtr)
{
    Nm_ChannelContextType* ctx;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }
    if (NULL == nmRemoteSleepIndPtr) {
        return NM_E_NOT_OK;
    }

    *nmRemoteSleepIndPtr = CanNm_CheckRemoteSleepIndication(nmChannelHandle);

    return NM_E_OK;
}
#endif

/*****************************************************************************/
Nm_ReturnType Nm_GetState(
    NetworkHandleType nmChannelHandle,
    Nm_StateType* nmStatePtr,
    Nm_ModeType* nmModePtr)
{
    Nm_ChannelContextType* ctx;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return NM_E_NOT_OK;
    }
    if (NULL == nmStatePtr || NULL == nmModePtr) {
        return NM_E_NOT_OK;
    }

    ctx = Nm_GetChannelContext(nmChannelHandle);
    *nmStatePtr = ctx->state;
    *nmModePtr  = ctx->mode;

    return NM_E_OK;
}

/*****************************************************************************/
#if (NM_VERSION_INFO_API == STD_ON)
void Nm_GetVersionInfo(Std_VersionInfoType* versionInfo)
{
    if (NULL != versionInfo) {
        versionInfo->vendorID         = NM_VENDOR_ID;
        versionInfo->moduleID         = NM_MODULE_ID;
        versionInfo->instanceID       = NM_INSTANCE_ID;
        versionInfo->sw_major_version = NM_SW_MAJOR_VERSION;
        versionInfo->sw_minor_version = NM_SW_MINOR_VERSION;
        versionInfo->sw_patch_version = NM_SW_PATCH_VERSION;
        versionInfo->ar_major_version = NM_AR_MAJOR_VERSION;
        versionInfo->ar_minor_version = NM_AR_MINOR_VERSION;
        versionInfo->ar_patch_version = NM_AR_PATCH_VERSION;
    }
}
#endif

/*****************************************************************************/
void Nm_MainFunction(void)
{
    uint8 i;

    if (0U == Nm_Core.initialized) {
        return;
    }

    /* Process state machines first — they poll Nm_Timer_IsExpired().
     * Nm_Timer_Process() runs after so PERIODIC timers are restarted
     * for the next cycle, but state transitions happen based on the
     * pre-restart expiry check. */
    for (i = 0; i < Nm_Core.config->numChannels && i < NM_MAX_CHANNELS; i++) {
        if (Nm_Core.channels[i].config != NULL) {
            CanNm_MainFunction(Nm_Core.channels[i].handle);
        }
    }

    Nm_Timer_Process();
}

/*****************************************************************************/
void Nm_ControllerBusOff(NetworkHandleType nmChannelHandle)
{
    Nm_ChannelContextType* ctx;

    if (NM_E_OK != Nm_ValidateChannel(nmChannelHandle)) {
        return;
    }

    ctx = Nm_GetChannelContext(nmChannelHandle);
    ctx->busOffActive = 1U;

    CanNm_ControllerBusOff(nmChannelHandle);
}

/*****************************************************************************/
void Nm_TxConfirmation(NetworkHandleType nmChannelHandle)
{
    if (0U == Nm_Core.initialized) {
        return;
    }

    CanNm_TxConfirmation(nmChannelHandle);
}

/*****************************************************************************/
void Nm_RxIndication(
    NetworkHandleType nmChannelHandle,
    const uint8* nmPduData,
    uint8 nmPduLength)
{
    Nm_ChannelContextType* ctx;
    uint8 i;

    if (0U == Nm_Core.initialized) {
        return;
    }
    if (NULL == nmPduData || 0U == nmPduLength || nmPduLength > NM_PDU_MAX_LENGTH) {
        return;
    }

    ctx = Nm_GetChannelContext(nmChannelHandle);
    if (NULL == ctx) {
        return;
    }

    /* Cache the received PDU for later retrieval via Nm_GetPduData etc. */
    for (i = 0; i < nmPduLength; i++) {
        ctx->lastRxPdu.data[i] = nmPduData[i];
    }
    ctx->lastRxPdu.length = nmPduLength;
    ctx->lastRxNodeId     = (nmPduLength >= 2U) ? nmPduData[1] : NM_INVALID_NODE_ID;
    ctx->rxPduAvailable   = 1U;

    /* Forward to bus-specific NM layer */
    CanNm_RxIndication(nmChannelHandle, nmPduData, nmPduLength);
}

/*=======[ C A L L B A C K   D I S P A T C H ]===============================*/

void Nm_Core_DispatchStateChange(NetworkHandleType channel, Nm_StateType newState)
{
    Nm_ChannelContextType* ctx = Nm_GetChannelContext(channel);
    Nm_StateType prevState;

    if (NULL == ctx) { return; }

    prevState  = ctx->state;
    ctx->state = newState;

#if (NM_STATE_CHANGE_IND_ENABLED == STD_ON)
    Nm_StateChangeNotification(channel, prevState, newState);
#endif
}

void Nm_Core_DispatchNetworkMode(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = Nm_GetChannelContext(channel);
    if (NULL == ctx) { return; }
    ctx->mode = NM_MODE_NETWORK;
    Nm_NetworkMode(channel);
}

void Nm_Core_DispatchPrepareBusSleep(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = Nm_GetChannelContext(channel);
    if (NULL == ctx) { return; }
    ctx->mode = NM_MODE_PREPARE_BUS_SLEEP;
    Nm_PrepareBusSleepMode(channel);
}

void Nm_Core_DispatchBusSleep(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = Nm_GetChannelContext(channel);
    if (NULL == ctx) { return; }
    ctx->state = NM_STATE_BUS_SLEEP;
    ctx->mode  = NM_MODE_BUS_SLEEP;
    Nm_BusSleepMode(channel);
}

void Nm_Core_DispatchNetworkStart(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = Nm_GetChannelContext(channel);
    if (NULL == ctx) { return; }
    ctx->state = NM_STATE_REPEAT_MESSAGE;
    ctx->mode  = NM_MODE_NETWORK;
    Nm_NetworkStartIndication(channel);
}

