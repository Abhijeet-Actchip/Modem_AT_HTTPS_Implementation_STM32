/*
 * app_hal_delay.c
 *
 *  Created on: Oct 12, 2023
 *      Author: Mahesh Murty
 */

#include "app_hal_delay.h"
#include "bsp/board.h"
#include "cmsis_os.h"

/*!
 * \brief Retrieves the current system tick count.
 *
 * This function returns the system tick count based on the current operating
 * system state. If the RTOS is running, it retrieves the tick count from the
 * OS kernel. Otherwise, it uses the HAL tick count.
 *
 * \note When the operating system is running, a conversion from ticks to milliseconds
 *       may be required, especially if the tick rate is not 1000Hz.
 *
 * \return Current tick count in milliseconds.
 */
uint32_t APPHAL_GetTick(void)
{
	uint32_t ticks;
	if(osKernelGetState() == osKernelRunning)
	{
		ticks = osKernelGetTickCount();
		/* TODO: Apply math to convert ticks to MS.
		 * This is not required if tickrate is 1000Hz. */
	}
	else
	{
		ticks = HAL_GetTick();
	}

	return ticks;
}
/*!
 * \brief Delays the execution for a specified duration in milliseconds.
 *
 * This function introduces a delay in the execution based on the current state
 * of the operating system. If the RTOS is running, it uses the OS delay function.
 * Otherwise, it relies on the HAL delay function.
 *
 * \param delayMs The duration of the delay in milliseconds.
 */
void APPHAL_Delay(uint32_t delayMs)
{
	if(osKernelGetState() == osKernelRunning)
	{
		osDelay(pdMS_TO_TICKS(delayMs));
	}
	else
	{
		HAL_Delay(delayMs);
	}
}
