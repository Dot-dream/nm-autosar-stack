/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       Nm_Timer.c
 *  @brief      Timer middleware implementation (linked-list based)
 *
 *  Dependencies: Nm_Timer_GetTick() -- platform tick source
 *                Nm_EnterCritical / Nm_ExitCritical -- from Nm_OsIf.h
 */
/*============================================================================*/

#include "Nm_Timer.h"
#include "Nm_OsIf.h"

/*=======[ I N T E R N A L   T I M E R   S T R U C T ]======================*/

typedef struct {
    uint8            allocated;
    uint8            running;

    Nm_TimerMsType   intervalMs;
    Nm_TimerMode     mode;
    Nm_TimerCallback callback;
    void*            userData;

    uint32           startTick;
    uint32           elapsedCache;
} Nm_TimerNode;

/*=======[ G L O B A L S ]===================================================*/

static Nm_TimerNode g_timers[NM_TIMER_MAX_COUNT];
static uint8        g_initialized;

#define TIMER_IDX(handle)  ((uint8)(handle))

/*=======[ I N T E R N A L ]=================================================*/

static Nm_TimerNode* Timer_Get(Nm_TimerHandle handle)
{
    if (handle >= NM_TIMER_MAX_COUNT) { return NULL; }
    if (0U == g_timers[TIMER_IDX(handle)].allocated) { return NULL; }
    return &g_timers[TIMER_IDX(handle)];
}

static uint32 Timer_Elapsed(Nm_TimerNode* node)
{
    uint32 now;
    if (NULL == node) { return 0U; }
    if (0U == node->running) { return node->elapsedCache; }
    now = Nm_Timer_GetTick();
    if (now >= node->startTick) {
        return node->elapsedCache + (now - node->startTick);
    }
    /* wrap-around */
    return node->elapsedCache + (0xFFFFFFFFUL - node->startTick + now + 1U);
}

/*=======[ P U B L I C   A P I ]=============================================*/

void Nm_Timer_Init(void)
{
    uint8 i;
    for (i = 0; i < NM_TIMER_MAX_COUNT; i++) {
        g_timers[i].allocated    = 0U;
        g_timers[i].running      = 0U;
        g_timers[i].intervalMs   = 0U;
        g_timers[i].callback     = NULL;
        g_timers[i].userData     = NULL;
        g_timers[i].startTick    = 0U;
        g_timers[i].elapsedCache = 0U;
    }
    g_initialized = 1U;
}

Nm_TimerHandle Nm_Timer_Alloc(
    Nm_TimerMsType   intervalMs,
    Nm_TimerMode     mode,
    Nm_TimerCallback callback,
    void*            userData)
{
    uint8 i;
    if (0U == g_initialized) { return NM_TIMER_INVALID_HANDLE; }
    for (i = 0; i < NM_TIMER_MAX_COUNT; i++) {
        if (0U == g_timers[i].allocated) {
            g_timers[i].allocated    = 1U;
            g_timers[i].running      = 0U;
            g_timers[i].intervalMs   = intervalMs;
            g_timers[i].mode         = mode;
            g_timers[i].callback     = callback;
            g_timers[i].userData     = userData;
            g_timers[i].startTick    = 0U;
            g_timers[i].elapsedCache = 0U;
            return (Nm_TimerHandle)i;
        }
    }
    return NM_TIMER_INVALID_HANDLE;
}

boolean Nm_Timer_Start(Nm_TimerHandle handle)
{
    Nm_TimerNode* node = Timer_Get(handle);
    if (NULL == node) { return FALSE; }
    node->running      = 1U;
    node->startTick    = Nm_Timer_GetTick();
    node->elapsedCache = 0U;
    return TRUE;
}

void Nm_Timer_Stop(Nm_TimerHandle handle)
{
    Nm_TimerNode* node = Timer_Get(handle);
    if (NULL == node) { return; }
    if (node->running) {
        node->elapsedCache = Timer_Elapsed(node);
    }
    node->running = 0U;
}

void Nm_Timer_Free(Nm_TimerHandle handle)
{
    Nm_TimerNode* node = Timer_Get(handle);
    if (NULL == node) { return; }
    node->allocated    = 0U;
    node->running      = 0U;
    node->callback     = NULL;
    node->userData     = NULL;
}

boolean Nm_Timer_IsExpired(Nm_TimerHandle handle)
{
    Nm_TimerNode* node = Timer_Get(handle);
    if (NULL == node) { return FALSE; }
    return (Timer_Elapsed(node) >= node->intervalMs) ? TRUE : FALSE;
}

Nm_TimerMsType Nm_Timer_GetElapsed(Nm_TimerHandle handle)
{
    Nm_TimerNode* node = Timer_Get(handle);
    if (NULL == node) { return 0U; }
    return Timer_Elapsed(node);
}

Nm_TimerMsType Nm_Timer_GetRemaining(Nm_TimerHandle handle)
{
    Nm_TimerNode* node = Timer_Get(handle);
    uint32 elapsed;
    if (NULL == node) { return 0U; }
    elapsed = Timer_Elapsed(node);
    if (elapsed >= node->intervalMs) { return 0U; }
    return node->intervalMs - elapsed;
}

void Nm_Timer_SetInterval(Nm_TimerHandle handle, Nm_TimerMsType newMs)
{
    Nm_TimerNode* node = Timer_Get(handle);
    if (NULL == node) { return; }
    node->intervalMs = newMs;
}

void Nm_Timer_Process(void)
{
    uint8 i;
    if (0U == g_initialized) { return; }
    for (i = 0; i < NM_TIMER_MAX_COUNT; i++) {
        Nm_TimerNode* node = &g_timers[i];
        if (0U == node->allocated || 0U == node->running) { continue; }
        if (Timer_Elapsed(node) < node->intervalMs) { continue; }

        /* Expired */
        if (NM_TIMER_MODE_ONESHOT == node->mode) {
            node->running      = 0U;
            node->elapsedCache = node->intervalMs;
        } else {
            node->startTick   += node->intervalMs;
            node->elapsedCache = 0U;
        }
        if (NULL != node->callback) {
            node->callback((Nm_TimerHandle)i, node->userData);
        }
    }
}
