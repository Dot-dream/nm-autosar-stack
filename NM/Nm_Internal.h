/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       Nm_Internal.h
 *  @brief      NM Core internal types and state (NOT exposed to application)
 *
 *  This header defines the internal runtime state structures used by Nm.c.
 *  Application code MUST NOT include this file directly.
 */
/*============================================================================*/

#ifndef NM_INTERNAL_H_
#define NM_INTERNAL_H_

#include "Nm_ConfigTypes.h"
#include "Nm_OsIf.h"
#include "CanNm/CanNm.h"

/*=======[ I N T E R N A L   S T A T E   T Y P E S ]=========================*/

/* Per-channel NM runtime state */
typedef struct Nm_ChannelContextType {
    /* --- Static configuration (pointer to ROM) --- */
    const Nm_ChannelConfigType* config;

    /* --- Cached values from config (for fast access) --- */
    NetworkHandleType handle;
    uint8             nodeId;
    Nm_BusType        busType;
    Nm_NmModeType     nmMode;
    const struct CanNm_VtableType* canNmVtable;  /* vtable for polymorphic dispatch */

    /* --- Runtime state --- */
    Nm_StateType      state;              /* Current NM state */
    Nm_ModeType       mode;               /* Current NM mode */

    /* --- RX data cache (updated on RxIndication) --- */
    Nm_PduType        lastRxPdu;          /* Most recently received NM PDU */
    uint8             lastRxNodeId;       /* Source node ID from last Rx */
    uint8             rxPduAvailable;     /* Flag: lastRxPdu is valid */

    /* --- TX data cache --- */
    uint8             txUserData[NM_USER_DATA_MAX]; /* User data for outgoing NM msg */
    uint8             txUserDataLength;

    /* --- Communication control --- */
    uint8             commEnabled;        /* TRUE when NM PDU Tx is allowed */
    uint8             repeatMsgRequested; /* Repeat Message Request bit */

    /* --- Sleep tracking --- */
    uint8             remoteSleepInd;     /* TRUE if remote sleep indicated */

    /* --- Bus-off tracking --- */
    uint8             busOffActive;       /* TRUE if CAN controller is Bus-Off */

} Nm_ChannelContextType;

/* NM Core global state */
typedef struct {
    uint8                   initialized;    /* NM module initialized flag */
    const Nm_ConfigType*    config;         /* Pointer to global config (ROM) */
    Nm_ChannelContextType   channels[NM_MAX_CHANNELS]; /* Per-channel state */
} Nm_CoreType;

/*=======[ I N T E R N A L   G L O B A L ]==================================*/

/* Singleton NM Core state.
 * Defined in Nm.c, accessed by CanNm.*.c via extern. */
extern Nm_CoreType Nm_Core;

/*=======[ S T A T E   M A P P I N G   H E L P E R S ]======================*/

/* Map CanNm internal state to Nm_StateType (used by CanNm -> Nm Core) */
Nm_StateType Nm_MapCanNmStateToNmState(CanNm_StateType canNmState);

/* Map Nm_StateType to CanNm internal state (used by Nm Core -> CanNm) */
CanNm_StateType Nm_MapNmStateToCanNmState(Nm_StateType nmState);

/*=======[ C A L L B A C K   D I S P A T C H ]==============================*/

/* Internal dispatch functions called by CanNm layer.
 * These in turn call the application-level callbacks in Nm_Cbk.h */

void Nm_Core_DispatchStateChange(NetworkHandleType channel, Nm_StateType newState);
void Nm_Core_DispatchNetworkMode(NetworkHandleType channel);
void Nm_Core_DispatchPrepareBusSleep(NetworkHandleType channel);
void Nm_Core_DispatchBusSleep(NetworkHandleType channel);
void Nm_Core_DispatchNetworkStart(NetworkHandleType channel);

#endif /* NM_INTERNAL_H_ */
