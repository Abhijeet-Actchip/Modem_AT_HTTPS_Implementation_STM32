#include "mqtt_common.h"
#include "mqtt_at_mgr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config/appconfig.h"

#include <string.h>

static const char *TAG = "MQTT-COMMON";

static mqttAgentConfig_t *mqttConfig = NULL;

static int8_t ValidateConfig(mqttAgentConfig_t *config)
{
	if(config == NULL)
		return APP_ERR_INV_ARG;

    if((config->ctxType != MQTT_CTX_AGENT) && (config->ctxType != MQTT_CTX_AT_MGR))
    {
        return APP_ERR_INV_ARG;
    }

	if(!strlen(config->brokerUrl) || !strlen(config->clientId))
	{
		return APP_ERR_INV_ARG;
	}

	/* Validate public brokerUrl and port */
	if(config->port == 1883)
	{
		if(config->ctxType == MQTT_CTX_AGENT)
		{
			if(strstr(config->brokerUrl, "mqtt:") == NULL)
			{
				return APP_ERR_INV_ARG;
			}
		}
		else
		{
			/** TODO: There is no scheme check for AT manager, but the buffer should not consist the scheme */
			/** TODO: Remove scheme string if exists in brokerUrl for AT Manager */
		}
	}
	else if(config->port == 8883)
	{
		if(config->ctxType == MQTT_CTX_AGENT)
		{
			if(strstr(config->brokerUrl, "mqtts:") == NULL)
			{
				return APP_ERR_INV_ARG;
			}
		}
		else
		{
			/** NOTE: There is no scheme check for AT manager, but the buffer should not consist the scheme */
			/** TODO: Remove scheme string if exists in brokerUrl for AT Manager */
		}
	}
	else
	{
		/* Invalid port */
		return APP_ERR_INV_ARG;
	}

	if((config->rxQos >= 2) || (config->txQos >= 2) || (config->dataQos >= 2))
	{
		return APP_ERR_INV_ARG;
	}

	if((config->NwReadyWait== NULL) && (config->ctxType == MQTT_CTX_AGENT))
	{
		return APP_ERR_INV_ARG;
	}
	
	return APP_ERR_NONE;
}

int8_t MqttCommonMgrInit(mqttAgentConfig_t *config)
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
        /* Invalid config, set device in config mode */
//        ESP_LOGE(TAG, "Mqtt config Validation failed: %d", retVal);
		/* Set hasDefCfg to true, to start webpage in access point */
		SaveConfig(NULL, APP_NVS_SAVE_ALL);
	
//		ESP_LOGI(TAG, "Setting device into config mode!!!");
		vTaskDelay(pdMS_TO_TICKS(2000));
		esp_restart();
    }

    /* Declare tx and rx queues */
    config->mqttRxQHandle = xQueueCreate(MQTT_RX_QUEUE_SIZE, sizeof(mqttDataPkt_t *));
    configASSERT(config->mqttRxQHandle != NULL);

	config->mqttTxQHandle = xQueueCreate(MQTT_TX_QUEUE_SIZE, sizeof(mqttDataPkt_t *));
    configASSERT(config->mqttTxQHandle != NULL);

	/* Config is valid, store it */
	mqttConfig = config;

	if(config->ctxType == MQTT_CTX_AGENT)
	{
		/* Not implemented */
		retVal = APP_ERR_INV_ARG;
	}
	else
	{
		retVal = MqttATMgrInit(config);
	}

	return retVal;
}

int8_t MQTTWriteToPubQ(mqttDataPkt_t *dataPktPtr, uint32_t timeoutMs)
{
	/* Write to publish queue. */
	if(mqttConfig->mqttTxQHandle != NULL)
	{
		return (xQueueSend(mqttConfig->mqttTxQHandle, &dataPktPtr, pdMS_TO_TICKS(timeoutMs)) == pdPASS) ? APP_ERR_NONE : APP_ERR_TIMEOUT;
	}
	return APP_ERR_INV_STATE;
}

int8_t MQTTReadFromSubQ(mqttDataPkt_t **dataPktPtr, uint32_t timeoutMs)
{
	/* Read from sub queue. */
	if(mqttConfig->mqttRxQHandle == NULL)
	{
		return APP_ERR_INV_STATE;
	}
	return (xQueueReceive(mqttConfig->mqttRxQHandle, dataPktPtr, pdMS_TO_TICKS(timeoutMs)) == pdPASS) ? APP_ERR_NONE : APP_ERR_TIMEOUT;
}

int8_t MQTTWriteToSubQ(mqttDataPkt_t *dataPktPtr, uint32_t timeoutMs)
{
	/* Write to publish queue. */
	if(mqttConfig->mqttRxQHandle != NULL)
	{
		return (xQueueSend(mqttConfig->mqttRxQHandle, &dataPktPtr, pdMS_TO_TICKS(timeoutMs)) == pdPASS) ? APP_ERR_NONE : APP_ERR_TIMEOUT;
	}
	return APP_ERR_INV_STATE;
}

int8_t MQTTReadFromPubQ(mqttDataPkt_t **dataPktPtr, uint32_t timeoutMs)
{
	/* Read from sub queue. */
	if(mqttConfig->mqttTxQHandle == NULL)
	{
		return APP_ERR_INV_STATE;
	}
	return (xQueueReceive(mqttConfig->mqttTxQHandle, dataPktPtr, pdMS_TO_TICKS(timeoutMs)) == pdPASS) ? APP_ERR_NONE : APP_ERR_TIMEOUT;
}

int8_t MQTTIsSubQEmpty(void)
{
	/* Check if any pending msgs are there in sub queue. */
	if(mqttConfig->mqttRxQHandle != NULL)
	{
		return (uxQueueMessagesWaiting(mqttConfig->mqttRxQHandle) == 0);
	}
	return APP_ERR_INV_STATE;
}

int8_t MQTTIsPubQEmpty(void)
{
	/* Check if any pending msgs are there in pub queue. */
	if(mqttConfig->mqttTxQHandle != NULL)
	{
		return (uxQueueMessagesWaiting(mqttConfig->mqttTxQHandle) == 0);
	}
	return APP_ERR_INV_STATE;
}

int8_t MQTTGetPubQFreeSlots(void)
{
	/* Check if any pending msgs are there in pub queue. */
	if(mqttConfig->mqttTxQHandle != NULL)
	{
		return uxQueueSpacesAvailable(mqttConfig->mqttTxQHandle);
	}
	return APP_ERR_INV_STATE;
}

int8_t MQTTGetSubQFreeSlots(void)
{
	/* Check if any pending msgs are there in pub queue. */
	if(mqttConfig->mqttRxQHandle != NULL)
	{
		return uxQueueSpacesAvailable(mqttConfig->mqttRxQHandle);
	}
	return APP_ERR_INV_STATE;
}

#ifndef MANUAL_TEST
int8_t MQTTPublish(char *topic, char *msg, mqtt_pktid_t packetType, int32_t timeoutMs, uint8_t retainFlag)
{
    int8_t retVal = APP_ERR_INV_ARG;
    if((msg == NULL) || (strlen(msg) == 0))
    {
        return retVal;
    }

	int16_t msgLen = strlen(msg);
	mqttDataPkt_t *newCommMsg = NULL;

    retVal = APP_ERR_NO_MEM;
    newCommMsg = pvPortMalloc(sizeof(mqttDataPkt_t));
    if(newCommMsg == NULL)
    {
//		ESP_LOGE(TAG, "Failed to allocate memory for MQTT data packet.");
        return retVal;
    }

    newCommMsg->dataPtr = pvPortMalloc(sizeof(uint8_t) * (msgLen + 1));
    if(newCommMsg->dataPtr == NULL)
    {
        vPortFree(newCommMsg);   
//		ESP_LOGE(TAG, "Failed to allocate memory for Tx msg.");
        return retVal;
    }

    memcpy(newCommMsg->dataPtr, msg, msgLen);

	newCommMsg->dataPtr[msgLen] = '\0'; /* Null terminate the string */

	if(topic != NULL && strlen(topic) > 0)
	{
		newCommMsg->hasTopic = 1;
		newCommMsg->topicPtr = (uint8_t *)pvPortMalloc(sizeof(uint8_t) * (strlen(topic) + 1));
		if(newCommMsg->topicPtr == NULL)
		{
			vPortFree(newCommMsg->dataPtr);
			vPortFree(newCommMsg);   
//			ESP_LOGE(TAG, "Failed to allocate memory for Tx topic.");
			return APP_ERR_NO_MEM;
		}
		memcpy(newCommMsg->topicPtr, topic, strlen(topic));
		newCommMsg->topicPtr[strlen(topic)] = '\0'; /* Null terminate the string */
	}
	else
	{
		newCommMsg->hasTopic = 0;
		newCommMsg->topicPtr = NULL;
	}

    newCommMsg->dataLen = msgLen;
    newCommMsg->pktType = packetType;
    newCommMsg->retain = retainFlag;

    if(MQTTWriteToPubQ(newCommMsg, pdMS_TO_TICKS(timeoutMs)) == APP_ERR_NONE)
    {
//        ESP_LOGI(TAG, "MQTT send to Q success.");
        retVal = APP_ERR_NONE;
    }
    else
    {
        /* Failed to publish the message. De-allocate allocated memory. */
        vPortFree(newCommMsg->dataPtr);
        vPortFree(newCommMsg);
//        ESP_LOGI(TAG, "MQTT send to Q fail.");
        retVal = APP_ERR_NO_MEM;
    }

	return retVal;
}

int8_t MQTTGetServerConnectStatus(void)
{
	int8_t connStatus = 0;
	if(mqttConfig == NULL)
	{
		return connStatus;
	}

	if(mqttConfig->ctxType == MQTT_CTX_AGENT)
	{
		connStatus = 0;
	}
	else
	{
		connStatus = WaitMQTT_ATConnect(0);
	}

	return connStatus;
}
#elif (defined(TEST) || defined(MANUAL_TEST))
static int8_t connStatus = 0;

void MQTTMockServerConnectStatus(int8_t status)
{
	connStatus = status;
}

int8_t MQTTGetServerConnectStatus(void)
{
	return connStatus;
}

int8_t MQTTPublish(char *topic, char *msg, mqtt_pktid_t packetType, int32_t timeoutMs, uint8_t retainFlag)
{
	/* Mock MQTTPublish */
	return APP_ERR_NONE;
}

#endif
