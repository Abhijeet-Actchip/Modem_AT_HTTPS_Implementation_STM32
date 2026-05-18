/*
 * appmgr.c
 *
 *  Created on: May 18, 2026
 *      Author: abhij
 */

#include "bsp/board.h"
#include "appmgr.h"
#include "FreeRTOS.h"
#include "task.h"
#include "utils/trice/trice.h"
#include "app_hal_delay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config/appconfig.h"
#include "https_common.h"

#define APPMGR_PER_UPLOAD_INTERVAL_MS     5000

static TaskHandle_t appmgrHandle = NULL;

static void PublishDataToQ(float a, float b)
{
	char buffer[128];
	
	/* Formulate HTTP POST body */
	snprintf(buffer, sizeof(buffer), "api_key=%s&field1=%.2f&field2=%.2f", APP_HTTPS_WRITE_API_KEY, a, b);
	
	trice(iD(1121), "dbg: Publishing data to HTTPS Q...\n");
	HTTPSSendData(buffer, 100);
}

static void WriteUploadDataToQ(void)
{
	float a, b;

	/* Generate random data in range 0.0 to 100.0 */
	a = (float)rand() / (float)(RAND_MAX / 100.0f);
	b = (float)rand() / (float)(RAND_MAX / 100.0f);

	PublishDataToQ(a, b);
}

static void vTaskAppMgr(void *args)
{
	TickType_t startTicks = xTaskGetTickCount(), diffTicks = 0;

	while(1)
	{
		diffTicks = xTaskGetTickCount() - startTicks;
		if(diffTicks >= pdMS_TO_TICKS(APPMGR_PER_UPLOAD_INTERVAL_MS))
		{
			WriteUploadDataToQ();
			startTicks = xTaskGetTickCount();
		}
		APPHAL_Delay(500);
	}
}


int8_t AppMgrInit(void)
{
	int8_t retVal = APP_ERR_NONE;

	BaseType_t status = xTaskCreate(vTaskAppMgr, "APPMGR", STACK_APPMGR, NULL, PRIORITY_APPMGR, &appmgrHandle);
	if(status != pdTRUE)
	{
		trice(iD(6722), "dbg: Failed to create AppMgr task\n");
		retVal = APP_ERR_NO_MEM;
	}

	return retVal;
}

void AppMgrPrintDiags(void)
{
	if(appmgrHandle != NULL)
	{
		uint32_t space = uxTaskGetStackHighWaterMark(appmgrHandle) * sizeof(StackType_t);
		trice32(iD(5813), "dbg: diag: APP-MGR stack rem = %d \n", space);
	}
}
