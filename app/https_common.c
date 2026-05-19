#include "https_common.h"
#include "https_at_mgr.h"
#include "FreeRTOS.h"
#include "task.h"
#include "config/appconfig.h"
#include <string.h>
#include "utils/trice/trice.h"

static httpsAgentConfig_t *httpsConfig = NULL;

static int8_t ValidateConfig(httpsAgentConfig_t *config)
{
    if(config == NULL)
        return APP_ERR_INV_ARG;

    if(!strlen(config->baseUrl))
    {
        return APP_ERR_INV_ARG;
    }

    return APP_ERR_NONE;
}

int8_t HttpsCommonMgrInit(httpsAgentConfig_t *config)
{
    int8_t retVal = APP_ERR_INV_ARG;

    if(config == NULL)
    {
        return retVal;
    }

    /* Validate config parameters */
    retVal = ValidateConfig(config);
    if(retVal != APP_ERR_NONE)
    {
        return retVal;
    }

    /* Declare tx and rx queues */
    config->httpsRxQHandle = xQueueCreate(HTTP_RX_QUEUE_SIZE, sizeof(httpsDataPkt_t *));
    configASSERT(config->httpsRxQHandle != NULL);

    config->httpsTxQHandle = xQueueCreate(HTTP_TX_QUEUE_SIZE, sizeof(httpsDataPkt_t *));
    configASSERT(config->httpsTxQHandle != NULL);

    /* Config is valid, store it */
    httpsConfig = config;

    retVal = HttpsATMgrInit(config);

    return retVal;
}

int8_t HttpsSetWriteApiKey(const char *key)
{
    if(httpsConfig == NULL || key == NULL) return APP_ERR_INV_STATE;
    strncpy(httpsConfig->writeApiKey, key, 31);
    httpsConfig->writeApiKey[31] = '\0';
    return APP_ERR_NONE;
}

int8_t HttpsSetReadApiKey(const char *key)
{
    if(httpsConfig == NULL || key == NULL) return APP_ERR_INV_STATE;
    strncpy(httpsConfig->readApiKey, key, 31);
    httpsConfig->readApiKey[31] = '\0';
    return APP_ERR_NONE;
}

int8_t HTTPSWriteToTxQ(httpsDataPkt_t *dataPktPtr, uint32_t timeoutMs)
{
    if(httpsConfig->httpsTxQHandle != NULL)
    {
        return (xQueueSend(httpsConfig->httpsTxQHandle, &dataPktPtr, pdMS_TO_TICKS(timeoutMs)) == pdPASS) ? APP_ERR_NONE : APP_ERR_TIMEOUT;
    }
    return APP_ERR_INV_STATE;
}

int8_t HTTPSReadFromRxQ(httpsDataPkt_t **dataPktPtr, uint32_t timeoutMs)
{
    if(httpsConfig->httpsRxQHandle == NULL)
    {
        return APP_ERR_INV_STATE;
    }
    return (xQueueReceive(httpsConfig->httpsRxQHandle, dataPktPtr, pdMS_TO_TICKS(timeoutMs)) == pdPASS) ? APP_ERR_NONE : APP_ERR_TIMEOUT;
}

int8_t HTTPSWriteToRxQ(httpsDataPkt_t *dataPktPtr, uint32_t timeoutMs)
{
    if(httpsConfig->httpsRxQHandle != NULL)
    {
        return (xQueueSend(httpsConfig->httpsRxQHandle, &dataPktPtr, pdMS_TO_TICKS(timeoutMs)) == pdPASS) ? APP_ERR_NONE : APP_ERR_TIMEOUT;
    }
    return APP_ERR_INV_STATE;
}

int8_t HTTPSReadFromTxQ(httpsDataPkt_t **dataPktPtr, uint32_t timeoutMs)
{
    if(httpsConfig->httpsTxQHandle == NULL)
    {
        return APP_ERR_INV_STATE;
    }
    return (xQueueReceive(httpsConfig->httpsTxQHandle, dataPktPtr, pdMS_TO_TICKS(timeoutMs)) == pdPASS) ? APP_ERR_NONE : APP_ERR_TIMEOUT;
}

int8_t HTTPSIsRxQEmpty(void)
{
    if(httpsConfig->httpsRxQHandle != NULL)
    {
        return (uxQueueMessagesWaiting(httpsConfig->httpsRxQHandle) == 0);
    }
    return APP_ERR_INV_STATE;
}

int8_t HTTPSIsTxQEmpty(void)
{
    if(httpsConfig->httpsTxQHandle != NULL)
    {
        return (uxQueueMessagesWaiting(httpsConfig->httpsTxQHandle) == 0);
    }
    return APP_ERR_INV_STATE;
}

int8_t HTTPSGetTxQFreeSlots(void)
{
    if(httpsConfig->httpsTxQHandle != NULL)
    {
        return uxQueueSpacesAvailable(httpsConfig->httpsTxQHandle);
    }
    return APP_ERR_INV_STATE;
}

int8_t HTTPSGetRxQFreeSlots(void)
{
    if(httpsConfig->httpsRxQHandle != NULL)
    {
        return uxQueueSpacesAvailable(httpsConfig->httpsRxQHandle);
    }
    return APP_ERR_INV_STATE;
}

int8_t HTTPSSendData(char *msg, int32_t timeoutMs)
{
    int8_t retVal = APP_ERR_INV_ARG;
    if((msg == NULL) || (strlen(msg) == 0))
    {
        return retVal;
    }

    int16_t msgLen = strlen(msg);
    httpsDataPkt_t *newMsg = NULL;

    retVal = APP_ERR_NO_MEM;

    if(HTTPSGetTxQFreeSlots() > 0)
    {
    	newMsg = pvPortMalloc(sizeof(httpsDataPkt_t));
		if(newMsg == NULL)
		{
			return retVal;
		}

		newMsg->dataPtr = pvPortMalloc(sizeof(uint8_t) * (msgLen + 1));
		if(newMsg->dataPtr == NULL)
		{
			vPortFree(newMsg);
			return retVal;
		}

		memcpy(newMsg->dataPtr, msg, msgLen);
		newMsg->dataPtr[msgLen] = '\0';
		newMsg->dataLen = msgLen;
		newMsg->pktType = HTTPS_TX_PKT;

		if(HTTPSWriteToTxQ(newMsg, pdMS_TO_TICKS(timeoutMs)) == APP_ERR_NONE)
		{
			retVal = APP_ERR_NONE;
		}
		else
		{
			vPortFree(newMsg->dataPtr);
			vPortFree(newMsg);
			retVal = APP_ERR_NO_MEM;
		}
    }
    else
    {
    	trice(iD(5060), "dbg: TxQ FULL, skipping packet...\n");
    }

    return retVal;
}

int8_t HTTPSGetConnectStatus(void)
{
    /* For HTTPS, connection status could mean network registration and PDP activation status */
    /* Implementation depends on HttpsATMgr */
    return 1; // Placeholder
}
