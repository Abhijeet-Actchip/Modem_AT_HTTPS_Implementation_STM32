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

#define APPMGR_PER_UPLOAD_INTERVAL_MS     20000

static TaskHandle_t appmgrHandle = NULL;

static void PublishDataToQ(float a, float b)
{
	int8_t ret;
	char buffer[256];

	int32_t a_whole = (int32_t)a;
	int32_t a_frac = (int32_t)((a - (float)a_whole) * 100.0f + 0.5f);
	if (a_frac >= 100)
	{
		a_whole += 1;
		a_frac = 0;
	}

	int32_t b_whole = (int32_t)b;
	int32_t b_frac = (int32_t)((b - (float)b_whole) * 100.0f + 0.5f);
	if (b_frac >= 100)
	{
		b_whole += 1;
		b_frac = 0;
	}

	/* Formulate HTTP POST body with incrementing simulated timestamp, avoiding float formatting */
#if (APP_HTTPS_CONTENT_TYPE == HTTP_CONTENT_APPLICATION_JSON)
	snprintf(buffer, sizeof(buffer),
	         "{\"api_key\":\"%s\",\"field1\":%ld.%02ld,\"field2\":%ld.%02ld}",
	         APP_HTTPS_WRITE_API_KEY, a_whole, a_frac, b_whole, b_frac);
#else
	snprintf(buffer, sizeof(buffer), "api_key=%s&field1=%ld.%02ld&field2=%ld.%02ld",
	         APP_HTTPS_WRITE_API_KEY, a_whole, a_frac, b_whole, b_frac);
#endif

	trice(iD(1121), "dbg: Publishing data to HTTPS Q...\n");
	TRICE_S(id(5403), "%s\n", buffer);
	ret = HTTPSSendData(buffer, 100);
	if(ret != APP_ERR_NONE)
	{
		trice32(iD(5958), "dbg: Failed to send data to Q\n, %d",ret);
	}
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
			trice(iD(6916), "dbg: Upload Event\n");
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
