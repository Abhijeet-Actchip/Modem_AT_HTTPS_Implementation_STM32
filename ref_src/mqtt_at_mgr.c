#include "mqtt_at_mgr.h"
#include "mqtt_common.h"
#include "bsp/board.h"
#include "app_hal/app_hal_delay.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#include <sys/time.h>

/* ============================================================================
 * Modem Abstraction Layer
 * Maps generic MODEM_* macros to modem-specific APIs based on SELECT_MODEM.
 * ============================================================================ */
#if (SELECT_MODEM == MODEM_TYPE_EC200)
    #include "modem_mgr/modem_ec200u.h"
    /* Instance type */
    #define modem_dce_inst_t                ec200_dce_t
    /* Struct types */
    #define modem_mqtt_pub_pkt_t            ec200_mqtt_pub_pkt_t
    #define modem_mqtt_rx_state_t           ec200_mqtt_rx_state_t
    #define modem_file_upl_ctx_t            ec200_file_upl_ctx_t
    /* Enum values */
    #define MODEM_SSL_SECLEVEL_2            EC200_SSL_SECLEVEL_2
    #define MODEM_MQTT_RX_BUFF_ID_MAX       EC200_MQTT_RX_BUFF_ID_MAX
    /* API mappings */
    #define MODEM_DCEInit                   EC200DCEInit
    #define MODEM_DCEPDPCtxConfig           EC200DCEPDPCtxConfig
    #define MODEM_DCEPDPCtxActivate         EC200DCEPDPCtxActivate
    #define MODEM_DCESetCACertPath          EC200DCESetCACertPath
    #define MODEM_DCESetClientCertPath      EC200DCESetClientCertPath
    #define MODEM_DCESetClientKeyPath       EC200DCESetClientKeyPath
    #define MODEM_DCESetSecurityLevel       EC200DCESetSecurityLevel
    #define MODEM_DCESetMQTTSSLEn           EC200DCESetMQTTSSLEn
    #define MODEM_DCESetMQTTRecvURCLenEn    EC200DCESetMQTTRecvURCLenEn
    #define MODEM_DCEMQTTOpen               EC200DCEMQTTOpen
    #define MODEM_DCEMQTTConnect            EC200DCEMQTTConnect
    #define MODEM_DCEMQTTSub                EC200DCEMQTTSub
    #define MODEM_DCEMQTTPub                EC200DCEMQTTPub
    #define MODEM_DCEMQTTGetRxStatus        EC200DCEMQTTGetRxStatus
    #define MODEM_DCEMQTTReadRxBuff         EC200DCEMQTTReadRxBuff
    #define MODEM_DCEMQTTDisConnect         EC200DCEMQTTDisConnect
    #define MODEM_DCEMQTTClose              EC200DCEMQTTClose
    #define MODEM_DCEDeleteFile             EC200DCEDeleteFile
    #define MODEM_DCEUploadFile             EC200DCEUploadFile

#else
    #error "Unsupported modem type selected. Check SELECT_MODEM in appconfig.h"
#endif

#define TAG                         "MQTT-AT-MGR"

#define PDP_CTX_IN_USE              1
#define SSL_CTX_IN_USE              0
#define MQTT_CTX_IN_USE             0
#define NETWORK_REG_DELAY_SEC       120
#define TIME_SYNC_DELAY_SEC         120
#define CA_CERT_PATH                "cacert.crt"
#define CLIENT_CERT_PATH            "clientcert.crt"
#define CLIENT_KEY_PATH             "clientkey.key"
#define TIME_SYNC_DUR_MSEC          ((1UL * 60*60UL) * 1000UL)
#define MAX_PUB_SUB_ERR_COUNT       2

typedef enum _nw_mgr_states_t
{
    NW_MGR_STATE_RESET = 0,
    NW_MGR_STATE_CHECK_SYNC,
    NW_MGR_STATE_ERROR,
	NW_MGR_STATE_DO_NW_SEL,
    NW_MGR_STATE_SYNC_SUCCESS,
    NW_MGR_STATE_WAIT_NW_REG,
    NW_MGR_STATE_WAIT_DATA_REG,
    NW_MGR_STATE_CONNECT
}nw_mgr_states_t;

typedef enum _mqtt_mgr_states_t
{
    MQTT_MGR_STATE_ACT_PDP_CTX = 0,
    MQTT_MGR_STATE_UL_CERTS,
    MQTT_MGR_STATE_CONF_SSL,
    MQTT_MGR_STATE_DO_CONNECT,
    MQTT_MGR_STATE_CONN_RETRY,
    MQTT_MGR_STATE_DO_SUB,
    MQTT_MGR_STATE_CONNECTED,
    MQTT_MGR_STATE_DO_DISCON

}mqtt_mgr_states_t;


#define MODEM_UART 0

static modem_dce_inst_t modemDCE;
static modem_dte_t dte = {
    .uartPid = MODEM_UART
};

static app_nvs_const_t nvsConfig;
static nw_mgr_states_t nwState = NW_MGR_STATE_CHECK_SYNC;
static mqtt_mgr_states_t mqttState = MQTT_MGR_STATE_ACT_PDP_CTX;
/* If it set, the code will upload TLS certs. As of now the certs
 * are uploaded on every CPU restart.
 */
static uint32_t sslCertUlFlag = 1;
static uint16_t msgId = 1;
static uint16_t pubsubErrCount = 0;

static void PowerOffModem(void)
{
//    ESP_LOGI(TAG, "POFF: Turning OFF Modem.");
	Board_CtrlModemPwrKey(0);
	APPHAL_Delay(2500);
	Board_CtrlModemPwrKey(1);
    APPHAL_Delay(5000);
//    ESP_LOGI(TAG, "POFF: Power off logic done.");
}

static void PowerOnModem(void)
{
//    ESP_LOGI(TAG, "PON: Turning ON Modem.");
    Board_EnableModemPower(1);
    APPHAL_Delay(30000);
}

void ResetModem(void)
{
	PowerOffModem();
	/* Wait for minimum 800ms before issuing power on cmd. */
	APPHAL_Delay(pdMS_TO_TICKS(1000));
	PowerOnModem();
}

static int8_t ModemEvtHandler(const char * const resp, const uint16_t len, void * args)
{
    if(strncmp(resp, "+QMTRECV", 8) == 0)
    {
        uint16_t rxLen = 0;
        mqttDataPkt_t *dataPktPtr = NULL;

        sscanf(resp, "%*[^\"]\"%*[^\"]\",%hd", &rxLen);
        if(rxLen > 0)
        {
            /* Check if control queue has empty slots. */
            if(MQTTIsSubQEmpty() > 0)
            {
                /* Allocate 1 data pkt. */
                dataPktPtr = calloc(1, sizeof(mqttDataPkt_t));
                if(dataPktPtr != NULL)
                {
                    dataPktPtr->pktType = MQTT_RX_PKT;
                    /* Allocating some extra bytes to compensate for '\0' &
                     * " being received in the messages. */
                    dataPktPtr->dataPtr = calloc(rxLen + 10, sizeof(char));
                    if(dataPktPtr->dataPtr != NULL)
                    {
                        int used;
                        sscanf(resp, "%*[^\"]\"%*[^\"]\"%*[^\"]\"%n", &used);
                        strncpy((char*)dataPktPtr->dataPtr, &resp[used], rxLen);
                        dataPktPtr->dataLen = rxLen;
                        /* Write to control rx queue. */
                        MQTTWriteToSubQ(dataPktPtr, 0);
                    }
                    else
                    {
                        free(dataPktPtr);
//                        ESP_LOGE(TAG, "MQTT Rx failed to alloc data buffer.");
                    }
                }
                else
                {
//                    ESP_LOGE(TAG, "MQTT Rx failed to alloc pkt.");
                }
            }
            else
            {
//                ESP_LOGE(TAG, "MQTT Rx queue full.");
            }
        }
    }
    
    return APP_ERR_NONE;
}

static int8_t SyncTime(void)
{
    char buff[64];
    int8_t retVal;
    retVal = ModemDCEGetTime((modem_dce_t *)&modemDCE, buff);
//    ESP_LOGI(TAG, "Time buff = %s", buff);

    if(retVal == APP_ERR_NONE)
    {
        retVal = APP_ERR_INV_PACKET;

        /* Parse time buffer and set local time. */
        struct tm ti = {0};
        uint16_t utcOffset = 0;
        struct timeval tv = {0};

        int ret = sscanf(buff, "\"%d/%d/%d,%d:%d:%d+%hd",
            &ti.tm_year, &ti.tm_mon, &ti.tm_mday,
            &ti.tm_hour, &ti.tm_min, &ti.tm_sec,
            &utcOffset);
        if(ret == 7)
        {
            ti.tm_year -= 1900;
            ti.tm_mon -= 1;
            tv.tv_sec = mktime(&ti);
            settimeofday(&tv, NULL);
            retVal = APP_ERR_NONE;
        }
    }
    return retVal;
}

static int8_t SSLCertsUl(void)
{
    int8_t retVal;

    MODEM_DCEDeleteFile(&modemDCE, CA_CERT_PATH);
    MODEM_DCEDeleteFile(&modemDCE, CLIENT_CERT_PATH);
    MODEM_DCEDeleteFile(&modemDCE, CLIENT_KEY_PATH);

    // const app_nvs_ssl_const_t * sslNvs = GetNvsSSLConfigPtr();
    const app_nvs_tls_const_t * tlsNvs = GetNvsTLSConfigPtr();
    modem_file_upl_ctx_t ctx = {};
    
    /* Upload CA cert. */
    ctx.fileName = CA_CERT_PATH;
    #if (APP_USE_TEST_CERTS == 1)
        ctx.dataBuff = (uint8_t *)test_ca_cert;
        ctx.dataLen =  strlen(test_ca_cert);
    #else
        ctx.dataBuff = (uint8_t *)tlsNvs->caCert;
        ctx.dataLen =  strlen(tlsNvs->caCert);
    #endif
    retVal = MODEM_DCEUploadFile(&modemDCE, &ctx);
    if(retVal == APP_ERR_NONE)
    {
//        ESP_LOGI(TAG, "Upload CA cert success, Len = %d", ctx.dataLen);
        /* Upload client cert. */
        ctx.fileName = CLIENT_CERT_PATH;
        #if (APP_USE_TEST_CERTS == 1)
            ctx.dataBuff = (uint8_t *)test_client_cert;
            ctx.dataLen = strlen(test_client_cert);
        #else
            ctx.dataBuff = (uint8_t *)tlsNvs->clientCert;
            ctx.dataLen =  strlen(tlsNvs->clientCert);
        #endif
        retVal = MODEM_DCEUploadFile(&modemDCE, &ctx);
        if(retVal == APP_ERR_NONE)
        {
//            ESP_LOGI(TAG, "Upload client cert success, Len = %d", ctx.dataLen);
            /* Upload client key. */
            ctx.fileName = CLIENT_KEY_PATH;
            #if (APP_USE_TEST_CERTS == 1)
                ctx.dataBuff = (uint8_t *)test_client_key;
                ctx.dataLen = strlen(test_client_key);
            #else
                ctx.dataBuff = (uint8_t *)tlsNvs->clientKey;
                ctx.dataLen =  strlen(tlsNvs->clientKey);
            #endif
            retVal = MODEM_DCEUploadFile(&modemDCE, &ctx);
//            ESP_LOGI(TAG, "Upload client key Len = %d", ctx.dataLen);
        }
    }

    return retVal;
}

static void NwMgmtStateMc(void)
{
    int8_t retVal;
    static int16_t delayCtr;
    uint8_t status;

    switch(nwState)
    {
        case NW_MGR_STATE_RESET:
//            ESP_LOGI(TAG, "Resetting modem.");
            ResetModem();
//            ESP_LOGI(TAG, "Reset done, Waiting before sync.");
            APPHAL_Delay(5000);
            nwState = NW_MGR_STATE_CHECK_SYNC;
            break;

        case NW_MGR_STATE_CHECK_SYNC:
            retVal = ModemDCESync((modem_dce_t *)&modemDCE);
            if(retVal == APP_ERR_NONE)
            {
                retVal = ModemDCEEcho((modem_dce_t *)&modemDCE, 0);
                if(retVal == APP_ERR_NONE)
                {
//                    ESP_LOGI(TAG, "Sync Echo Success.");
                    // nwState = NW_MGR_STATE_DO_NW_SEL;    // Skipping force network selection
                    nwState = NW_MGR_STATE_SYNC_SUCCESS;
                }
                else
                {
//                    ESP_LOGE(TAG, "Echo Failed.");
                    nwState = NW_MGR_STATE_ERROR;
                }
            }
            else
            {
//                ESP_LOGE(TAG, "Sync Failed.");
                nwState = NW_MGR_STATE_ERROR;
            }
            break;
        case NW_MGR_STATE_DO_NW_SEL:
        	APPHAL_Delay(2000);
        	/* Force select 2G mode on the modem. */
        	retVal = ModemDCESetNwScanMode((modem_dce_t *)&modemDCE, 1);
        	if(retVal == APP_ERR_NONE)
			{
//				ESP_LOGI(TAG, "2G network selection success.");
				nwState = NW_MGR_STATE_SYNC_SUCCESS;
			}
			else
			{
//				ESP_LOGE(TAG, "Unable to select 2G network.");
				nwState = NW_MGR_STATE_ERROR;
			}
            // retVal = ModemDCESetNwScanMode((modem_dce_t *)&modemDCE, 3);
        	// if(retVal == APP_ERR_NONE)
			// {
			// 	ESP_LOGI(TAG, "LTE network selection success.");
			// 	nwState = NW_MGR_STATE_SYNC_SUCCESS;
			// }
			// else
			// {
			// 	ESP_LOGE(TAG, "Unable to select LTE network.");
			// 	nwState = NW_MGR_STATE_ERROR;
			// }
        	break;
        case NW_MGR_STATE_SYNC_SUCCESS:
            APPHAL_Delay(2000);
            /* Set PDP context. TODO: Select below values from config. */
            retVal = MODEM_DCEPDPCtxConfig(&modemDCE, PDP_CTX_IN_USE,
                            APP_MODEM_PDP_CTX_TYPE_DEF, nvsConfig.nwConfig.apn);
            if(retVal == APP_ERR_NONE)
            {
//                ESP_LOGI(TAG, "Set PDP Ctx Success.");
                nwState = NW_MGR_STATE_WAIT_NW_REG;
                delayCtr = NETWORK_REG_DELAY_SEC;
            }
            else
            {
//                ESP_LOGE(TAG, "Unable to set PDP context.");
                nwState = NW_MGR_STATE_ERROR;
            }
            break;
        case NW_MGR_STATE_WAIT_NW_REG:
//            ESP_LOGI(TAG, "Wait N/W reg.");
            APPHAL_Delay(1000);
            status = -1;
            retVal = ModemDCEGetNwStatus((modem_dce_t *)&modemDCE, &status);
            if(retVal == APP_ERR_NONE)
            {
//                ESP_LOGI(TAG, "N/W Status = %d", status);
                if((status == 1) || (status == 5))
                {
//                    ESP_LOGI(TAG, "N/W registered");
                    nwState = NW_MGR_STATE_WAIT_DATA_REG;
                    delayCtr = NETWORK_REG_DELAY_SEC;
                    break;
                }
            }
            else
            {
//                ESP_LOGE(TAG, "Failed to read N/W status");
            }
            delayCtr--;
            if(delayCtr <= 0)
            {
//                ESP_LOGE(TAG, "Unable to register to N/W.");
                nwState = NW_MGR_STATE_ERROR;
            }
            break;
        case NW_MGR_STATE_WAIT_DATA_REG:
//            ESP_LOGI(TAG, "Wait GPRS reg.");
            APPHAL_Delay(1000);
            status = -1;
            retVal = ModemDCEGetGPRSStatus((modem_dce_t *)&modemDCE, &status);
            if(retVal == APP_ERR_NONE)
            {
//                ESP_LOGI(TAG, "GPRS Status = %d", status);
                if((status == 1) || (status == 5))
                {
                    ESP_LOGI(TAG, "GPRS registered");
                    nwState = NW_MGR_STATE_CONNECT;
                    mqttState = MQTT_MGR_STATE_ACT_PDP_CTX;
                    break;
                }
            }
            else
            {
//                ESP_LOGE(TAG, "Failed to read GPRS status");
            }
            delayCtr--;
            if(delayCtr <= 0)
            {
//                ESP_LOGE(TAG, "Unable to register to GPRS.");
                nwState = NW_MGR_STATE_ERROR;
            }
            break;
        case NW_MGR_STATE_CONNECT:
            /* Connected state. */
            break;
        case NW_MGR_STATE_ERROR:
//            ESP_LOGI(TAG, "Err reset delay");
            APPHAL_Delay(10000);
            nwState = NW_MGR_STATE_RESET;
            /* NOTE: This is to re-activate PDP context and let the device establish connection
               By this way, we are clearing the mqttState, so that when publishing to Q, 
               it fails and writes to log*/
            mqttState = MQTT_MGR_STATE_ACT_PDP_CTX; 
            break;   
    }
}

static void DoTimeSync(void)
{
    static uint8_t syncDone = 0;
    int8_t retVal;
    static TickType_t startTicks = 0;
    TickType_t diffTicks;

    if(!syncDone)
    {
        retVal = SyncTime();
        if(retVal == APP_ERR_NONE)
        {
            syncDone = 1;
            time_t now = 0;
            time(&now);
//            ESP_LOGI(TAG, "TS: %lld", now);
//            ESP_LOGI(TAG, "Time sync done.");
//            retVal = SyncRTCTime();
//            if(retVal != APP_ERR_NONE)
//            {
//                ESP_LOGE(TAG, "RTC sync failed with error %d", retVal);
//            }
        }
    }
    else
    {
        diffTicks = xTaskGetTickCount() - startTicks;
        if(diffTicks >= pdMS_TO_TICKS(TIME_SYNC_DUR_MSEC))
        {
            startTicks = xTaskGetTickCount();
            syncDone = 0;
//            ESP_LOGI(TAG, "Time sync started.");
        }
    }
}

static void ProcessTxQueue(mqttAgentConfig_t *config)
{
    int8_t retVal;
    mqttDataPkt_t *dataPktPtr = NULL;
    modem_mqtt_pub_pkt_t txDataPkt = {};

    /* Check if any cmd is to be sent on published topics. */
    if(MQTTReadFromPubQ(&dataPktPtr, pdMS_TO_TICKS(1000)) == APP_ERR_NONE)
    {
        if(dataPktPtr != NULL)
        {
            if(dataPktPtr->dataLen > 0)
            {
                txDataPkt.mqttCtxId = MQTT_CTX_IN_USE;
                txDataPkt.msgId = msgId++;

                switch(dataPktPtr->pktType)
                {
                    case MQTT_TX_PKT:
//                        ESP_LOGI(TAG, "Publishing on TX topic.");
                        if(dataPktPtr->hasTopic)
                        {
                            txDataPkt.topic = (char *)dataPktPtr->topicPtr;
                        }
                        else
                        {
                            txDataPkt.topic = NULL;
                        }
                        txDataPkt.qos = config->txQos;
                        break;

                    case MQTT_DATA_PKT:
//                        ESP_LOGI(TAG, "Publishing on DATA topic.");
                        if(dataPktPtr->hasTopic)
                        {
                            txDataPkt.topic = (char *)dataPktPtr->topicPtr;
                        }
                        else
                        {
                            txDataPkt.topic = NULL;
                        }
                        txDataPkt.qos = config->dataQos;
                        break;

                    default:
                        break;
                }

                txDataPkt.data = (char *)dataPktPtr->dataPtr;
                txDataPkt.dataLen = dataPktPtr->dataLen;
                txDataPkt.retain = dataPktPtr->retain;
                
                retVal = MODEM_DCEMQTTPub(&modemDCE, &txDataPkt);
//                ESP_LOGI(TAG, "PUB-STATUS = %d", retVal);

                if(retVal != APP_ERR_NONE)
                {
                    pubsubErrCount++;
                    if(pubsubErrCount >= MAX_PUB_SUB_ERR_COUNT)
                    {
                        /* Reset N/W mgr state m/c. */
                        nwState = NW_MGR_STATE_ERROR;
                        pubsubErrCount = 0;
                    }
                }
                else
                {
                    pubsubErrCount = 0;
                }
            }
            
            /* Free memory */
            free(dataPktPtr->dataPtr);
            free(dataPktPtr->topicPtr);
            free(dataPktPtr);
            dataPktPtr = NULL;
        }
    }
}

static void ProcessRxQueue()
{
    mqttDataPkt_t *dataPktPtr = NULL;

    if(MQTTReadFromSubQ(&dataPktPtr, 1000) == pdTRUE)
    {
        if(dataPktPtr != NULL)
        {
            if(dataPktPtr->dataLen > 0)
            {
//                ESP_LOGI(TAG, "Rx Data: %s, Len = %u ",(char *)dataPktPtr->dataPtr, dataPktPtr->dataLen);
            }
            
            /* Free memory */
            free(dataPktPtr->dataPtr);
            free(dataPktPtr);
            dataPktPtr = NULL;
        }
    }
}

static void PollRxStatus(mqttAgentConfig_t *config)
{
    modem_mqtt_rx_state_t rxState = {};
    rxState.mqttCtxId = MQTT_CTX_IN_USE;

    int8_t err = MODEM_DCEMQTTGetRxStatus(&modemDCE, &rxState);

    if(err == APP_ERR_NONE)
    {
        for(uint8_t i = 0; i <= MODEM_MQTT_RX_BUFF_ID_MAX; i++)
        {
            /* If data available in buffer */
            if(rxState.rxState & (1<<i))
            {
                /* Its response will be processed in the rx event. */
                MODEM_DCEMQTTReadRxBuff(&modemDCE, MQTT_CTX_IN_USE, i);
            }
        }
    }
    else
    {
//        ESP_LOGE(TAG, "Unable to read rx status = %d", err);
        pubsubErrCount++;
        if(pubsubErrCount >= MAX_PUB_SUB_ERR_COUNT)
        {
            /* Reset N/W mgr state m/c. */
            nwState = NW_MGR_STATE_ERROR;
            pubsubErrCount = 0;
        }
    }
}

static void MqttMgmtStateMc(mqttAgentConfig_t *config)
{
    int8_t retVal;
    static int16_t retryCtr = 0;

    switch(mqttState)
    {
        case MQTT_MGR_STATE_ACT_PDP_CTX:
            retVal = MODEM_DCEPDPCtxActivate(&modemDCE, PDP_CTX_IN_USE);
            if(retVal == APP_ERR_NONE)
            {
//                ESP_LOGI(TAG, "PDP ctx activation successful.");
                if(config->useTLS)
                {
                    mqttState = MQTT_MGR_STATE_UL_CERTS;
                }
                else
                {
                    mqttState = MQTT_MGR_STATE_DO_CONNECT;
                }
            }
            else
            {
//                ESP_LOGE(TAG, "Failed to activate PDP ctx");
                /* Reset N/W mgr state m/c. */
                nwState = NW_MGR_STATE_ERROR;
            }
            break;
        case MQTT_MGR_STATE_UL_CERTS:
            if(sslCertUlFlag)
            {
//                ESP_LOGI(TAG, "Uploading certs");
                retVal = SSLCertsUl();
                if(retVal == APP_ERR_NONE)
                {
//                    ESP_LOGI(TAG, "Upload client key success.");
                    /* Cert UL is only done once after reset. */
                    sslCertUlFlag = 0;
                    mqttState = MQTT_MGR_STATE_CONF_SSL;
                }
                else
                {
//                    ESP_LOGE(TAG, "Unable to upload certs");
                    /* Reset N/W mgr state m/c. */
                    nwState = NW_MGR_STATE_ERROR;
                }
            }
            else
            {
//                ESP_LOGI(TAG, "Not uploading certs");
                mqttState = MQTT_MGR_STATE_CONF_SSL;
            }
            break;
        case MQTT_MGR_STATE_CONF_SSL:
            retVal = MODEM_DCESetCACertPath(&modemDCE, SSL_CTX_IN_USE, CA_CERT_PATH);
            if(retVal != APP_ERR_NONE)
            {
//                ESP_LOGE(TAG, "Unable to set cacert path.");
                /* Reset N/W mgr state m/c. */
                nwState = NW_MGR_STATE_ERROR;
                break;
            }
//            ESP_LOGI(TAG, "Set CA cert path success.");
            retVal = MODEM_DCESetClientCertPath(&modemDCE, SSL_CTX_IN_USE, CLIENT_CERT_PATH);
            if(retVal != APP_ERR_NONE)
            {
//                ESP_LOGE(TAG, "Unable to set client cert path.");
                /* Reset N/W mgr state m/c. */
                nwState = NW_MGR_STATE_ERROR;
                break;
            }
//            ESP_LOGI(TAG, "Set client cert path success.");
            retVal = MODEM_DCESetClientKeyPath(&modemDCE, SSL_CTX_IN_USE, CLIENT_KEY_PATH);
            if(retVal != APP_ERR_NONE)
            {
//                ESP_LOGE(TAG, "Unable to set client key path.");
                /* Reset N/W mgr state m/c. */
                nwState = NW_MGR_STATE_ERROR;
                break;
            }
//            ESP_LOGI(TAG, "Set client key path success.");
            retVal = MODEM_DCESetSecurityLevel(&modemDCE, SSL_CTX_IN_USE, MODEM_SSL_SECLEVEL_2);
            if(retVal != APP_ERR_NONE)
            {
//                ESP_LOGE(TAG, "Unable to set security level");
                /* Reset N/W mgr state m/c. */
                nwState = NW_MGR_STATE_ERROR;
                break;
            }
//            ESP_LOGI(TAG, "Set security level success.");
            retVal = MODEM_DCESetMQTTSSLEn(&modemDCE, MQTT_CTX_IN_USE, SSL_CTX_IN_USE, 1);
            if(retVal != APP_ERR_NONE)
            {
                ESP_LOGE(TAG, "Unable to enable MQTT security");
                /* Reset N/W mgr state m/c. */
                nwState = NW_MGR_STATE_ERROR;
                break;
            }
//            ESP_LOGI(TAG, "Set MQTT security success.");
            retryCtr = 3;
            mqttState = MQTT_MGR_STATE_DO_CONNECT;
            break;
        case MQTT_MGR_STATE_DO_CONNECT:
            pubsubErrCount = 0;
            /* Enable length info in subscribe URC. */
            MODEM_DCESetMQTTRecvURCLenEn(&modemDCE, MQTT_CTX_IN_USE, 1);
            
//            ESP_LOGI(TAG, "MQTT send connect: %s, %d", config->brokerUrl, config->port);
            retVal = MODEM_DCEMQTTOpen(&modemDCE, MQTT_CTX_IN_USE, 
                                    config->brokerUrl, config->port);
            if(retVal != APP_ERR_NONE)
            {
//                ESP_LOGE(TAG, "Unable to open MQTT connection.");
                mqttState = MQTT_MGR_STATE_CONN_RETRY;
                break;
            }
            retVal = MODEM_DCEMQTTConnect(&modemDCE, MQTT_CTX_IN_USE,
                                    config->clientId, config->uname, config->passwd);
            if(retVal != APP_ERR_NONE)
            {
//                ESP_LOGE(TAG, "Unable to connect MQTT.");
                mqttState = MQTT_MGR_STATE_CONN_RETRY;
                break;
            }
//            ESP_LOGI(TAG, "MQTT connected.");
            mqttState = MQTT_MGR_STATE_DO_SUB;
            break;
        case MQTT_MGR_STATE_CONN_RETRY:
            APPHAL_Delay(5000);
            retryCtr--;
            if(retryCtr > 0)
            {
//                ESP_LOGI(TAG, "Retry MQTT connect.");
                mqttState = MQTT_MGR_STATE_DO_CONNECT;
            }
            else
            {
                /* Reset N/W mgr state m/c. */
                nwState = NW_MGR_STATE_ERROR;
            }
            break;
        case MQTT_MGR_STATE_DO_SUB:
            if((!config->skipSubscribe) && (strlen(config->topicRx1) > 0) && (strlen(config->topicRx2) > 0))
            {
                retVal = MODEM_DCEMQTTSub(&modemDCE, MQTT_CTX_IN_USE, msgId,
                            config->topicRx1, config->rxQos);
                if(retVal != APP_ERR_NONE)
                {
//                    ESP_LOGE(TAG, "Unable to MQTT subscribe to topicRx1.");
                    mqttState = MQTT_MGR_STATE_DO_DISCON;
                    break;
                }

                retVal = MODEM_DCEMQTTSub(&modemDCE, MQTT_CTX_IN_USE, msgId,
                            config->topicRx2, config->rxQos);
                if(retVal != APP_ERR_NONE)
                {
                    ESP_LOGE(TAG, "Unable to MQTT subscribe to topicRx2.");
                    mqttState = MQTT_MGR_STATE_DO_DISCON;
                    break;
                }
            }
            mqttState = MQTT_MGR_STATE_CONNECTED;
            break;
        case MQTT_MGR_STATE_CONNECTED:
            /* Connected state. */
            ProcessTxQueue(config);
            PollRxStatus(config);
            ProcessRxQueue();
            DoTimeSync();
            if(msgId == 0)
            {
                msgId++;
            }
            /* TODO: Check connectivity status and update connect flag. */
            break;
        case MQTT_MGR_STATE_DO_DISCON:
            MODEM_DCEMQTTDisConnect(&modemDCE, MQTT_CTX_IN_USE);
            MODEM_DCEMQTTClose(&modemDCE, MQTT_CTX_IN_USE);
            mqttState = MQTT_MGR_STATE_CONN_RETRY;
            break;
    }
}

static void MqttMgrTask(void *pvArgs)
{
    mqttAgentConfig_t *config = (mqttAgentConfig_t *)pvArgs;

    /* Set SSL certificate upload flag */
    sslCertUlFlag = config->useTLS;

    /* Wait initially for the modem to power up. */
    APPHAL_Delay(5000);
    PowerOnModem();
    APPHAL_Delay(1000);

    while (1)
    {
        /* If connected to the cellular network ?*/
        if(nwState == NW_MGR_STATE_CONNECT)
        {
            MqttMgmtStateMc(config);
        }
        else
        {
            NwMgmtStateMc();
        }
    }
}

int8_t MqttATMgrInit(mqttAgentConfig_t *config)
{
    int8_t retVal;

    configASSERT(config != NULL);

    /* MQTT Topics */
//	ESP_LOGI(TAG, "Broker %s", config->brokerUrl);
//	ESP_LOGI(TAG, "Port %d", config->port);
//	ESP_LOGI(TAG, "Client ID %s", config->clientId);
//	ESP_LOGI(TAG, "Topic Data 1 %s", config->topicData1);
//	ESP_LOGI(TAG, "Topic Ctrl Rx 1 %s", config->topicRx1);
//	ESP_LOGI(TAG, "Topic Ctrl Tx 1 %s", config->topicTx1);
//
//    ESP_LOGI(TAG, "Topic Data 2 %s", config->topicData2);
//	ESP_LOGI(TAG, "Topic Ctrl Rx 2 %s", config->topicRx2);
//	ESP_LOGI(TAG, "Topic Ctrl Tx 2 %s", config->topicTx2);

    retVal = MODEM_DCEInit(&modemDCE, &dte); 
    configASSERT(retVal == APP_ERR_NONE);

    retVal = ModemDTERegEvtHandler(&dte, ModemEvtHandler);
    configASSERT(retVal == APP_ERR_NONE);

//    GetCurrentConfig(&nvsConfig);

    /* Start MqttMgrTask. */
	configASSERT(xTaskCreate(MqttMgrTask, "MQTT", MQTT_TASK_STACKSIZE, config, MQTT_TASK_PRIORITY, NULL) == pdTRUE);

    return retVal;
}

uint8_t WaitMQTT_ATConnect(int32_t timeoutMs)
{
    while (timeoutMs > 0)
    {
        if(mqttState != MQTT_MGR_STATE_CONNECTED)
        {
            APPHAL_Delay(500);
            timeoutMs -= 500;
        }
        else
        {
            break;
        }
    }
    
    return (mqttState == MQTT_MGR_STATE_CONNECTED);
}
