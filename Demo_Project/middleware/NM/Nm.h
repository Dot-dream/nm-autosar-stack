/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       Nm.h
 *  @brief      Unified Nm public API -- AUTOSAR-compatible
 *
 *  This module provides a standardized Network Management interface.
 *  It abstracts bus-specific NM implementations (CanNm, LinNm, FrNm)
 *  behind a single Nm_* API, supporting both Direct and Indirect NM modes.
 *
 *  Target AUTOSAR Version: Nm 4.2.2 / OSEK NM 2.5.3 compatible
 *
 *  Design based on analysis of:
 *    - 6AQ0 project (iSOFT NmIf/OsekNm, STM32F103)
 *    - 6ER1 project (Arctic Core OsekNm, GD32F303)
 */
/*============================================================================*/

#ifndef NM_H_
#define NM_H_

/*=======[ I N C L U D E S ]================================================*/

#include "Nm_ConfigTypes.h"
#include "Nm_OsIf.h"

#if (NM_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*=======[ V E R S I O N   I N F O ]=========================================*/

#define NM_MODULE_ID                29U
#define NM_VENDOR_ID                60U
#define NM_INSTANCE_ID              1U

#define NM_AR_MAJOR_VERSION         4U
#define NM_AR_MINOR_VERSION         2U
#define NM_AR_PATCH_VERSION         2U

#define NM_SW_MAJOR_VERSION         1U
#define NM_SW_MINOR_VERSION         0U
#define NM_SW_PATCH_VERSION         0U

/*=======[ E R R O R   C O D E S ]==========================================*/

#if (NM_DEV_ERROR_DETECT == STD_ON)
#define NM_E_UNINIT                 0x00U
#define NM_E_HANDLE_UNDEF           0x01U
#define NM_E_PARAM_POINTER          0x02U
#define NM_E_INVALID_CHANNEL        0x03U
#endif

/*=======[ S E R V I C E   I D S ]==========================================*/

#if (NM_DEV_ERROR_DETECT == STD_ON)
#define NM_SID_INIT                         0x00U
#define NM_SID_PASSIVE_STARTUP              0x01U
#define NM_SID_NETWORK_REQUEST              0x02U
#define NM_SID_NETWORK_RELEASE              0x03U
#define NM_SID_DISABLE_COMMUNICATION        0x04U
#define NM_SID_ENABLE_COMMUNICATION         0x05U
#define NM_SID_SET_USER_DATA                0x06U
#define NM_SID_GET_USER_DATA                0x07U
#define NM_SID_GET_PDU_DATA                 0x08U
#define NM_SID_REPEAT_MESSAGE_REQUEST       0x09U
#define NM_SID_GET_NODE_IDENTIFIER          0x0AU
#define NM_SID_GET_LOCAL_NODE_IDENTIFIER    0x0BU
#define NM_SID_CHECK_REMOTE_SLEEP_IND       0x0DU
#define NM_SID_GET_STATE                    0x0EU
#define NM_SID_GET_VERSION_INFO             0x0FU
#define NM_SID_MAIN_FUNCTION                0x10U
#define NM_SID_DEINIT                       0x11U
#define NM_SID_CONTROLLER_BUS_OFF           0x12U
#endif

/*=======[ P U B L I C   A P I ]============================================*/

/*****************************************************************************/
/*  Nm_Init                                                                  */
/*  Brief:        Initialize the NM module with configuration.               */
/*  Param[in]:    configPtr -- pointer to Nm_ConfigType (must be static)     */
/*  ServiceId:    0x00                                                       */
/*****************************************************************************/
void Nm_Init(const Nm_ConfigType* configPtr);

/*****************************************************************************/
/*  Nm_DeInit                                                                */
/*  Brief:        De-initialize all NM channels.                             */
/*  ServiceId:    0x11                                                       */
/*****************************************************************************/
#if (NM_DEINIT_API == STD_ON)
void Nm_DeInit(void);
#endif

/*****************************************************************************/
/*  Nm_PassiveStartUp                                                        */
/*  Brief:        Trigger passive wake-up from Bus-Sleep to Network Mode.    */
/*  Param[in]:    nmChannelHandle                                            */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x01                                                       */
/*****************************************************************************/
Nm_ReturnType Nm_PassiveStartUp(NetworkHandleType nmChannelHandle);

/*****************************************************************************/
/*  Nm_NetworkRequest                                                        */
/*  Brief:        Request the NM to enter Network Mode (begin sending        */
/*                periodic NM messages).                                     */
/*  Param[in]:    nmChannelHandle                                            */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x02                                                       */
/*****************************************************************************/
Nm_ReturnType Nm_NetworkRequest(NetworkHandleType nmChannelHandle);

/*****************************************************************************/
/*  Nm_NetworkRelease                                                        */
/*  Brief:        Request the NM to release the network (transition toward   */
/*                Bus-Sleep).                                                */
/*  Param[in]:    nmChannelHandle                                            */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x03                                                       */
/*****************************************************************************/
Nm_ReturnType Nm_NetworkRelease(NetworkHandleType nmChannelHandle);

/*****************************************************************************/
/*  Nm_DisableCommunication                                                  */
/*  Brief:        Disable NM PDU transmission (node goes silent).            */
/*                Reception and state tracking continue.                     */
/*  Param[in]:    nmChannelHandle                                            */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x04                                                       */
/*****************************************************************************/
#if (NM_COM_CONTROL_ENABLED == STD_ON)
Nm_ReturnType Nm_DisableCommunication(NetworkHandleType nmChannelHandle);
#endif

/*****************************************************************************/
/*  Nm_EnableCommunication                                                   */
/*  Brief:        Re-enable NM PDU transmission after DisableCommunication.  */
/*  Param[in]:    nmChannelHandle                                            */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x05                                                       */
/*****************************************************************************/
#if (NM_COM_CONTROL_ENABLED == STD_ON)
Nm_ReturnType Nm_EnableCommunication(NetworkHandleType nmChannelHandle);
#endif

/*****************************************************************************/
/*  Nm_SetUserData                                                           */
/*  Brief:        Set user data bytes in outgoing NM messages.               */
/*  Param[in]:    nmChannelHandle                                            */
/*  Param[in]:    nmUserDataPtr -- pointer to user data buffer               */
/*  Param[in]:    nmUserDataLength -- number of bytes (max NM_USER_DATA_MAX) */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x06                                                       */
/*****************************************************************************/
#if (NM_USER_DATA_ENABLED == STD_ON)
Nm_ReturnType Nm_SetUserData(
    NetworkHandleType nmChannelHandle,
    const uint8* nmUserDataPtr,
    uint8 nmUserDataLength
);
#endif

/*****************************************************************************/
/*  Nm_GetUserData                                                           */
/*  Brief:        Get user data from the most recently received NM message.  */
/*  Param[in]:    nmChannelHandle                                            */
/*  Param[out]:   nmUserDataPtr -- buffer to receive user data               */
/*  Param[out]:   nmNodeIdPtr -- source node ID                              */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x07                                                       */
/*****************************************************************************/
#if (NM_USER_DATA_ENABLED == STD_ON)
Nm_ReturnType Nm_GetUserData(
    NetworkHandleType nmChannelHandle,
    uint8* nmUserDataPtr,
    uint8* nmNodeIdPtr
);
#endif

/*****************************************************************************/
/*  Nm_GetPduData                                                            */
/*  Brief:        Get the complete PDU of the last received NM message.      */
/*  Param[in]:    nmChannelHandle                                            */
/*  Param[out]:   nmPduData -- buffer to receive full PDU                   */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x08                                                       */
/*****************************************************************************/
#if (NM_USER_DATA_ENABLED == STD_ON) || (NM_NODE_ID_ENABLED == STD_ON) \
    || (NM_NODE_DETECTION_ENABLED == STD_ON)
Nm_ReturnType Nm_GetPduData(
    NetworkHandleType nmChannelHandle,
    uint8* nmPduData
);
#endif

/*****************************************************************************/
/*  Nm_RepeatMessageRequest                                                  */
/*  Brief:        Set Repeat Message Request bit for next outgoing NM msg.   */
/*  Param[in]:    nmChannelHandle                                            */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x09                                                       */
/*****************************************************************************/
#if (NM_NODE_DETECTION_ENABLED == STD_ON)
Nm_ReturnType Nm_RepeatMessageRequest(NetworkHandleType nmChannelHandle);
#endif

/*****************************************************************************/
/*  Nm_GetNodeIdentifier                                                     */
/*  Brief:        Get source node ID from the last received NM message.      */
/*  Param[in]:    nmChannelHandle                                            */
/*  Param[out]:   nmNodeIdPtr                                                */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x0A                                                       */
/*****************************************************************************/
#if (NM_NODE_ID_ENABLED == STD_ON)
Nm_ReturnType Nm_GetNodeIdentifier(
    NetworkHandleType nmChannelHandle,
    uint8* nmNodeIdPtr
);
#endif

/*****************************************************************************/
/*  Nm_GetLocalNodeIdentifier                                                */
/*  Brief:        Get the locally configured node ID for this channel.       */
/*  Param[in]:    nmChannelHandle                                            */
/*  Param[out]:   nmNodeIdPtr                                                */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x0B                                                       */
/*****************************************************************************/
#if (NM_NODE_ID_ENABLED == STD_ON)
Nm_ReturnType Nm_GetLocalNodeIdentifier(
    NetworkHandleType nmChannelHandle,
    uint8* nmNodeIdPtr
);
#endif

/*****************************************************************************/
/*  Nm_CheckRemoteSleepIndication                                            */
/*  Brief:        Check if all remote nodes are ready to sleep.              */
/*  Param[in]:    nmChannelHandle                                            */
/*  Param[out]:   nmRemoteSleepIndPtr -- TRUE if all nodes ready             */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x0D                                                       */
/*****************************************************************************/
#if (NM_REMOTE_SLEEP_IND_ENABLED == STD_ON)
Nm_ReturnType Nm_CheckRemoteSleepIndication(
    NetworkHandleType nmChannelHandle,
    boolean* nmRemoteSleepIndPtr
);
#endif

/*****************************************************************************/
/*  Nm_GetState                                                              */
/*  Brief:        Get the current NM state and mode for a channel.           */
/*  Param[in]:    nmChannelHandle                                            */
/*  Param[out]:   nmStatePtr -- current Nm_StateType                         */
/*  Param[out]:   nmModePtr -- current Nm_ModeType                           */
/*  Return:       NM_E_OK / NM_E_NOT_OK                                      */
/*  ServiceId:    0x0E                                                       */
/*****************************************************************************/
Nm_ReturnType Nm_GetState(
    NetworkHandleType nmChannelHandle,
    Nm_StateType* nmStatePtr,
    Nm_ModeType* nmModePtr
);

/*****************************************************************************/
/*  Nm_GetVersionInfo                                                        */
/*  Brief:        Get module version information.                            */
/*  Param[out]:   versionInfo -- pointer to Std_VersionInfoType              */
/*  ServiceId:    0x0F                                                       */
/*****************************************************************************/
#if (NM_VERSION_INFO_API == STD_ON)
void Nm_GetVersionInfo(Std_VersionInfoType* versionInfo);
#endif

/*****************************************************************************/
/*  Nm_MainFunction                                                          */
/*  Brief:        Cyclic NM processing. Call periodically (e.g., every 5ms). */
/*  ServiceId:    0x10                                                       */
/*****************************************************************************/
void Nm_MainFunction(void);

/*****************************************************************************/
/*  Nm_ControllerBusOff                                                      */
/*  Brief:        Notify NM of a CAN controller Bus-Off event.               */
/*                NM transitions affected channel to LimpHome state.         */
/*  Param[in]:    nmChannelHandle                                            */
/*  ServiceId:    0x12                                                       */
/*****************************************************************************/
void Nm_ControllerBusOff(NetworkHandleType nmChannelHandle);

/*****************************************************************************/
/*  Nm_TxConfirmation                                                        */
/*  Brief:        Callback from CAN driver: NM PDU transmission confirmed.   */
/*  Param[in]:    nmChannelHandle                                            */
/*****************************************************************************/
void Nm_TxConfirmation(NetworkHandleType nmChannelHandle);

/*****************************************************************************/
/*  Nm_RxIndication                                                          */
/*  Brief:        Callback from CAN driver: NM PDU received.                 */
/*  Param[in]:    nmChannelHandle                                            */
/*  Param[in]:    nmPduData -- pointer to raw PDU bytes                      */
/*  Param[in]:    nmPduLength -- number of bytes in PDU                      */
/*****************************************************************************/
void Nm_RxIndication(
    NetworkHandleType nmChannelHandle,
    const uint8* nmPduData,
    uint8 nmPduLength
);

#endif /* NM_H_ */

