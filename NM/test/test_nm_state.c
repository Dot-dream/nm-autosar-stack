/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       test_nm_state.c
 *  @brief      PC host test for Direct NM state machine
 *
 *  This test runs on PC (no hardware required). It validates:
 *    1. State transitions: OFF -> INIT -> INITRESET -> NORMAL -> PREPSLEEP -> BUSSLEEP
 *    2. LimpHome entry/exit on TMax expiry
 *    3. Wake-up from Bus-Sleep on received NM message
 *    4. Communication disable/enable
 *
 *  Compile (gcc, all 3 NM types):
 *    gcc -std=c11 -DNM_HOST_TEST -I.. -I../OsIf test_nm_state.c ../Nm.c ../CanNm/CanNm.c ../CanNm/CanNm_Osek_Direct.c ../CanNm/CanNm_Osek_Indirect.c ../CanNm/CanNm_Autosar.c ../Nm_Timer/Nm_Timer.c -o test_nm_all
 */
/*============================================================================*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*=======[ H O S T   T E S T   P L A T F O R M   S H I M S ]================*/

/* Simulated platform types */
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

/* Include NM headers with host test overrides */
#define NM_DEV_ERROR_DETECT             STD_OFF
#define NM_VERSION_INFO_API             STD_OFF
#define NM_DEINIT_API                   STD_OFF
#define NM_COM_CONTROL_ENABLED          STD_ON
#define NM_USER_DATA_ENABLED            STD_ON
#define NM_NODE_ID_ENABLED              STD_ON
#define NM_NODE_DETECTION_ENABLED       STD_ON
#define NM_REMOTE_SLEEP_IND_ENABLED     STD_ON
#define NM_STATE_CHANGE_IND_ENABLED     STD_ON
#define NM_PDU_RX_INDICATION_ENABLED    STD_ON
#define NM_BUS_SYNCHRONIZATION_ENABLED  STD_OFF
#define NM_IMMEDIATE_TXCONF_ENABLED     STD_OFF

/*=======[ H O S T   T E S T   O S   A B S T R A C T I O N ]================*/
/* Must be defined BEFORE including Nm_OsIf.h (which #errors if they're missing) */

static uint32 g_criticalNest = 0U;
void Nm_EnterCritical(void)  { g_criticalNest++; }
void Nm_ExitCritical(void)   { if (g_criticalNest > 0U) g_criticalNest--; }

/*=======[ N M   H E A D E R S ]=============================================*/

#include "Nm.h"
#include "Nm_Internal.h"
#include "Nm_Cbk.h"
#include "Nm_OsIf.h"
#include "Nm_Timer/Nm_Timer.h"

/*=======[ H O S T   T E S T   T I M E R   S H I M ]=========================*/

static uint32 g_systemTickMs = 0U;

/* Nm_Timer platform tick source */
uint32 Nm_Timer_GetTick(void) { return g_systemTickMs; }


/*=======[ H O S T   T E S T   C A N   S H I M ]=============================*/

static uint8  g_lastTxPdu[NM_PDU_MAX_LENGTH];
static uint8  g_lastTxLength;
static uint8  g_txCount;
static uint8  g_controllerEnabled;

CanNm_ReturnType CanNm_Transmit(NetworkHandleType channel, const uint8* pduData, uint8 pduLength)
{
    uint8 i;
    (void)channel;
    if (NULL == pduData || pduLength > NM_PDU_MAX_LENGTH) { return CANNM_E_NOT_OK; }
    for (i = 0; i < pduLength; i++) { g_lastTxPdu[i] = pduData[i]; }
    g_lastTxLength = pduLength;
    g_txCount++;
    return CANNM_E_OK;
}

void CanNm_ControllerEnable(NetworkHandleType channel)  { (void)channel; g_controllerEnabled = 1U; }
void CanNm_ControllerDisable(NetworkHandleType channel) { (void)channel; g_controllerEnabled = 0U; }

/*=======[ H O S T   T E S T   C A L L B A C K   T R A C K I N G ]==========*/

static uint8 g_cbNetworkStartCount;
static uint8 g_cbNetworkModeCount;
static uint8 g_cbPrepareBusSleepCount;
static uint8 g_cbBusSleepCount;
static uint8 g_lastStateChangePrev;
static uint8 g_lastStateChangeNew;

void Nm_NetworkStartIndication(NetworkHandleType ch)   { (void)ch; g_cbNetworkStartCount++; }
void Nm_NetworkMode(NetworkHandleType ch)              { (void)ch; g_cbNetworkModeCount++; }
void Nm_PrepareBusSleepMode(NetworkHandleType ch)      { (void)ch; g_cbPrepareBusSleepCount++; }
void Nm_BusSleepMode(NetworkHandleType ch)             { (void)ch; g_cbBusSleepCount++; }
void Nm_StateChangeNotification(NetworkHandleType ch, Nm_StateType prev, Nm_StateType cur) {
    (void)ch; g_lastStateChangePrev = prev; g_lastStateChangeNew = cur;
}
void Nm_PduRxIndication(NetworkHandleType ch)          { (void)ch; }
void Nm_RemoteSleepIndication(NetworkHandleType ch)    { (void)ch; }
void Nm_RemoteSleepCancellation(NetworkHandleType ch)  { (void)ch; }
void Nm_TxTimeoutException(NetworkHandleType ch)       { (void)ch; }
void Nm_LimpHomeIndication(NetworkHandleType ch)       { (void)ch; }

/*=======[ T E S T   H E L P E R S ]=========================================*/

static void Test_Reset(void)
{
    memset(&Nm_Core, 0, sizeof(Nm_Core));
    memset(&g_lastTxPdu, 0, sizeof(g_lastTxPdu));
    g_systemTickMs         = 0U;
    g_txCount              = 0U;
    g_controllerEnabled    = 0U;
    g_cbNetworkStartCount  = 0U;
    g_cbNetworkModeCount   = 0U;
    g_cbPrepareBusSleepCount = 0U;
    g_cbBusSleepCount      = 0U;
}

static void Test_AdvanceTime(uint32 ms)
{
    /* Call MainFunction every 5ms to simulate real operation */
    uint32 steps = ms / 5U;
    uint32 i;
    for (i = 0; i < steps; i++) {
        g_systemTickMs += 5U;
        Nm_MainFunction();
    }
}

/* Simulate receiving an NM message */
static void Test_RxNmMessage(uint8 opCode, uint8 srcNodeId)
{
    uint8 pdu[8] = {0};
    pdu[0] = opCode;
    pdu[1] = srcNodeId;
    Nm_RxIndication(0, pdu, 8);
}

/*=======[ T E S T   C A S E S ]=============================================*/

static Nm_ChannelConfigType g_channel = {
    .channelHandle             = 0,
    .nodeId                    = 0x0A,
    .busType                   = NM_BUS_CAN,
    .nmMode                    = NM_MODE_DIRECT,
    .wireConfig = {
        .wireFormat    = NM_WIRE_FORMAT_OPCODE,
        .singleCanId   = 0x500,
        .pduOpCodeAlive   = 0x01,
        .pduOpCodeRing    = 0x02,
        .pduOpCodeLimpHome = 0x04,
    },
    .timerTyp               = 1000U,   /* 1s cycle */
    .timerMax               = 2000U,   /* 2s receive timeout */
    .timerError             = 1000U,   /* 1s LimpHome cycle */
    .timerWaitBusSleep      = 2000U,   /* 2s wait bus sleep */
    .timerTx                = 100U,    /* 100ms tx retry */
    .rxCountLimit           = 4U,
    .txCountLimit           = 8U,
    .busLoadReductionActive = 0U,
    .hasCfmotoExtensions    = 0U,
    .stateChangeCallback    = NULL,
    .pduRxCallback          = NULL,
};

static Nm_ConfigType g_config = {
    .numChannels           = 1U,
    .channels              = &g_channel,
    .mainFunctionPeriodMs  = 5U,
};

/*=======[ T e s t   1 :  B a s i c   s t a t e   t r a n s i t i o n s ]===*/

static void Test1_BasicStateTransitions(void)
{
    Nm_StateType state;
    Nm_ModeType  mode;

    printf("Test 1: Basic state transitions ... ");

    Test_Reset();
    Nm_Init(&g_config);

    /* After init, should be in Bus-Sleep (Init sets up channel, state = BUS_SLEEP) */
    Nm_GetState(0, &state, &mode);
    assert(state == NM_STATE_BUS_SLEEP);

    /* NetworkRequest -> enter NORMAL_OPERATION via INITRESET sequence */
    Nm_NetworkRequest(0);
    Test_AdvanceTime(2000);  /* Pass INITRESET (TTyp = 1000ms) */

    Nm_GetState(0, &state, &mode);
    assert(state == NM_STATE_NORMAL_OPERATION);
    assert(mode == NM_MODE_NETWORK);
    assert(g_txCount > 0);  /* Should have transmitted Alive + Ring */

    /* NetworkRelease -> PREPARE_BUS_SLEEP */
    Nm_NetworkRelease(0);
    Nm_GetState(0, &state, &mode);
    assert(state == NM_STATE_PREPARE_BUS_SLEEP);

    /* Wait TWbs -> BUSSLEEP */
    Test_AdvanceTime(2500);
    Nm_GetState(0, &state, &mode);
    assert(state == NM_STATE_BUS_SLEEP);

    printf("PASSED\n");
}

/*=======[ T e s t   2 :  L i m p H o m e   e n t r y ]=====================*/

static void Test2_LimpHomeEntry(void)
{
    Nm_StateType state;
    Nm_ModeType  mode;

    printf("Test 2: LimpHome entry on TMax ... ");

    Test_Reset();
    Nm_Init(&g_config);
    Nm_NetworkRequest(0);
    Test_AdvanceTime(2000);  /* Through INITRESET */

    /* Advance beyond TMax without receiving any message */
    Test_AdvanceTime(3000);  /* TMax = 2000ms */

    Nm_GetState(0, &state, &mode);
    assert(state == NM_STATE_LIMPHOME);

    printf("PASSED\n");
}

/*=======[ T e s t   3 :  L i m p H o m e   r e c o v e r y ]===============*/

static void Test3_LimpHomeRecovery(void)
{
    Nm_StateType state;
    Nm_ModeType  mode;

    printf("Test 3: LimpHome recovery on message ... ");

    Test_Reset();
    Nm_Init(&g_config);
    Nm_NetworkRequest(0);
    Test_AdvanceTime(2000);  /* Through INITRESET */
    Test_AdvanceTime(3000);  /* Timeout -> LimpHome */

    /* Verify LimpHome */
    Nm_GetState(0, &state, &mode);
    assert(state == NM_STATE_LIMPHOME);

    /* Receive a message -> should recover to INITRESET */
    Test_RxNmMessage(NM_OP_RING_BIT, 0x0B);
    Nm_MainFunction();

    Nm_GetState(0, &state, &mode);
    /* Should transition to INITRESET and then NORMAL */
    assert(state == NM_STATE_INITRESET || state == NM_STATE_NORMAL_OPERATION);

    printf("PASSED\n");
}

/*=======[ T e s t   4 :  W a k e - u p   f r o m   B u s S l e e p ]======*/

static void Test4_WakeupFromBusSleep(void)
{
    Nm_StateType state;
    Nm_ModeType  mode;

    printf("Test 4: Wake-up from Bus-Sleep ... ");

    Test_Reset();
    Nm_Init(&g_config);
    Nm_NetworkRequest(0);
    Test_AdvanceTime(2000);
    Nm_NetworkRelease(0);
    Test_AdvanceTime(3000);

    /* Should be in Bus-Sleep */
    Nm_GetState(0, &state, &mode);
    assert(state == NM_STATE_BUS_SLEEP);

    /* Simulate receiving an NM message -> wake-up */
    Test_RxNmMessage(NM_OP_ALIVE_BIT, 0x0B);
    Nm_MainFunction();

    Nm_GetState(0, &state, &mode);
    assert(state != NM_STATE_BUS_SLEEP);  /* Should have woken up */
    assert(g_cbNetworkStartCount > 0);

    printf("PASSED\n");
}

/*=======[ T e s t   5 :  C o m m u n i c a t i o n   D i s a b l e ]======*/

static void Test5_CommunicationDisable(void)
{
    Nm_StateType state;
    Nm_ModeType  mode;
    uint32 txBefore;

    printf("Test 5: Communication disable ... ");

    Test_Reset();
    Nm_Init(&g_config);
    Nm_NetworkRequest(0);
    Test_AdvanceTime(2000);

    Nm_GetState(0, &state, &mode);
    assert(state == NM_STATE_NORMAL_OPERATION);

    txBefore = g_txCount;
    Nm_DisableCommunication(0);
    Test_AdvanceTime(2000);  /* One TTyp cycle */

    /* No new transmissions should have occurred */
    assert(g_txCount == txBefore);

    /* Re-enable */
    Nm_EnableCommunication(0);
    Test_AdvanceTime(2000);
    assert(g_txCount > txBefore);

    printf("PASSED\n");
}

/*=======[ T e s t   6 :  U s e r   D a t a ]===============================*/

static void Test6_UserData(void)
{
    uint8 txData[2] = {0xAA, 0xBB};
    uint8 rxData[6] = {0};
    uint8 nodeId = 0;

    printf("Test 6: User data set/get ... ");

    Test_Reset();
    Nm_Init(&g_config);

    /* Set user data */
    Nm_SetUserData(0, txData, 2);

    /* Simulate Rx of a message with user data */
    {
        uint8 pdu[8] = {0x01, 0x0B, 0xCC, 0xDD, 0x00, 0x00, 0x00, 0x00};
        Nm_RxIndication(0, pdu, 8);
    }

    Nm_GetUserData(0, rxData, &nodeId);
    assert(rxData[0] == 0xCC);
    assert(rxData[1] == 0xDD);
    assert(nodeId == 0x0B);

    printf("PASSED\n");
}

/*=======[ T e s t   7 :  I n d i r e c t   N M   s t a t e s ]============*/

static void Test7_IndirectNM(void)
{
    Nm_StateType state;
    Nm_ModeType  mode;

    printf("Test 7: Indirect NM ... ");

    /* Indirect NM test: uses application messages instead of NM PDUs.
     * Any received CAN message counts as "activity."
     * Config must use NM_MODE_INDIRECT and OPCODE wire format. */

    {
        Nm_ChannelConfigType indCh = g_channel;  /* copy */
        Nm_ConfigType indCfg = g_config;

        indCh.nmMode = NM_MODE_INDIRECT;
        indCfg.channels = &indCh;

        Test_Reset();
        Nm_Init(&indCfg);

        /* After Init, NM layer state = BUS_SLEEP (consistent with Direct NM) */
        Nm_GetState(0, &state, &mode);
        assert(state == NM_STATE_BUS_SLEEP || state == NM_STATE_UNINIT || state == NM_STATE_NORMAL_OPERATION);

        /* Receive an application message -> should go to NORMAL */
        {
            uint8 appPdu[8] = {0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            Nm_RxIndication(0, appPdu, 8);
        }
        Nm_MainFunction();

        /* Advance time without messages -> should go to WAITBUSSLEEP then BUSSLEEP */
        Test_AdvanceTime(5000);
        Nm_GetState(0, &state, &mode);
        assert(state == NM_STATE_BUS_SLEEP);

        printf("PASSED\n");
    }
}

/*=======[ M A I N ]=========================================================*/

int main(void)
{
    printf("\n=== NM Module Host Test Suite ===\n\n");

    Test1_BasicStateTransitions();
    Test2_LimpHomeEntry();
    Test3_LimpHomeRecovery();
    Test4_WakeupFromBusSleep();
    Test5_CommunicationDisable();
    Test6_UserData();
    Test7_IndirectNM();

    printf("\n=== All tests PASSED ===\n\n");
    return 0;
}

