/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       CanNm.h
 *  @brief      CanNm internal interface (Nм Core <-> bus-specific NM)
 *
 *  CanNm is the CAN-bus-specific NM implementation layer.
 *  It implements the OSEK Direct NM (and optionally Indirect NM)
 *  state machine and communicates with the upper Nm Core layer
 *  through this interface.
 *
 *  CanNm is called by Nm Core (Nm.c). It does NOT expose a public API
 *  to the application. All application interaction goes through Nm_*().
 *
 *  The actual state machine implementation lives in:
 *    - CanNm_Direct.c   : Direct NM (OSEK NM alive/ring/limphome)
 *    - CanNm_Indirect.c : Indirect NM (application message monitoring)
 */
/*============================================================================*/

#ifndef CANNM_H_
#define CANNM_H_

#include "Nm_ConfigTypes.h"

/* Forward declaration -- defined in Nm_Internal.h */
struct Nm_ChannelContextType;

/*=======[ C A N N M   S T A T E S ]=========================================*/

/* CanNm-specific states -- maps to OSEK NM Direct/Indirect states */
typedef uint8 CanNm_StateType;

/* Direct NM states (from OsekNmDirectNmStateType) */
#define CANNM_STATE_OFF                 0x00U
#define CANNM_STATE_INIT                0x01U
#define CANNM_STATE_INITRESET           0x02U
#define CANNM_STATE_NORMAL              0x03U
#define CANNM_STATE_NORMALPREPSLEEP     0x04U
#define CANNM_STATE_TWBS_NORMAL         0x05U
#define CANNM_STATE_BUSSLEEP            0x06U
#define CANNM_STATE_LIMPHOME            0x07U
#define CANNM_STATE_LIMPHOMEPREPSLEEP   0x08U
#define CANNM_STATE_TWBS_LIMPHOME       0x09U
#define CANNM_STATE_ON                  0x0AU

/* Indirect NM states (from OsekNmIndirectNmStateType) */
#define CANNM_IND_STATE_OFF             0x00U
#define CANNM_IND_STATE_INIT            0x10U
#define CANNM_IND_STATE_AWAKE           0x11U
#define CANNM_IND_STATE_BUSSLEEP        0x12U
#define CANNM_IND_STATE_NORMAL          0x13U
#define CANNM_IND_STATE_WAITBUSSLEEP    0x14U
#define CANNM_IND_STATE_LIMPHOME        0x15U
#define CANNM_IND_STATE_ON              0x16U

/* AUTOSAR NM states (AUTOSAR Nm 4.x, 0x20-0x2F range) */
#define CANNM_AUTOSAR_STATE_UNINIT             0x20U
#define CANNM_AUTOSAR_STATE_BUS_SLEEP          0x21U
#define CANNM_AUTOSAR_STATE_REPEAT_MESSAGE     0x22U
#define CANNM_AUTOSAR_STATE_NORMAL_OPERATION   0x23U
#define CANNM_AUTOSAR_STATE_READY_SLEEP        0x24U
#define CANNM_AUTOSAR_STATE_PREPARE_BUS_SLEEP  0x25U
#define CANNM_AUTOSAR_STATE_SYNCHRONIZE        0x26U

/*=======[ C A N N M   R E T U R N   T Y P E ]===============================*/

typedef uint8 CanNm_ReturnType;
#define CANNM_E_OK              0x00U
#define CANNM_E_NOT_OK          0x01U
#define CANNM_E_PENDING         0x02U

/*=======[ C A N   D R I V E R   A B S T R A C T I O N ]====================*/

/* The CanNm layer does NOT call the CAN driver directly.
 * Instead it uses the following abstract interface, which the
 * integrating project must wire to CanIf / CAN driver.
 */

/* Transmit an NM PDU on the specified channel.
 * Returns: CANNM_E_OK if queued, CANNM_E_NOT_OK on error. */
extern CanNm_ReturnType CanNm_Transmit(
    NetworkHandleType channel,
    const uint8* pduData,
    uint8 pduLength
);

/* Enable/disable CAN controller on the specified channel */
extern void CanNm_ControllerEnable(NetworkHandleType channel);
extern void CanNm_ControllerDisable(NetworkHandleType channel);

/*=======[ C A N N M   V T A B L E   ( p o l y m o r p h i c   d i s p a t c h ) ]===*/

/* Function pointer vtable for NM mode polymorphism.
 * One static const instance per NM type (Direct / Indirect / Autosar).
 * Assigned to Nm_ChannelContextType.canNmVtable at Init time.
 * Eliminates all runtime if/else branching in CanNm.c. */

typedef struct CanNm_VtableType {
    void             (*Init)(const Nm_ChannelConfigType* config, struct Nm_ChannelContextType* ctx);
    void             (*DeInit)(NetworkHandleType channel);
    CanNm_ReturnType (*PassiveStartUp)(NetworkHandleType channel);
    CanNm_ReturnType (*NetworkRequest)(NetworkHandleType channel);
    CanNm_ReturnType (*NetworkRelease)(NetworkHandleType channel);
    void             (*DisableCommunication)(NetworkHandleType channel);
    void             (*EnableCommunication)(NetworkHandleType channel);
    void             (*SetUserData)(NetworkHandleType channel, const uint8* data, uint8 length);
    void             (*GetUserData)(NetworkHandleType channel, uint8* data, uint8* nodeId);
    void             (*GetPduData)(NetworkHandleType channel, uint8* pdu);
    uint8            (*GetNodeIdentifier)(NetworkHandleType channel);
    uint8            (*GetLocalNodeIdentifier)(NetworkHandleType channel);
    boolean          (*CheckRemoteSleepIndication)(NetworkHandleType channel);
    void             (*GetState)(NetworkHandleType channel, Nm_StateType* state, Nm_ModeType* mode);
    void             (*MainFunction)(NetworkHandleType channel);
    void             (*TxConfirmation)(NetworkHandleType channel);
    void             (*RxIndication)(NetworkHandleType channel, const uint8* pduData, uint8 pduLength);
    void             (*ControllerBusOff)(NetworkHandleType channel);
} CanNm_VtableType;

/* Vtable instances — defined in CanNm.c */
extern const CanNm_VtableType CanNm_VtableDirect;
extern const CanNm_VtableType CanNm_VtableIndirect;
extern const CanNm_VtableType CanNm_VtableAutosar;

/*=======[ C A N N M   C O R E   A P I   ( c a l l e d   b y   N m ) ]=====*/

/*****************************************************************************/
/*  CanNm_Init                                                               */
/*  Brief:  Initialize CanNm with channel config. Called by Nm_Init().       */
/*  Param:  channelConfig -- pointer to this channel's config                */
/*****************************************************************************/
void CanNm_Init(const Nm_ChannelConfigType* channelConfig);

/*****************************************************************************/
/*  CanNm_DeInit                                                             */
/*  Brief:  De-initialize CanNm channel. Called by Nm_DeInit().              */
/*  Param:  channel -- channel handle                                        */
/*****************************************************************************/
void CanNm_DeInit(NetworkHandleType channel);

/*****************************************************************************/
/*  CanNm_PassiveStartUp                                                     */
/*  Brief:  Wake-up from Bus-Sleep due to received NM message.               */
/*  Param:  channel -- channel handle                                        */
/*  Return: CANNM_E_OK / CANNM_E_NOT_OK                                      */
/*****************************************************************************/
CanNm_ReturnType CanNm_PassiveStartUp(NetworkHandleType channel);

/*****************************************************************************/
/*  CanNm_NetworkRequest                                                     */
/*  Brief:  Request entry into Network Mode.                                 */
/*  Param:  channel -- channel handle                                        */
/*  Return: CANNM_E_OK / CANNM_E_NOT_OK                                      */
/*****************************************************************************/
CanNm_ReturnType CanNm_NetworkRequest(NetworkHandleType channel);

/*****************************************************************************/
/*  CanNm_NetworkRelease                                                     */
/*  Brief:  Request release of the network (toward Bus-Sleep).               */
/*  Param:  channel -- channel handle                                        */
/*  Return: CANNM_E_OK / CANNM_E_NOT_OK                                      */
/*****************************************************************************/
CanNm_ReturnType CanNm_NetworkRelease(NetworkHandleType channel);

/*****************************************************************************/
/*  CanNm_DisableCommunication                                               */
/*  Brief:  Stop transmitting NM PDUs (silent mode).                         */
/*  Param:  channel -- channel handle                                        */
/*****************************************************************************/
void CanNm_DisableCommunication(NetworkHandleType channel);

/*****************************************************************************/
/*  CanNm_EnableCommunication                                                */
/*  Brief:  Resume transmitting NM PDUs.                                     */
/*  Param:  channel -- channel handle                                        */
/*****************************************************************************/
void CanNm_EnableCommunication(NetworkHandleType channel);

/*****************************************************************************/
/*  CanNm_SetUserData                                                        */
/*  Brief:  Set user data bytes in outgoing NM messages.                     */
/*  Param:  channel -- channel handle                                        */
/*  Param:  data -- user data buffer                                         */
/*  Param:  length -- number of bytes                                        */
/*****************************************************************************/
void CanNm_SetUserData(
    NetworkHandleType channel,
    const uint8* data,
    uint8 length
);

/*****************************************************************************/
/*  CanNm_GetUserData                                                        */
/*  Brief:  Get user data from last received NM message.                     */
/*  Param:  channel -- channel handle                                        */
/*  Param:  data -- output buffer                                            */
/*  Param:  nodeId -- output: source node ID                                 */
/*****************************************************************************/
void CanNm_GetUserData(
    NetworkHandleType channel,
    uint8* data,
    uint8* nodeId
);

/*****************************************************************************/
/*  CanNm_GetPduData                                                         */
/*  Brief:  Get full PDU of last received NM message.                        */
/*  Param:  channel -- channel handle                                        */
/*  Param:  pdu -- output PDU buffer                                         */
/*****************************************************************************/
void CanNm_GetPduData(NetworkHandleType channel, uint8* pdu);

/*****************************************************************************/
/*  CanNm_GetNodeIdentifier                                                  */
/*  Brief:  Get source node ID from last received NM message.                */
/*  Param:  channel -- channel handle                                        */
/*  Return: Node ID (0xFF = invalid)                                         */
/*****************************************************************************/
uint8 CanNm_GetNodeIdentifier(NetworkHandleType channel);

/*****************************************************************************/
/*  CanNm_GetLocalNodeIdentifier                                             */
/*  Brief:  Get the locally configured node ID.                              */
/*  Param:  channel -- channel handle                                        */
/*  Return: Local node ID                                                     */
/*****************************************************************************/
uint8 CanNm_GetLocalNodeIdentifier(NetworkHandleType channel);

/*****************************************************************************/
/*  CanNm_CheckRemoteSleepIndication                                         */
/*  Brief:  Check if all remote nodes are ready to sleep.                    */
/*  Param:  channel -- channel handle                                        */
/*  Return: TRUE if all remote nodes indicate sleep-readiness                */
/*****************************************************************************/
boolean CanNm_CheckRemoteSleepIndication(NetworkHandleType channel);

/*****************************************************************************/
/*  CanNm_GetState                                                           */
/*  Brief:  Get current NM state and mode.                                   */
/*  Param:  channel -- channel handle                                        */
/*  Param:  state -- output: Nm_StateType                                    */
/*  Param:  mode -- output: Nm_ModeType                                      */
/*****************************************************************************/
void CanNm_GetState(
    NetworkHandleType channel,
    Nm_StateType* state,
    Nm_ModeType* mode
);

/*****************************************************************************/
/*  CanNm_MainFunction                                                       */
/*  Brief:  Cyclic processing for one channel. Called by Nm_MainFunction().  */
/*  Param:  channel -- channel handle                                        */
/*****************************************************************************/
void CanNm_MainFunction(NetworkHandleType channel);

/*****************************************************************************/
/*  CanNm_TxConfirmation                                                     */
/*  Brief:  Called when an NM PDU transmission completes.                    */
/*  Param:  channel -- channel handle                                        */
/*****************************************************************************/
void CanNm_TxConfirmation(NetworkHandleType channel);

/*****************************************************************************/
/*  CanNm_RxIndication                                                       */
/*  Brief:  Called when an NM PDU is received.                               */
/*  Param:  channel -- channel handle                                        */
/*  Param:  pduData -- raw PDU data                                          */
/*  Param:  pduLength -- PDU length                                          */
/*****************************************************************************/
void CanNm_RxIndication(
    NetworkHandleType channel,
    const uint8* pduData,
    uint8 pduLength
);

/*****************************************************************************/
/*  CanNm_ControllerBusOff                                                   */
/*  Brief:  Handle CAN controller Bus-Off event.                             */
/*  Param:  channel -- channel handle                                        */
/*****************************************************************************/
void CanNm_ControllerBusOff(NetworkHandleType channel);

/*=======[ C A N N M   - >   N M   C O R E   C A L L B A C K S ]============*/

/* These functions are called by CanNm to notify the Nm Core layer.
 * Implemented in Nm.c */

extern void Nm_Core_StateChangeNotification(
    NetworkHandleType channel,
    Nm_StateType newState
);

extern void Nm_Core_NetworkStartIndication(NetworkHandleType channel);
extern void Nm_Core_NetworkMode(NetworkHandleType channel);
extern void Nm_Core_PrepareBusSleepMode(NetworkHandleType channel);
extern void Nm_Core_BusSleepMode(NetworkHandleType channel);

#endif /* CANNM_H_ */
