/*
 * app_hal_wdt.c
 *
 *  Created on: Sep 20, 2023
 *      Author: Mahesh Murty
 */

#include "bsp/board.h"

#if (APP_WDT_EN == 1)
	extern IWDG_HandleTypeDef hiwdg;
#endif

/*!
 * \brief Clears the watchdog timer to prevent system reset.
 *
 * When the watchdog timer (IWDG) is enabled and starts counting, it expects to be 
 * refreshed periodically to prevent a system reset. This function ensures that the 
 * watchdog timer is refreshed, thus preventing an unintended system reset.
 *
 * \note This function is relevant only when the watchdog timer is enabled (APP_WDT_EN == 1).
 *
 * \return An error code that indicates the success of the watchdog refresh operation.
 *         Typically, if the watchdog is enabled and successfully refreshed, it returns APP_ERR_NONE.
 */
int8_t APPHAL_WDT_Clear(void)
{
	#if (APP_WDT_EN == 1)
		HAL_IWDG_Refresh(&hiwdg);
	#endif

	return APP_ERR_NONE;
}
