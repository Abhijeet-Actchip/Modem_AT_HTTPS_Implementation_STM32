/*! \file triceConfig.h
\author Thomas.Hoehenleitner [at] seerose.net
*******************************************************************************/

#ifndef TRICE_CONFIG_H_
#define TRICE_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//#define TRICE_BUFFER TRICE_DOUBLE_BUFFER
//#define TRICE_DEFERRED_BUFFER_SIZE 4096
//#define TRICE_DIAGNOSTICS 0

//! TriceStamp16 returns a 16-bit value to stamp `Id` TRICE macros. Usually it is a timestamp, but could also be a destination address or a counter for example.
//! The user has to provide this function. Defining a macro here, instead if providing `int16_t TriceStamp16( void );` has significant speed impact.
//#define TriceStamp16() (DWT->CYCCNT) // 64 MHz wraps after >1ms
//#define TriceStamp16 (DWT->CYCCNT)

//! TriceStamp32 returns a 32-bit value to stamp `ID` TRICE macros. Usually it is a timestamp, but could also be a destination address or a counter for example.
//! The user has to provide this function. Defining a macro here, instead if providing `int32_t TriceStamp32( void );` has significant speed impact.
//#define TriceStamp32() ((DWT->CYCCNT)>>6) // 64 MHz -> 1 µs
//#define TriceStamp32 (DWT->CYCCNT)

#define TRICE_DEFERRED_OUTPUT		1
#define TRICE_DEFERRED_UARTA		1
#define TRICE_UARTA					LPUART1
#define TRICE_BUFFER				TRICE_DOUBLE_BUFFER
#define TRICE_DEFERRED_BUFFER_SIZE	1024
#define TRICE_PROTECT 				1
#define TRICE_DIAGNOSTICS			1
#define TRICE_CYCLE_COUNTER			1

//! USE_SEGGER_RTT_LOCK_UNLOCK_MACROS == 1 includes SEGGER_RTT header files even SEGGER_RTT is not used. THis allows SEGGER code for critical sections.
#define USE_SEGGER_RTT_LOCK_UNLOCK_MACROS 1

//! TRICE_ENTER_CRITICAL_SECTION saves interrupt state and disables Interrupts.
//! If trices are used only outside critical sections or interrupts,
//! you can leave this macro empty for more speed. Use only '{' in that case.
//! #define TRICE_ENTER_CRITICAL_SECTION { SEGGER_RTT_LOCK() { - does the job for many compilers.
//! #define TRICE_ENTER_CRITICAL_SECTION { 
//! #define TRICE_ENTER_CRITICAL_SECTION { uint32_t old_mask = cm_mask_interrupts(1); { // copied from test/OpenCM3_STM32F411_Nucleo/triceConfig.h
//! #define TRICE_ENTER_CRITICAL_SECTION { uint32_t primaskstate = __get_PRIMASK(); __disable_irq(); {
#define TRICE_ENTER_CRITICAL_SECTION { SEGGER_RTT_LOCK() {

//! TRICE_LEAVE_CRITICAL_SECTION restores interrupt state.
//! If trices are used only outside critical sections or interrupts,
//! you can leave this macro empty for more speed. Use only '}' in that case.
//! #define TRICE_LEAVE_CRITICAL_SECTION } SEGGER_RTT_UNLOCK() } - does the job for many compilers.
//! #define TRICE_LEAVE_CRITICAL_SECTION } 
//! #define TRICE_LEAVE_CRITICAL_SECTION } cm_mask_interrupts(old_mask); } // copied from test/OpenCM3_STM32F411_Nucleo/triceConfig.h
//! #define TRICE_LEAVE_CRITICAL_SECTION } __set_PRIMASK(primaskstate); }
#define TRICE_LEAVE_CRITICAL_SECTION } SEGGER_RTT_UNLOCK() }

#define TRICE_INLINE static inline //! used for trice code

// hardware interface:
#ifndef TEST
#include "bsp/board.h"
#endif

//
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* TRICE_CONFIG_H_ */
