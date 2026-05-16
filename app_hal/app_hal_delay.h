/*
 * app_hal_delay.h
 *
 *  Created on: Oct 12, 2023
 *      Author: Mahesh Murty
 */

#ifndef __APP_HAL_DELAY_H__
#define __APP_HAL_DELAY_H__				1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
/*!
 * \brief Retrieves the current system tick count.
 *
 * This function is responsible for fetching the current system tick count.
 * Depending on the system state, it can fetch ticks from the RTOS kernel or
 * the HAL layer.
 *
 * \note The return value might need further processing, especially if the tick rate
 *       is not set to 1000Hz, to convert the ticks to milliseconds.
 *
 * \return The current system tick count in milliseconds.
 */
uint32_t APPHAL_GetTick(void);
/*!
 * \brief Delays the execution for a specified duration in milliseconds.
 *
 * This function introduces a delay in the execution flow for a specified
 * amount of time, measured in milliseconds. It utilizes either the OS delay
 * mechanism (if the OS is running) or the HAL delay mechanism.
 *
 * \param delayMs The duration of the delay in milliseconds.
 */
void APPHAL_Delay(uint32_t delayMs);

#ifdef __cplusplus
}
#endif

#endif /* __APP_HAL_DELAY_H__ */
