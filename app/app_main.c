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
#include "https_common.h"

void SetupHTTPConfig(httpsAgentConfig_t *var)
{
	var->NwReadyWait = NULL;
	var->nwReadyBit = 0;

	strncpy(var->baseUrl, APP_HTTPS_URL, sizeof(var->baseUrl) - 1);
	var->contentType = HTTP_CONTENT_APPLICATION_JSON;
	strncpy(var->readApiKey, APP_HTTPS_READ_API_KEY, sizeof(var->readApiKey) - 1);
	strncpy(var->writeApiKey, APP_HTTPS_WRITE_API_KEY, sizeof(var->writeApiKey) - 1);
	var->useTLS = APP_USE_TLS;
	var->responseTimeMs = 10000;
	var->timeoutMs = 1000;
}

int main(void)
{
	int8_t retVal;
	httpsAgentConfig_t httpCfg = {0};

	retVal = Board_InitPeripherals();
	APP_ASSERT(retVal == APP_ERR_NONE);

	SetupHTTPConfig(&httpCfg);

	retVal = DiagSvcInit();
	APP_ASSERT(retVal == APP_ERR_NONE);

	retVal = HttpsCommonMgrInit(&httpCfg);
	APP_ASSERT(retVal == APP_ERR_NONE);

	APPHAL_WDT_Clear();

	/* Start scheduler */
	vTaskStartScheduler();
	/* We should never get here as control is now taken by the scheduler */
	APP_ASSERT(0);
}
