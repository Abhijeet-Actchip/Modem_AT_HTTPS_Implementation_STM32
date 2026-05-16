/*
 * app_main.c
 *
 *  Created on: May 15, 2026
 *      Author: abhij
 */



#include "bsp/board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_hal/app_hal_wdt.h"
#include "utils/assert/app_assert.h"
#include "diag_svc.h"

int main(void)
{
	int8_t retVal;

	retVal = Board_InitPeripherals();
	APP_ASSERT(retVal == APP_ERR_NONE);

	retVal = DiagSvcInit();
	APP_ASSERT(retVal == APP_ERR_NONE);

	APPHAL_WDT_Clear();

	/* Start scheduler */
	vTaskStartScheduler();
	/* We should never get here as control is now taken by the scheduler */
	APP_ASSERT(0);
}
