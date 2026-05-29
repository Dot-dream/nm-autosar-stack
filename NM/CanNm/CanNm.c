/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       CanNm.c
 *  @brief      CanNm adapter — vtable-based dispatch to NM state machines
 *
 *  This file routes CanNm_*() calls from Nm Core to the appropriate
 *  state machine implementation (Direct / Indirect / Autosar) via
 *  a function pointer vtable selected at Init time.
 *
 *  Adding a new NM type requires:
 *    1. New CanNm_Xxx.c with 18 functions matching CanNm_VtableType
 *    2. One new vtable instance in this file
 *    3. One new case in CanNm_Init()
 */
/*============================================================================*/

#include "CanNm.h"
#include "Nm_Internal.h"
#include "Nm_OsIf.h"

/*=======[ F O R W A R D   D E C L A R A T I O N S ]=========================*/

/* Direct NM functions (defined in CanNm_Direct.c) */
extern void     CanNmOsekDirect_Init(const Nm_ChannelConfigType* config, Nm_ChannelContextType* ctx);
extern void     CanNmOsekDirect_DeInit(NetworkHandleType channel);
extern CanNm_ReturnType CanNmOsekDirect_PassiveStartUp(NetworkHandleType channel);
extern CanNm_ReturnType CanNmOsekDirect_NetworkRequest(NetworkHandleType channel);
extern CanNm_ReturnType CanNmOsekDirect_NetworkRelease(NetworkHandleType channel);
extern void     CanNmOsekDirect_DisableCommunication(NetworkHandleType channel);
extern void     CanNmOsekDirect_EnableCommunication(NetworkHandleType channel);
extern void     CanNmOsekDirect_SetUserData(NetworkHandleType channel, const uint8* data, uint8 length);
extern void     CanNmOsekDirect_GetUserData(NetworkHandleType channel, uint8* data, uint8* nodeId);
extern void     CanNmOsekDirect_GetPduData(NetworkHandleType channel, uint8* pdu);
extern uint8    CanNmOsekDirect_GetNodeIdentifier(NetworkHandleType channel);
extern uint8    CanNmOsekDirect_GetLocalNodeIdentifier(NetworkHandleType channel);
extern boolean  CanNmOsekDirect_CheckRemoteSleepIndication(NetworkHandleType channel);
extern void     CanNmOsekDirect_GetState(NetworkHandleType channel, Nm_StateType* state, Nm_ModeType* mode);
extern void     CanNmOsekDirect_MainFunction(NetworkHandleType channel);
extern void     CanNmOsekDirect_TxConfirmation(NetworkHandleType channel);
extern void     CanNmOsekDirect_RxIndication(NetworkHandleType channel, const uint8* pduData, uint8 pduLength);
extern void     CanNmOsekDirect_ControllerBusOff(NetworkHandleType channel);

/* Indirect NM functions (defined in CanNm_Indirect.c) */
extern void     CanNmOsekIndirect_Init(const Nm_ChannelConfigType* config, Nm_ChannelContextType* ctx);
extern void     CanNmOsekIndirect_DeInit(NetworkHandleType channel);
extern CanNm_ReturnType CanNmOsekIndirect_PassiveStartUp(NetworkHandleType channel);
extern CanNm_ReturnType CanNmOsekIndirect_NetworkRequest(NetworkHandleType channel);
extern CanNm_ReturnType CanNmOsekIndirect_NetworkRelease(NetworkHandleType channel);
extern void     CanNmOsekIndirect_DisableCommunication(NetworkHandleType channel);
extern void     CanNmOsekIndirect_EnableCommunication(NetworkHandleType channel);
extern void     CanNmOsekIndirect_SetUserData(NetworkHandleType channel, const uint8* data, uint8 length);
extern void     CanNmOsekIndirect_GetUserData(NetworkHandleType channel, uint8* data, uint8* nodeId);
extern void     CanNmOsekIndirect_GetPduData(NetworkHandleType channel, uint8* pdu);
extern uint8    CanNmOsekIndirect_GetNodeIdentifier(NetworkHandleType channel);
extern uint8    CanNmOsekIndirect_GetLocalNodeIdentifier(NetworkHandleType channel);
extern boolean  CanNmOsekIndirect_CheckRemoteSleepIndication(NetworkHandleType channel);
extern void     CanNmOsekIndirect_GetState(NetworkHandleType channel, Nm_StateType* state, Nm_ModeType* mode);
extern void     CanNmOsekIndirect_MainFunction(NetworkHandleType channel);
extern void     CanNmOsekIndirect_TxConfirmation(NetworkHandleType channel);
extern void     CanNmOsekIndirect_RxIndication(NetworkHandleType channel, const uint8* pduData, uint8 pduLength);
extern void     CanNmOsekIndirect_ControllerBusOff(NetworkHandleType channel);

/* AUTOSAR NM functions (defined in CanNm_Autosar.c) */
extern void     CanNmAutosar_Init(const Nm_ChannelConfigType* config, Nm_ChannelContextType* ctx);
extern void     CanNmAutosar_DeInit(NetworkHandleType channel);
extern CanNm_ReturnType CanNmAutosar_PassiveStartUp(NetworkHandleType channel);
extern CanNm_ReturnType CanNmAutosar_NetworkRequest(NetworkHandleType channel);
extern CanNm_ReturnType CanNmAutosar_NetworkRelease(NetworkHandleType channel);
extern void     CanNmAutosar_DisableCommunication(NetworkHandleType channel);
extern void     CanNmAutosar_EnableCommunication(NetworkHandleType channel);
extern void     CanNmAutosar_SetUserData(NetworkHandleType channel, const uint8* data, uint8 length);
extern void     CanNmAutosar_GetUserData(NetworkHandleType channel, uint8* data, uint8* nodeId);
extern void     CanNmAutosar_GetPduData(NetworkHandleType channel, uint8* pdu);
extern uint8    CanNmAutosar_GetNodeIdentifier(NetworkHandleType channel);
extern uint8    CanNmAutosar_GetLocalNodeIdentifier(NetworkHandleType channel);
extern boolean  CanNmAutosar_CheckRemoteSleepIndication(NetworkHandleType channel);
extern void     CanNmAutosar_GetState(NetworkHandleType channel, Nm_StateType* state, Nm_ModeType* mode);
extern void     CanNmAutosar_MainFunction(NetworkHandleType channel);
extern void     CanNmAutosar_TxConfirmation(NetworkHandleType channel);
extern void     CanNmAutosar_RxIndication(NetworkHandleType channel, const uint8* pduData, uint8 pduLength);
extern void     CanNmAutosar_ControllerBusOff(NetworkHandleType channel);

/*=======[ V T A B L E   I N S T A N C E S ]==================================*/

static const CanNm_VtableType g_vtableDirect = {
    CanNmOsekDirect_Init,
    CanNmOsekDirect_DeInit,
    CanNmOsekDirect_PassiveStartUp,
    CanNmOsekDirect_NetworkRequest,
    CanNmOsekDirect_NetworkRelease,
    CanNmOsekDirect_DisableCommunication,
    CanNmOsekDirect_EnableCommunication,
    CanNmOsekDirect_SetUserData,
    CanNmOsekDirect_GetUserData,
    CanNmOsekDirect_GetPduData,
    CanNmOsekDirect_GetNodeIdentifier,
    CanNmOsekDirect_GetLocalNodeIdentifier,
    CanNmOsekDirect_CheckRemoteSleepIndication,
    CanNmOsekDirect_GetState,
    CanNmOsekDirect_MainFunction,
    CanNmOsekDirect_TxConfirmation,
    CanNmOsekDirect_RxIndication,
    CanNmOsekDirect_ControllerBusOff
};

static const CanNm_VtableType g_vtableIndirect = {
    CanNmOsekIndirect_Init,
    CanNmOsekIndirect_DeInit,
    CanNmOsekIndirect_PassiveStartUp,
    CanNmOsekIndirect_NetworkRequest,
    CanNmOsekIndirect_NetworkRelease,
    CanNmOsekIndirect_DisableCommunication,
    CanNmOsekIndirect_EnableCommunication,
    CanNmOsekIndirect_SetUserData,
    CanNmOsekIndirect_GetUserData,
    CanNmOsekIndirect_GetPduData,
    CanNmOsekIndirect_GetNodeIdentifier,
    CanNmOsekIndirect_GetLocalNodeIdentifier,
    CanNmOsekIndirect_CheckRemoteSleepIndication,
    CanNmOsekIndirect_GetState,
    CanNmOsekIndirect_MainFunction,
    CanNmOsekIndirect_TxConfirmation,
    CanNmOsekIndirect_RxIndication,
    CanNmOsekIndirect_ControllerBusOff
};

static const CanNm_VtableType g_vtableAutosar = {
    CanNmAutosar_Init,
    CanNmAutosar_DeInit,
    CanNmAutosar_PassiveStartUp,
    CanNmAutosar_NetworkRequest,
    CanNmAutosar_NetworkRelease,
    CanNmAutosar_DisableCommunication,
    CanNmAutosar_EnableCommunication,
    CanNmAutosar_SetUserData,
    CanNmAutosar_GetUserData,
    CanNmAutosar_GetPduData,
    CanNmAutosar_GetNodeIdentifier,
    CanNmAutosar_GetLocalNodeIdentifier,
    CanNmAutosar_CheckRemoteSleepIndication,
    CanNmAutosar_GetState,
    CanNmAutosar_MainFunction,
    CanNmAutosar_TxConfirmation,
    CanNmAutosar_RxIndication,
    CanNmAutosar_ControllerBusOff
};

/* Exported vtable instances for external reference */
const CanNm_VtableType CanNm_VtableDirect   = {
    CanNmOsekDirect_Init, CanNmOsekDirect_DeInit,
    CanNmOsekDirect_PassiveStartUp, CanNmOsekDirect_NetworkRequest, CanNmOsekDirect_NetworkRelease,
    CanNmOsekDirect_DisableCommunication, CanNmOsekDirect_EnableCommunication,
    CanNmOsekDirect_SetUserData, CanNmOsekDirect_GetUserData, CanNmOsekDirect_GetPduData,
    CanNmOsekDirect_GetNodeIdentifier, CanNmOsekDirect_GetLocalNodeIdentifier,
    CanNmOsekDirect_CheckRemoteSleepIndication,
    CanNmOsekDirect_GetState, CanNmOsekDirect_MainFunction,
    CanNmOsekDirect_TxConfirmation, CanNmOsekDirect_RxIndication,
    CanNmOsekDirect_ControllerBusOff
};

const CanNm_VtableType CanNm_VtableIndirect = {
    CanNmOsekIndirect_Init, CanNmOsekIndirect_DeInit,
    CanNmOsekIndirect_PassiveStartUp, CanNmOsekIndirect_NetworkRequest, CanNmOsekIndirect_NetworkRelease,
    CanNmOsekIndirect_DisableCommunication, CanNmOsekIndirect_EnableCommunication,
    CanNmOsekIndirect_SetUserData, CanNmOsekIndirect_GetUserData, CanNmOsekIndirect_GetPduData,
    CanNmOsekIndirect_GetNodeIdentifier, CanNmOsekIndirect_GetLocalNodeIdentifier,
    CanNmOsekIndirect_CheckRemoteSleepIndication,
    CanNmOsekIndirect_GetState, CanNmOsekIndirect_MainFunction,
    CanNmOsekIndirect_TxConfirmation, CanNmOsekIndirect_RxIndication,
    CanNmOsekIndirect_ControllerBusOff
};

const CanNm_VtableType CanNm_VtableAutosar = {
    CanNmAutosar_Init, CanNmAutosar_DeInit,
    CanNmAutosar_PassiveStartUp, CanNmAutosar_NetworkRequest, CanNmAutosar_NetworkRelease,
    CanNmAutosar_DisableCommunication, CanNmAutosar_EnableCommunication,
    CanNmAutosar_SetUserData, CanNmAutosar_GetUserData, CanNmAutosar_GetPduData,
    CanNmAutosar_GetNodeIdentifier, CanNmAutosar_GetLocalNodeIdentifier,
    CanNmAutosar_CheckRemoteSleepIndication,
    CanNmAutosar_GetState, CanNmAutosar_MainFunction,
    CanNmAutosar_TxConfirmation, CanNmAutosar_RxIndication,
    CanNmAutosar_ControllerBusOff
};

/*=======[ C O N T E X T   L O O K U P ]=====================================*/

static Nm_ChannelContextType* CanNm_GetCtx(NetworkHandleType channel)
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

static Nm_ChannelContextType* CanNm_GetCtxChecked(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtx(channel);
    if (NULL == ctx || NULL == ctx->canNmVtable) { return NULL; }
    return ctx;
}

/*=======[ I N I T   /   D E I N I T ]=======================================*/

void CanNm_Init(const Nm_ChannelConfigType* channelConfig)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtx(channelConfig->channelHandle);
    if (NULL == ctx) { return; }

    /* Select vtable based on NM mode */
    switch (channelConfig->nmMode) {
        case NM_MODE_DIRECT:   ctx->canNmVtable = &g_vtableDirect;   break;
        case NM_MODE_INDIRECT: ctx->canNmVtable = &g_vtableIndirect; break;
        case NM_MODE_AUTOSAR:  ctx->canNmVtable = &g_vtableAutosar;  break;
        default: return;
    }

    ctx->canNmVtable->Init(channelConfig, ctx);
}

/*=======[ D I S P A T C H   F U N C T I O N S ]=============================*/

void CanNm_DeInit(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL != ctx) { ctx->canNmVtable->DeInit(channel); }
}

CanNm_ReturnType CanNm_PassiveStartUp(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL == ctx) { return CANNM_E_NOT_OK; }
    return ctx->canNmVtable->PassiveStartUp(channel);
}

CanNm_ReturnType CanNm_NetworkRequest(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL == ctx) { return CANNM_E_NOT_OK; }
    return ctx->canNmVtable->NetworkRequest(channel);
}

CanNm_ReturnType CanNm_NetworkRelease(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL == ctx) { return CANNM_E_NOT_OK; }
    return ctx->canNmVtable->NetworkRelease(channel);
}

void CanNm_DisableCommunication(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL != ctx) { ctx->canNmVtable->DisableCommunication(channel); }
}

void CanNm_EnableCommunication(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL != ctx) { ctx->canNmVtable->EnableCommunication(channel); }
}

void CanNm_SetUserData(NetworkHandleType channel, const uint8* data, uint8 length)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL != ctx) { ctx->canNmVtable->SetUserData(channel, data, length); }
}

void CanNm_GetUserData(NetworkHandleType channel, uint8* data, uint8* nodeId)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL != ctx) { ctx->canNmVtable->GetUserData(channel, data, nodeId); }
}

void CanNm_GetPduData(NetworkHandleType channel, uint8* pdu)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL != ctx) { ctx->canNmVtable->GetPduData(channel, pdu); }
}

uint8 CanNm_GetNodeIdentifier(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL == ctx) { return NM_INVALID_NODE_ID; }
    return ctx->canNmVtable->GetNodeIdentifier(channel);
}

uint8 CanNm_GetLocalNodeIdentifier(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL == ctx) { return NM_INVALID_NODE_ID; }
    return ctx->canNmVtable->GetLocalNodeIdentifier(channel);
}

boolean CanNm_CheckRemoteSleepIndication(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL == ctx) { return FALSE; }
    return ctx->canNmVtable->CheckRemoteSleepIndication(channel);
}

void CanNm_GetState(NetworkHandleType channel, Nm_StateType* state, Nm_ModeType* mode)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL == ctx) {
        *state = NM_STATE_UNINIT;
        *mode  = NM_MODE_BUS_SLEEP;
        return;
    }
    ctx->canNmVtable->GetState(channel, state, mode);
}

void CanNm_MainFunction(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL != ctx) { ctx->canNmVtable->MainFunction(channel); }
}

void CanNm_TxConfirmation(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL != ctx) { ctx->canNmVtable->TxConfirmation(channel); }
}

void CanNm_RxIndication(NetworkHandleType channel, const uint8* pduData, uint8 pduLength)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL != ctx) { ctx->canNmVtable->RxIndication(channel, pduData, pduLength); }
}

void CanNm_ControllerBusOff(NetworkHandleType channel)
{
    Nm_ChannelContextType* ctx = CanNm_GetCtxChecked(channel);
    if (NULL != ctx) { ctx->canNmVtable->ControllerBusOff(channel); }
}

/*=======[ P L A T F O R M   H O O K S   ( t o   b e   w i r e d ) ]========*/

/*
 * The following functions are called by CanNm_Direct/Indirect/Autosar to
 * interact with the CAN hardware. The integrating project must implement these.
 */

NM_WEAK CanNm_ReturnType CanNm_Transmit(
    NetworkHandleType channel,
    const uint8* pduData,
    uint8 pduLength)
{
    (void)channel;
    (void)pduData;
    (void)pduLength;
    /* Platform must override this weak default */
    return CANNM_E_NOT_OK;
}

NM_WEAK void CanNm_ControllerEnable(NetworkHandleType channel)
{
    (void)channel;
    /* Platform must override this weak default */
}

NM_WEAK void CanNm_ControllerDisable(NetworkHandleType channel)
{
    (void)channel;
    /* Platform must override this weak default */
}
