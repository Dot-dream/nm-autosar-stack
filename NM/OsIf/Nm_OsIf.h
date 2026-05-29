/*============================================================================*/
/*  NM (Network Management) Standard Module
 *
 *  @file       Nm_OsIf.h
 *  @brief      OS abstraction layer for NM module (minimal)
 *
 *  Only TWO platform-specific items need adaptation:
 *    1. Nm_EnterCritical / Nm_ExitCritical (critical section / ISR lock)
 *    2. Nm_Timer_GetTick (in Nm_Timer/Nm_Timer.c, extern declared in Nm_Timer.h)
 *
 *  Timer abstraction has been moved to Nm_Timer/Nm_Timer.h.
 *  This file is intentionally minimal: no timers, no scheduler assumptions.
 */
/*============================================================================*/

#ifndef NM_OSIF_H_
#define NM_OSIF_H_

/*=======[ C R I T I C A L   S E C T I O N ]=================================*/

/* Nm_EnterCritical / Nm_ExitCritical must be provided by the project.
 * They may be macros OR function declarations. */
#if !defined(Nm_EnterCritical) && !defined(NM_HOST_TEST)
#error "Nm_EnterCritical() must be defined by the integrating project"
#endif
#if !defined(Nm_ExitCritical) && !defined(NM_HOST_TEST)
#error "Nm_ExitCritical() must be defined by the integrating project"
#endif

#ifdef NM_HOST_TEST
/* Host test: provide extern declarations (test defines the bodies) */
extern void Nm_EnterCritical(void);
extern void Nm_ExitCritical(void);
#endif

/*=======[ S T A T I C   A S S E R T ]=======================================*/

#ifndef NM_STATIC_ASSERT
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define NM_STATIC_ASSERT(cond, msg)  _Static_assert((cond), #msg)
#else
#define NM_STATIC_ASSERT(cond, msg)  typedef char NM_Assert_##msg[(cond) ? 1 : -1]
#endif
#endif

/*=======[ C O M P I L E R   A T T R I B U T E S ]=============================*/

/* Weak symbol attribute — portable across GCC, ARMCC, IAR */
#if defined(__GNUC__) || defined(__clang__)
#define NM_WEAK  __attribute__((weak))
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define NM_WEAK  __weak
#elif defined(__ICCARM__)
#define NM_WEAK  __weak
#else
#define NM_WEAK  /* weak not supported — provide a strong definition */
#endif

/*=======[ C O M P I L E   C H E C K S ]=====================================*/

NM_STATIC_ASSERT(NM_PDU_MAX_LENGTH == 8,  NmPduLengthMustBe8);
NM_STATIC_ASSERT(NM_MAX_CHANNELS  <= 8,  NmMaxChannelsLimit);

#endif /* NM_OSIF_H_ */
