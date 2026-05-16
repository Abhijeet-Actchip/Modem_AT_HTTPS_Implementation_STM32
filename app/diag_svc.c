/*
 * diag_svc.c
 *
 *  Created on: Mar 4, 2025
 *      Author: Mahesh Murty
 */

#include "diag_svc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "bsp/board.h"
#include "app_hal/app_hal_wdt.h"
#include "app_hal/app_hal_delay.h"
#include "utils/trice/trice.h"
#include "utils/assert/app_assert.h"
#include "https_at_mgr.h"

/* NOTE: mmurty: Using externs is a bad practice but these variables are not
 * accessible from the logger library. So this is the last resort. */
#if TRICE_DIAGNOSTICS == 1
	extern unsigned TriceSingleMaxWordCount;
	extern unsigned TriceHalfBufferDepthMax;
#endif

static TimerHandle_t logTimer = NULL;
static TaskHandle_t taskHandle = NULL;

static void LogTimerEvent(TimerHandle_t xTimer)
{
	TriceTransfer();
}

void DiagSvcTask(void *args)
{
	uint32_t space;

	/* Start the log timer to push logs on UART periodically. */
	BaseType_t status = xTimerStart(logTimer, pdMS_TO_TICKS(10));
	APP_ASSERT(status != pdFAIL);

	while(1)
	{
		APPHAL_WDT_Clear();

		/* TODO: Print diagnostic data for all tasks */
		HttpAtMgrPrintDiags();

		/* Get remaining stack space of this task and log it */
		space = uxTaskGetStackHighWaterMark(taskHandle) * sizeof(StackType_t);
		trice32(iD(7888), "dbg: diag: Diag Mgr stack rem = %d \n", space);
		#if TRICE_DIAGNOSTICS == 1
			trice32(iD(1843), "dbg: diag: TriceSingleMaxWordCount %d\n", TriceSingleMaxWordCount);
			trice32(iD(7071), "dbg: diag: TriceHalfBufferDepthMax %d\n", TriceHalfBufferDepthMax);
		#endif

		/* TODO: Print RTOS: Max heap diags */

		APPHAL_Delay(5000);
	}
}

int8_t DiagSvcInit(void)
{
	BaseType_t status;

	/* Create a auto-reload timer with a period of 200ms. */
	logTimer = xTimerCreate("LOGTIM", pdMS_TO_TICKS(200),
							pdTRUE, NULL, LogTimerEvent);
	if (logTimer == NULL)
	{
		return APP_ERR_NO_MEM;
	}

	/* Create the diagnostics service task. */
	status = xTaskCreate(DiagSvcTask, "DIAGSVC",
			DIAG_SVC_STACK_SIZE, NULL,
			DIAG_SVC_PRIORITY, &taskHandle);
	if(status != pdPASS)
	{
		return APP_ERR_NO_MEM;
	}

	return APP_ERR_NONE;
}
