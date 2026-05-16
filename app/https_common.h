#ifndef __HTTPS_COMMON_H__
#define __HTTPS_COMMON_H__      1

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "config/appconfig.h"


/* HTTP Content Types */
typedef enum {
    HTTP_CONTENT_PLAIN_TEXT = 0,
    HTTP_CONTENT_APPLICATION_X_WWW_FORM_URLENCODED = 1,
    HTTP_CONTENT_APPLICATION_JSON = 2,
    HTTP_CONTENT_MULTIPART_FORM_DATA = 3
} httpContentType_t;

typedef struct _httpsAgentConfig_t {
    /* Common fields */
    char baseUrl[256];
    httpContentType_t contentType;
    char writeApiKey[32];
    char readApiKey[32];
    uint16_t timeoutMs;
    uint16_t responseTimeMs;
    uint8_t useTLS;

    /* Network Ready Wait function pointer */
    uint32_t (*NwReadyWait)(void);
    uint8_t nwReadyBit;

    /* Private fields */
    QueueHandle_t httpsRxQHandle;
    QueueHandle_t httpsTxQHandle;
} httpsAgentConfig_t;

/* Packet type for HTTPS data */
typedef enum
{
    HTTPS_RX_PKT,
    HTTPS_TX_PKT
} https_pktid_t;

typedef struct {
    https_pktid_t pktType;
    uint8_t *dataPtr;
    int dataLen;
} httpsDataPkt_t;

int8_t HttpsCommonMgrInit(httpsAgentConfig_t *config);

int8_t HttpsSetWriteApiKey(const char *key);
int8_t HttpsSetReadApiKey(const char *key);

int8_t HTTPSWriteToRxQ(httpsDataPkt_t *dataPktPtr, uint32_t timeoutMs);
int8_t HTTPSReadFromRxQ(httpsDataPkt_t **dataPktPtr, uint32_t timeoutMs);
int8_t HTTPSWriteToTxQ(httpsDataPkt_t *dataPktPtr, uint32_t timeoutMs);
int8_t HTTPSReadFromTxQ(httpsDataPkt_t **dataPktPtr, uint32_t timeoutMs);

int8_t HTTPSIsRxQEmpty(void);
int8_t HTTPSIsTxQEmpty(void);
int8_t HTTPSGetRxQFreeSlots(void);
int8_t HTTPSGetTxQFreeSlots(void);

int8_t HTTPSSendData(char *msg, int32_t timeoutMs);
int8_t HTTPSGetConnectStatus(void);

#endif /* __HTTPS_COMMON_H__ */
