#ifndef	__MQTT_COMMON_H__
#define __MQTT_COMMON_H__        1       

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h" 

/* Context type for MQTT modules */
typedef enum
{
    MQTT_CTX_AT_MGR = 0,
    MQTT_CTX_AGENT
}mqttCtx_t;

/* Packet type for MQTT data */
typedef enum
{
	MQTT_RX_PKT,
	MQTT_TX_PKT,
	MQTT_DATA_PKT
}mqtt_pktid_t;

typedef struct _mqttAgentConfig_t
{
    /* Common fields */
	char brokerUrl[128];
	uint16_t port;
	char clientId[30];
	/* Channel 1 */
	char topicData1[256];
	char topicRx1[256], topicTx1[256];
	/* Channel 2 */
	char topicData2[256];
	char topicRx2[256], topicTx2[256];
	
	char uname[64], passwd[64];
	uint8_t rxQos, txQos, dataQos;
	uint8_t skipSubscribe;
	uint16_t txAckTimeoutMs;
	uint16_t interPktDelayMs;
	uint8_t useTLS;

    /* Network Ready Wait function pointer, 
       applicable for MQTT_CTX_AGENT */
	uint32_t (*NwReadyWait)(void);
	uint8_t nwReadyBit;

    /* Private fields */
    QueueHandle_t mqttRxQHandle;
    QueueHandle_t mqttTxQHandle;
    mqttCtx_t ctxType;
}mqttAgentConfig_t;

typedef struct
{
	mqtt_pktid_t pktType;
	uint8_t hasTopic;
	uint8_t *topicPtr;
	uint8_t *dataPtr;
	int dataLen;
	uint8_t ackTimeOut ;
    uint8_t retain;
}mqttDataPkt_t;

int8_t MqttCommonMgrInit(mqttAgentConfig_t *config);

int8_t MQTTWriteToSubQ(mqttDataPkt_t *dataPktPtr, uint32_t timeoutMs);
int8_t MQTTReadFromSubQ(mqttDataPkt_t **dataPktPtr, uint32_t timeoutMs);
int8_t MQTTWriteToPubQ(mqttDataPkt_t *dataPktPtr, uint32_t timeoutMs);
int8_t MQTTReadFromPubQ(mqttDataPkt_t **dataPktPtr, uint32_t timeoutMs);
int8_t MQTTIsSubQEmpty(void);
int8_t MQTTIsPubQEmpty(void);
int8_t MQTTGetSubQFreeSlots(void);
int8_t MQTTGetPubQFreeSlots(void);

int8_t MQTTPublish(char *topic, char *msg, mqtt_pktid_t packetType, int32_t timeoutMs, uint8_t retainFlag);
int8_t MQTTGetServerConnectStatus(void);


#endif /* __MQTT_COMMON_H__ */