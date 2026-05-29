/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       Nm_ConfigTypes.h
 *  @brief      NM configuration type definitions
 *
 *  All NM module configuration is centralized in Nm_ConfigType.
 *  This structure is passed to Nm_Init() once at startup and must
 *  reside in static/ROM memory.
 *
 *  Design covers both:
 *    - CAN ID-based message format (6AQ0 style: 0x400-0x4FF)
 *    - OpCode bitfield-based format (6ER1 Arctic Core style)
 *    - CFMOTO-specific timer extensions (6ER1)
 */
/*============================================================================*/

#ifndef NM_CONFIGTYPES_H_
#define NM_CONFIGTYPES_H_

#ifndef NM_HOST_TEST
#include "Std_Types.h"
#else
/* Host test shim — provides types normally from Std_Types.h */
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef unsigned char  boolean;
#define TRUE   1U
#define FALSE  0U
#ifndef NULL
#define NULL   ((void*)0)
#endif
#define STD_ON   1U
#define STD_OFF  0U
#endif

/*=======[ C O M P I L E - T I M E   S W I T C H E S ]======================*/

/* These MUST be defined by the integrating project (app_cfg.h or similar) */

#ifndef NM_DEV_ERROR_DETECT
#define NM_DEV_ERROR_DETECT         STD_OFF
#endif

#ifndef NM_VERSION_INFO_API
#define NM_VERSION_INFO_API         STD_ON
#endif

#ifndef NM_DEINIT_API
#define NM_DEINIT_API               STD_OFF
#endif

#ifndef NM_COM_CONTROL_ENABLED
#define NM_COM_CONTROL_ENABLED      STD_ON
#endif

#ifndef NM_USER_DATA_ENABLED
#define NM_USER_DATA_ENABLED        STD_ON
#endif

#ifndef NM_NODE_ID_ENABLED
#define NM_NODE_ID_ENABLED          STD_ON
#endif

#ifndef NM_NODE_DETECTION_ENABLED
#define NM_NODE_DETECTION_ENABLED   STD_ON
#endif

#ifndef NM_REMOTE_SLEEP_IND_ENABLED
#define NM_REMOTE_SLEEP_IND_ENABLED STD_ON
#endif

#ifndef NM_STATE_CHANGE_IND_ENABLED
#define NM_STATE_CHANGE_IND_ENABLED STD_ON
#endif

#ifndef NM_PDU_RX_INDICATION_ENABLED
#define NM_PDU_RX_INDICATION_ENABLED STD_ON
#endif

#ifndef NM_BUS_SYNCHRONIZATION_ENABLED
#define NM_BUS_SYNCHRONIZATION_ENABLED STD_OFF
#endif

#ifndef NM_IMMEDIATE_TXCONF_ENABLED
#define NM_IMMEDIATE_TXCONF_ENABLED STD_OFF
#endif

#ifndef NM_BUS_LOAD_REDUCTION_ENABLED
#define NM_BUS_LOAD_REDUCTION_ENABLED STD_ON
#endif

#ifndef NM_PASSIVE_MODE_ENABLED
#define NM_PASSIVE_MODE_ENABLED     STD_OFF
#endif

#ifndef NM_TX_PDU_PENDING_ENABLE
#define NM_TX_PDU_PENDING_ENABLE    STD_ON
#endif

/*=======[ C O N S T A N T S ]==============================================*/

#define NM_MAX_CHANNELS             8U
#define NM_MAX_NODES                32U
#define NM_PDU_MAX_LENGTH           8U
#define NM_USER_DATA_MAX            6U   /* 8-byte PDU minus OpCode + NodeID */
#define NM_INVALID_HANDLE           0xFFU
#define NM_INVALID_NODE_ID          0xFFU

/*=======[ W I R E   F O R M A T   S E L E C T I O N ]======================*/

/* The NM PDU wire format differs between the two codebases:
 *
 *   Mode 1 (CAN_ID_MODE):   Each NM message type maps to a unique CAN ID
 *                           within a configured range (e.g., 0x400-0x4FF).
 *                           PDU content = OpCode byte + user data.
 *                           Used by: 6AQ0 (iSOFT OsekNm 2.5.3)
 *
 *   Mode 2 (OPCODE_MODE):   A single CAN ID is used for all NM messages.
 *                           The first byte carries an OpCode bitfield
 *                           distinguishing Alive/Ring/LimpHome.
 *                           PDU content = OpCode byte + user data.
 *                           Used by: 6ER1 (Arctic Core OSEK NM 4.2.2)
 */

#define NM_WIRE_FORMAT_CAN_ID       0x01U
#define NM_WIRE_FORMAT_OPCODE       0x02U

/*=======[ C A N   I D   M O D E   C O N F I G ]============================*/

/* Message type opcodes (CAN_ID mode) */
#define NM_OPCODE_ALIVE             0x01U
#define NM_OPCODE_RING              0x02U
#define NM_OPCODE_LIMPHOME          0x04U
#define NM_OPCODE_ALIVE_IND         0x11U
#define NM_OPCODE_ALIVE_IND_ACK     0x31U
#define NM_OPCODE_RING_IND          0x12U
#define NM_OPCODE_RING_IND_ACK      0x32U
#define NM_OPCODE_RING_ACK          0x22U
#define NM_OPCODE_LIMPHOME_IND      0x14U
#define NM_OPCODE_LIMPHOME_IND_ACK  0x34U
#define NM_OPCODE_MASK              0x37U  /* Bits 0,1,2,4,5 */

/*=======[ O P C O D E   M O D E   C O N F I G ]============================*/

/* Message type opcodes (OpCode bitfield mode) */
#define NM_OP_ALIVE_BIT             0x01U
#define NM_OP_RING_BIT              0x02U
#define NM_OP_LIMPHOME_BIT          0x04U
#define NM_OP_SLEEP_ACK_BIT         0x08U
#define NM_OP_SLEEP_IND_BIT         0x10U
#define NM_OP_REPEAT_MSG_BIT        0x20U
#define NM_OP_ACTIVE_BIT            0x40U
#define NM_OP_RESERVED_BIT          0x80U

/*=======[ T Y P E   D E F I N I T I O N S ]=================================*/

/* --- Base types (single point of definition, used by all NM headers) --- */

typedef uint8 NetworkHandleType;

typedef uint8 Nm_ReturnType;
#define NM_E_OK                 0x00U
#define NM_E_NOT_OK             0x01U
#define NM_E_NOT_EXECUTED       0x02U

/* NM state enumeration -- superset of both projects' states */
typedef uint8 Nm_StateType;
#define NM_STATE_UNINIT             0x00U
#define NM_STATE_BUS_SLEEP          0x01U
#define NM_STATE_PREPARE_BUS_SLEEP  0x02U
#define NM_STATE_READY_SLEEP        0x03U
#define NM_STATE_NORMAL_OPERATION   0x04U
#define NM_STATE_REPEAT_MESSAGE     0x05U
#define NM_STATE_SYNCHRONIZE        0x06U
#define NM_STATE_LIMPHOME           0x07U
#define NM_STATE_LIMPHOME_PREPSLEEP 0x08U
#define NM_STATE_TWBS_LIMPHOME      0x09U
#define NM_STATE_INITRESET          0x0AU
#define NM_STATE_TWBS_NORMAL        0x0BU
#define NM_STATE_ON                 0x0CU

typedef uint8 Nm_ModeType;
#define NM_MODE_BUS_SLEEP           0x00U
#define NM_MODE_PREPARE_BUS_SLEEP   0x01U
#define NM_MODE_SYNCHRONIZE         0x02U
#define NM_MODE_NETWORK             0x03U

typedef uint8 Nm_BusType;
#define NM_BUS_CAN                  0x00U
#define NM_BUS_LIN                  0x01U
#define NM_BUS_FR                   0x02U

typedef uint8 Nm_NmModeType;
#define NM_MODE_DIRECT              0x00U
#define NM_MODE_INDIRECT            0x01U
#define NM_MODE_AUTOSAR             0x02U

/* --- Additional general types --- */

typedef uint8  NodeIdType;
typedef uint8  NetIdType;

/* PDU type: raw 8 bytes */
typedef struct {
    uint8 data[NM_PDU_MAX_LENGTH];
    uint8 length;
} Nm_PduType;

/* Timer value in milliseconds (platform tick conversion in OsIf layer) */
typedef uint32 Nm_TimerMsType;

/* --- Wire format configuration --- */

typedef struct {
    uint8  wireFormat;                     /* NM_WIRE_FORMAT_CAN_ID or NM_WIRE_FORMAT_OPCODE */
    /* CAN_ID mode: base + max CAN ID range (e.g., 0x400-0x4FF) */
    uint16 canIdBase;                      /* Base CAN ID for NM messages */
    uint16 canIdMax;                       /* Max CAN ID for NM messages */
    /* OpCode mode: single CAN ID for all NM messages */
    uint16 singleCanId;                    /* Single CAN ID (when wireFormat == NM_WIRE_FORMAT_OPCODE) */
    /* Common: PDU OpCode values used by both wire formats */
    uint8  pduOpCodeAlive;                 /* OpCode for Alive message */
    uint8  pduOpCodeRing;                  /* OpCode for Ring message */
    uint8  pduOpCodeLimpHome;              /* OpCode for LimpHome message */
} Nm_WireFormatConfigType;

/* --- Channel configuration --- */

typedef struct {
    /* Identity */
    NetworkHandleType channelHandle;       /* 0..NM_MAX_CHANNELS-1 */
    uint8             nodeId;              /* Local node ID on this channel */
    Nm_BusType        busType;             /* NM_BUS_CAN / NM_BUS_LIN / NM_BUS_FR */
    Nm_NmModeType     nmMode;              /* NM_MODE_DIRECT / NM_MODE_INDIRECT */

    /* Wire format (CAN ID mode vs OpCode mode) */
    Nm_WireFormatConfigType wireConfig;

    /* Timing parameters (in milliseconds) */
    Nm_TimerMsType timerTyp;               /* Typical message cycle time */
    Nm_TimerMsType timerMax;               /* Max timeout for receiving NM msg */
    Nm_TimerMsType timerError;             /* LimpHome error timeout */
    Nm_TimerMsType timerWaitBusSleep;      /* Wait Bus-Sleep timeout (TWbs) */
    Nm_TimerMsType timerTx;                /* Tx retry timeout (TTx) */
    Nm_TimerMsType timerNmMsgCycle;        /* NM message cycle offset */

    /* Retry limits */
    uint8  rxCountLimit;                   /* Max Rx retries before timeout */
    uint8  txCountLimit;                   /* Max Tx retries before error */

    /* Bus load reduction */
    uint8  busLoadReductionActive;         /* Enable message cycle time increase */
    uint8  busLoadReductionFactor;         /* Multiplier for TTyp during BLR */

    /* Immediate Tx confirmation */
    uint8  immediateTxConfEnabled;         /* Confirm Tx immediately or wait */

    /* Bus synchronization */
    uint8  busSyncEnabled;                 /* Coordinator sync enabled */

    /* --- CFMOTO-specific extensions (from 6ER1) --- */
    uint8  hasCfmotoExtensions;            /* TRUE if cfmoto timers are in use */
    Nm_TimerMsType cfmotoTSleepRequestMin; /* Min sleep request duration */
    Nm_TimerMsType cfmotoTLimpSleepReqMin; /* Min LimpHome sleep request */
    Nm_TimerMsType cfmotoTLimpHomeDTC;     /* LimpHome DTC timeout */

    /* --- Callback function pointers --- */
    void (*stateChangeCallback)(NetworkHandleType channel, Nm_StateType newState);
    void (*pduRxCallback)(NetworkHandleType channel, const Nm_PduType* pdu);
} Nm_ChannelConfigType;

/* --- Global NM configuration --- */

typedef struct {
    uint8              numChannels;        /* Number of active channels */
    const Nm_ChannelConfigType* channels;  /* Pointer to channel config array */
    uint32             mainFunctionPeriodMs; /* Nm_MainFunction() call period */
} Nm_ConfigType;

/* Std_VersionInfoType (minimal definition -- integrate with project's Std_Types.h) */
#ifndef STD_VERSION_INFO_TYPE_DEFINED
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8  instanceID;
    uint8  sw_major_version;
    uint8  sw_minor_version;
    uint8  sw_patch_version;
    uint8  ar_major_version;
    uint8  ar_minor_version;
    uint8  ar_patch_version;
} Std_VersionInfoType;
#define STD_VERSION_INFO_TYPE_DEFINED
#endif

#endif /* NM_CONFIGTYPES_H_ */



