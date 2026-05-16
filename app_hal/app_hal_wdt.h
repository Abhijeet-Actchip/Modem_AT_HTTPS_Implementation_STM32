/*
 * app_hal_wdt.h
 *
 *  Created on: Sep 20, 2023
 *      Author: Mahesh Murty
 */

#ifndef __APP_HAL_WDT_H__
#define __APP_HAL_WDT_H__				1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
/*!
 * \brief Clears the watchdog timer to prevent system reset.
 *
 * The watchdog timer (IWDG) is a safety feature designed to reset the system 
 * if it becomes unresponsive or stuck. This function provides a mechanism to 
 * refresh the watchdog timer, ensuring that the system does not reset unintentionally.
 *
 * \note Ensure that the watchdog timer (APP_WDT_EN) is enabled in the system configuration 
 * for this function to have an effect. If the watchdog is not enabled, calling this function 
 * will have no practical consequence.
 *
 * \return An error code indicating the outcome of the watchdog refresh operation. 
 *         A return value of APP_ERR_NONE suggests successful refresh, while other 
 *         error codes may indicate specific issues if applicable.
 */
int8_t APPHAL_WDT_Clear(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_HAL_WDT_H__ */
