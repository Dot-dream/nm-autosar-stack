/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       Nm_Timer.h
 *  @brief      Timer middleware for NM module
 *
 *  This module provides a lightweight software timer service for NM.
 *  It abstracts the system tick source so that NM code does not
 *  directly depend on any RTOS or bare-metal clock API.
 *
 *  Features:
 *    - Up to NM_TIMER_MAX_COUNT concurrent timers
 *    - One-shot mode (auto-stop after expiry)
 *    - Periodic mode (auto-restart after expiry)
 *    - Single linked list, O(n) expiry check
 *    - Tick source injected via Nm_Timer_Tick() / Nm_Timer_GetTick()
 *
 *  Design: NM calls Nm_Timer_Start() when it needs a timeout.
 *          Nm_Timer_Process() is called from Nm_MainFunction() each cycle.
 *          Timer callbacks fire when elapsed >= interval.
 *
 *  Platform adaptation: only Nm_Timer_GetTick() needs platform code.
 */
/*============================================================================*/

#ifndef NM_TIMER_H_
#define NM_TIMER_H_

#include "Nm_ConfigTypes.h"

/*=======[ C O N F I G U R A T I O N ]=======================================*/

/* Max number of concurrent timers.
 * Each NM channel uses ~5 timers (TTyp, TMax, TError, TWbs, TTx).
 * Default: 5 timers * 8 channels = 40.
 * Override in project config if needed. */
#ifndef NM_TIMER_MAX_COUNT
#define NM_TIMER_MAX_COUNT  40U
#endif

/*=======[ T Y P E S ]=======================================================*/

/* Timer handle. 0xFF = invalid / unallocated. */
typedef uint8 Nm_TimerHandle;
#define NM_TIMER_INVALID_HANDLE  0xFFU

/* Timer mode */
typedef uint8 Nm_TimerMode;
#define NM_TIMER_MODE_ONESHOT    0x00U   /* Fire once, then stop */
#define NM_TIMER_MODE_PERIODIC   0x01U   /* Fire every interval ms */

/* Timer callback: void (*)(Nm_TimerHandle handle, void* userData) */
typedef void (*Nm_TimerCallback)(Nm_TimerHandle handle, void* userData);

/*=======[ A P I ]===========================================================*/

/*****************************************************************************/
/*  Nm_Timer_Init                                                            */
/*  Brief:   Initialize the timer subsystem. Resets all timers to free.     */
/*           Must be called once before any other Nm_Timer_* function.       */
/*****************************************************************************/
void Nm_Timer_Init(void);

/*****************************************************************************/
/*  Nm_Timer_Alloc                                                           */
/*  Brief:   Allocate a timer. Returns handle on success,                    */
/*           NM_TIMER_INVALID_HANDLE if no free timer.                       */
/*  Param:   intervalMs  -- timeout in milliseconds                          */
/*  Param:   mode        -- NM_TIMER_MODE_ONESHOT or NM_TIMER_MODE_PERIODIC  */
/*  Param:   callback    -- function to call on expiry (NULL = no callback)  */
/*  Param:   userData    -- passed to callback                               */
/*  Return:  timer handle, or NM_TIMER_INVALID_HANDLE                        */
/*****************************************************************************/
Nm_TimerHandle Nm_Timer_Alloc(
    Nm_TimerMsType intervalMs,
    Nm_TimerMode   mode,
    Nm_TimerCallback callback,
    void*          userData
);

/*****************************************************************************/
/*  Nm_Timer_Start                                                           */
/*  Brief:   Start (or restart) a timer. Resets elapsed to 0.               */
/*  Param:   handle -- timer handle from Nm_Timer_Alloc()                    */
/*  Return:  TRUE on success, FALSE if handle invalid                        */
/*****************************************************************************/
boolean Nm_Timer_Start(Nm_TimerHandle handle);

/*****************************************************************************/
/*  Nm_Timer_Stop                                                            */
/*  Brief:   Stop a timer without firing callback. Elapsed is preserved.    */
/*  Param:   handle                                                          */
/*****************************************************************************/
void Nm_Timer_Stop(Nm_TimerHandle handle);

/*****************************************************************************/
/*  Nm_Timer_Free                                                            */
/*  Brief:   Deallocate a timer. Returns it to the free pool.               */
/*  Param:   handle                                                          */
/*****************************************************************************/
void Nm_Timer_Free(Nm_TimerHandle handle);

/*****************************************************************************/
/*  Nm_Timer_IsExpired                                                       */
/*  Brief:   Check if timer has expired (elapsed >= interval).              */
/*           Does NOT fire callback. Does NOT stop the timer.               */
/*  Param:   handle                                                          */
/*  Return:  TRUE if expired, FALSE otherwise                                */
/*****************************************************************************/
boolean Nm_Timer_IsExpired(Nm_TimerHandle handle);

/*****************************************************************************/
/*  Nm_Timer_GetElapsed                                                      */
/*  Brief:   Get elapsed time since last Start() in milliseconds.           */
/*  Param:   handle                                                          */
/*  Return:  Elapsed ms, or 0 if handle invalid                              */
/*****************************************************************************/
Nm_TimerMsType Nm_Timer_GetElapsed(Nm_TimerHandle handle);

/*****************************************************************************/
/*  Nm_Timer_GetRemaining                                                    */
/*  Brief:   Get remaining time before expiry in milliseconds.              */
/*  Param:   handle                                                          */
/*  Return:  Remaining ms, or 0 if expired / invalid handle                  */
/*****************************************************************************/
Nm_TimerMsType Nm_Timer_GetRemaining(Nm_TimerHandle handle);

/*****************************************************************************/
/*  Nm_Timer_SetInterval                                                     */
/*  Brief:   Change the timer interval without restarting.                  */
/*           If new interval <= elapsed, timer expires immediately.         */
/*  Param:   handle                                                          */
/*  Param:   newIntervalMs                                                   */
/*****************************************************************************/
void Nm_Timer_SetInterval(Nm_TimerHandle handle, Nm_TimerMsType newIntervalMs);

/*****************************************************************************/
/*  Nm_Timer_Process                                                         */
/*  Brief:   Process all active timers. Fires callbacks for expired timers. */
/*           Must be called periodically (typically from Nm_MainFunction).  */
/*           This is the ONLY place timer callbacks are invoked.            */
/*****************************************************************************/
void Nm_Timer_Process(void);

/*=======[ P L A T F O R M   A D A P T A T I O N   ( o n e   f u n c t i o n ) ]===*/

/*****************************************************************************/
/*  Nm_Timer_GetTick                                                         */
/*  Brief:   Get the current system tick in milliseconds.                   */
/*           THIS IS THE ONLY PLATFORM-SPECIFIC FUNCTION.                   */
/*           Implement in the integrating project.                          */
/*                                                                          */
/*  Examples:                                                               */
/*    FreeRTOS:  return xTaskGetTickCount() * portTICK_PERIOD_MS;           */
/*    bare-metal: return g_msCounter; (updated in SysTick ISR)              */
/*    Linux host: struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);  */
/*                return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;            */
/*****************************************************************************/
extern uint32 Nm_Timer_GetTick(void);

#endif /* NM_TIMER_H_ */
