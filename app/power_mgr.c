/*
 * power_mgr.c
 *
 *  Created on: May 23, 2026
 *      Author: abhij
 */


#include "power_mgr.h"
#include "board.h"

/*
 * @brief: PowerMgrInit initializes the peripherals required to put the micro in sleep state */
int8_t PowerMgrInit(void)
{
	/* TODO: Implement this
	 * Do the necessary init for putting micro in deep sleep. */
	return APP_ERR_INV_ARG;
}


/*
 * @brief: Here we do the necessary steps in order to enter deep sleep such as:
 * 			-> Cutting OFF power for other components.
 * 			-> Setting wakeup time
 * 			-> Enabling wakeup ON timer
 * 			-> Calling enter deep sleep API. */
int8_t PowerMgrEnterDeepSleep(void)
{
	void Enter_Shutdown_Timer(uint32_t seconds)
	{
	  // 1. Deactivate the timer momentarily to program new values cleanly
	  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

	  // 2. Clear all previous Wakeup Flags across the Power Peripheral
	  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	  /*
	    Calculate countdown value:
	    Clock Source = RTC_WAKEUPCLOCK_RTCCLK_DIV16 (LSE crystal = 32.768 kHz)
	    Resolution Frequency = 32768 / 16 = 2048 Hz
	    Ticks Needed = target seconds * 2048
	  */
	  uint32_t wakeup_counter = seconds * 2048;

	  // 3. Configure the wakeup timer structure utilizing internal IT lines
	  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, wakeup_counter, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  // 4. Critical step for Shutdown: Clear flags once more immediately before entry
	  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	  // 5. Trigger Shutdown Entry
	  HAL_PWREx_EnterSHUTDOWNMode();
	}
	return APP_ERR_INV_ARG;
}

