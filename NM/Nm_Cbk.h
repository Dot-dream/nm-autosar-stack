/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       Nm_Cbk.h
 *  @brief      NM callback function declarations (upward notifications)
 *
 *  The application layer (ComM / EcuM / mode manager) implements these
 *  callbacks. NM calls them to notify the upper layer of state changes
 *  and events.
 */
/*============================================================================*/

#ifndef NM_CBK_H_
#define NM_CBK_H_

#include "Nm_ConfigTypes.h"

/*=======[ S T A T E   N O T I F I C A T I O N   C A L L B A C K S ]========*/

/*****************************************************************************/
/*  Nm_NetworkStartIndication                                                */
/*  Brief: A NM message was received while in Bus-Sleep (wake-up event).     */
/*  Param: nmChannelHandle                                                   */
/*****************************************************************************/
void Nm_NetworkStartIndication(NetworkHandleType nmChannelHandle);

/*****************************************************************************/
/*  Nm_NetworkMode                                                           */
/*  Brief: NM has entered Network Mode (Normal Operation or Repeat Message). */
/*  Param: nmChannelHandle                                                   */
/*****************************************************************************/
void Nm_NetworkMode(NetworkHandleType nmChannelHandle);

/*****************************************************************************/
/*  Nm_PrepareBusSleepMode                                                   */
/*  Brief: NM has entered Prepare Bus-Sleep (network release in progress).   */
/*  Param: nmChannelHandle                                                   */
/*****************************************************************************/
void Nm_PrepareBusSleepMode(NetworkHandleType nmChannelHandle);

/*****************************************************************************/
/*  Nm_BusSleepMode                                                          */
/*  Brief: NM has entered Bus-Sleep mode (network is asleep).                */
/*  Param: nmChannelHandle                                                   */
/*****************************************************************************/
void Nm_BusSleepMode(NetworkHandleType nmChannelHandle);

/*****************************************************************************/
/*  Nm_RestartIndication                                                     */
/*  Brief: NM coordinator requests a restart (optional).                     */
/*  Param: nmChannelHandle                                                   */
/*****************************************************************************/
#if (NM_BUS_SYNCHRONIZATION_ENABLED == STD_ON)
void Nm_RestartIndication(NetworkHandleType nmChannelHandle);
#endif

/*****************************************************************************/
/*  Nm_StateChangeNotification                                               */
/*  Brief: NM state has changed (with previous and new state).               */
/*         This is the preferred callback when                                */
/*         NM_STATE_CHANGE_IND_ENABLED is ON.                                 */
/*  Param: nmChannelHandle                                                   */
/*  Param: previousState                                                     */
/*  Param: newState                                                          */
/*****************************************************************************/
#if (NM_STATE_CHANGE_IND_ENABLED == STD_ON)
void Nm_StateChangeNotification(
    NetworkHandleType nmChannelHandle,
    Nm_StateType previousState,
    Nm_StateType newState
);
#endif

/*****************************************************************************/
/*  Nm_CoordReadyToSleep                                                     */
/*  Brief: Coordinator has determined all nodes are ready to sleep.          */
/*         Only meaningful when NM_BUS_SYNCHRONIZATION_ENABLED is ON.        */
/*  Param: nmChannelHandle                                                   */
/*****************************************************************************/
#if (NM_BUS_SYNCHRONIZATION_ENABLED == STD_ON)
void Nm_CoordReadyToSleep(NetworkHandleType nmChannelHandle);
#endif

/*=======[ D A T A   N O T I F I C A T I O N   C A L L B A C K S ]=========*/

/*****************************************************************************/
/*  Nm_PduRxIndication                                                       */
/*  Brief: An NM PDU has been received. The application may inspect the PDU. */
/*  Param: nmChannelHandle                                                   */
/*****************************************************************************/
#if (NM_PDU_RX_INDICATION_ENABLED == STD_ON)
void Nm_PduRxIndication(NetworkHandleType nmChannelHandle);
#endif

/*****************************************************************************/
/*  Nm_RemoteSleepIndication                                                 */
/*  Brief: A remote node has indicated readiness to sleep.                   */
/*         Only meaningful in coordinator mode.                              */
/*  Param: nmChannelHandle                                                   */
/*****************************************************************************/
#if (NM_REMOTE_SLEEP_IND_ENABLED == STD_ON)
void Nm_RemoteSleepIndication(NetworkHandleType nmChannelHandle);
#endif

/*****************************************************************************/
/*  Nm_RemoteSleepCancellation                                               */
/*  Brief: A remote node that previously indicated sleep-ready has           */
/*         cancelled that indication.                                        */
/*  Param: nmChannelHandle                                                   */
/*****************************************************************************/
#if (NM_REMOTE_SLEEP_IND_ENABLED == STD_ON)
void Nm_RemoteSleepCancellation(NetworkHandleType nmChannelHandle);
#endif

/*=======[ E R R O R   N O T I F I C A T I O N   C A L L B A C K S ]=======*/

/*****************************************************************************/
/*  Nm_TxTimeoutException                                                    */
/*  Brief: NM PDU transmission has timed out.                                */
/*  Param: nmChannelHandle                                                   */
/*****************************************************************************/
void Nm_TxTimeoutException(NetworkHandleType nmChannelHandle);

/*****************************************************************************/
/*  Nm_LimpHomeIndication                                                    */
/*  Brief: NM has entered LimpHome state due to bus errors.                  */
/*         Application may take recovery action (e.g., log DTC).             */
/*  Param: nmChannelHandle                                                   */
/*****************************************************************************/
void Nm_LimpHomeIndication(NetworkHandleType nmChannelHandle);

#endif /* NM_CBK_H_ */
