#ifndef STD_TYPES_H_
#define STD_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t   sint8;
typedef int16_t  sint16;
typedef int32_t  sint32;
typedef int64_t  sint64;

typedef bool     boolean;

#ifndef TRUE
#define TRUE  1U
#endif
#ifndef FALSE
#define FALSE 0U
#endif

#ifndef NULL
#define NULL  ((void*)0)
#endif

#ifndef STD_ON
#define STD_ON  1U
#endif
#ifndef STD_OFF
#define STD_OFF 0U
#endif

/* Critical section macros for NM module.
 * Uses Cortex-M CPS instructions directly — no CMSIS header dependency.
 * Project may override by defining before inclusion of Nm_OsIf.h. */
#ifndef Nm_EnterCritical
#define Nm_EnterCritical()  __asm volatile ("cpsid i" ::: "memory")
#endif
#ifndef Nm_ExitCritical
#define Nm_ExitCritical()   __asm volatile ("cpsie i" ::: "memory")
#endif

#endif /* STD_TYPES_H_ */
