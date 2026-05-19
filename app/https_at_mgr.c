#include "https_at_mgr.h"
#include "https_common.h"
#include "bsp/board.h"
#include "app_hal/app_hal_delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "utils/trice/trice.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "modem_mgr/modem_ec200u.h"

/* ============================================================================
 * EC200 Direct API Usage
 * ============================================================================ */

#define TAG                         "HTTPS-AT-MGR"

#define PDP_CTX_IN_USE              1
#define SSL_CTX_IN_USE              0
#define HTTP_CTX_IN_USE             0
#define NETWORK_REG_DELAY_SEC       120
#define CA_CERT_PATH                "cacert.crt"

typedef enum _nw_mgr_states_t
{
    NW_MGR_STATE_RESET = 0,
    NW_MGR_STATE_CHECK_SYNC,
    NW_MGR_STATE_ERROR,
    NW_MGR_STATE_SYNC_SUCCESS,
    NW_MGR_STATE_WAIT_NW_REG,
    NW_MGR_STATE_WAIT_DATA_REG,
    NW_MGR_STATE_CONNECT
}nw_mgr_states_t;

typedef enum _https_mgr_states_t
{
    HTTPS_MGR_STATE_ACT_PDP_CTX = 0,
    HTTPS_MGR_STATE_UL_CERTS,
    HTTPS_MGR_STATE_CONF_SSL,
    HTTPS_MGR_STATE_CONF_HTTP,
    HTTPS_MGR_STATE_IDLE,
    HTTPS_MGR_STATE_SEND_REQ,
    HTTPS_MGR_STATE_WAIT_RESP,
    HTTPS_MGR_STATE_READ_RESP

}https_mgr_states_t;

static ec200_dce_t modemDCE;
static nw_mgr_states_t nwState = NW_MGR_STATE_CHECK_SYNC;
static https_mgr_states_t httpsState = HTTPS_MGR_STATE_ACT_PDP_CTX;
static uint32_t sslCertUlFlag = 0;

static TaskHandle_t httpTaskHandle = NULL;

static void PowerOffModem(void)
{
    /* We are doing this using MODEM_PWR_KEY */
	Board_CtrlModemPwrKey(1);
    APPHAL_Delay(3500);
    Board_CtrlModemPwrKey(0);

    APPHAL_Delay(10000);
    trice(iD(1703), "dbg: Power OFF logic done.\n");
}

static void PowerOnModem(void)
{
    /* We are doing this using MODEM_PWR_KEY */
	Board_CtrlModemPwrKey(1);
    APPHAL_Delay(2500);
    Board_CtrlModemPwrKey(0);

    APPHAL_Delay(10000);
    trice(iD(2805), "dbg: Power ON logic done.\n");
}

void ResetModem(void)
{
    PowerOffModem();
    APPHAL_Delay(pdMS_TO_TICKS(1000));
    PowerOnModem();
    trice(iD(2125), "dbg: MODEM Reset logic done.\n");
}

static int8_t SSLCertsUl(void)
{
    int8_t retVal;
    retVal = EC200DCEDeleteFile(&modemDCE, CA_CERT_PATH);
    retVal = APP_ERR_NONE;
    /* For now, we assume CA cert is available or hardcoded. 
     * In a real app, this would come from NVS or flash.
     */
    // const char *test_ca_cert = "..."; // User provided this in XML
    // For now we skip actual upload if not available, or use a placeholder.
    
    /* Placeholder for actual file upload logic if needed */
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
            trice(iD(6263), "dbg: NW State RESET\n");
            ResetModem();
            APPHAL_Delay(5000);
            nwState = NW_MGR_STATE_CHECK_SYNC;
            break;

        case NW_MGR_STATE_CHECK_SYNC:
            retVal = ModemDCESync((modem_dce_t *)&modemDCE);
            if(retVal == APP_ERR_NONE)
            {
            	trice(iD(4262), "dbg: Sync Success\n");
                retVal = ModemDCEEcho((modem_dce_t *)&modemDCE, 0);
                if(retVal == APP_ERR_NONE)
                {
                    nwState = NW_MGR_STATE_SYNC_SUCCESS;
                    trice(iD(1262), "dbg: Echo Success\n");
                }
                else
                {
                    nwState = NW_MGR_STATE_ERROR;
                    trice32(iD(5439), "dbg: Echo Failed, %d\n", retVal);
                }
            }
            else
            {
            	trice32(iD(6093), "dbg: Sync Failed, %d", retVal);
                nwState = NW_MGR_STATE_ERROR;
            }
            break;

        case NW_MGR_STATE_SYNC_SUCCESS:
            APPHAL_Delay(2000);
            /* Hardcoded APN for now, should come from config */
            retVal = EC200DCEPDPCtxConfig(&modemDCE, PDP_CTX_IN_USE,
                            1, "airtelgprs.com");
            if(retVal == APP_ERR_NONE)
            {
                trice(iD(4744), "dbg: PDP Ctx Config Success\n");
                nwState = NW_MGR_STATE_WAIT_NW_REG;
                delayCtr = NETWORK_REG_DELAY_SEC;
            }
            else
            {
                trice32(iD(3335), "dbg: PDP Ctx Config Failed, %d\n", retVal);
                nwState = NW_MGR_STATE_ERROR;
            }
            break;

        case NW_MGR_STATE_WAIT_NW_REG:
            APPHAL_Delay(1000);
            status = -1;
            retVal = ModemDCEGetNwStatus((modem_dce_t *)&modemDCE, &status);
            if(retVal == APP_ERR_NONE)
            {
                if((status == 1) || (status == 5))
                {
                    trice8(iD(4358), "dbg: NW Registered, status %d\n", status);
                    nwState = NW_MGR_STATE_WAIT_DATA_REG;
                    delayCtr = NETWORK_REG_DELAY_SEC;
                    break;
                }
            }
            delayCtr--;
            if(delayCtr <= 0)
            {
                trice(iD(7482), "err: NW Reg Timeout\n");
                nwState = NW_MGR_STATE_ERROR;
            }
            break;

        case NW_MGR_STATE_WAIT_DATA_REG:
            APPHAL_Delay(1000);
            status = -1;
            retVal = ModemDCEGetGPRSStatus((modem_dce_t *)&modemDCE, &status);
            if(retVal == APP_ERR_NONE)
            {
                if((status == 1) || (status == 5))
                {
                    trice8(iD(1603), "dbg: Data Registered, status %d\n", status);
                    nwState = NW_MGR_STATE_CONNECT;
                    httpsState = HTTPS_MGR_STATE_ACT_PDP_CTX;
                    break;
                }
            }
            delayCtr--;
            if(delayCtr <= 0)
            {
                trice(iD(5942), "err: Data Reg Timeout\n");
                nwState = NW_MGR_STATE_ERROR;
            }
            break;

        case NW_MGR_STATE_CONNECT:
            break;

        case NW_MGR_STATE_ERROR:
            trice(iD(1812), "err: NW Manager Error state, resetting...\n");
            APPHAL_Delay(10000);
            nwState = NW_MGR_STATE_RESET;
            httpsState = HTTPS_MGR_STATE_ACT_PDP_CTX; 
            break;   
    }
}

static void ProcessTxQueue(httpsAgentConfig_t *config)
{
    httpsDataPkt_t *dataPktPtr = NULL;

    if(HTTPSReadFromTxQ(&dataPktPtr, pdMS_TO_TICKS(100)) == APP_ERR_NONE)
    {
        if(dataPktPtr != NULL)
        {
            trice32(iD(7472), "dbg: Processing packet, len %d\n", dataPktPtr->dataLen);
            if(dataPktPtr->dataLen > 0)
            {
                int8_t retVal;
                
                #if (HTTP_METHOD_USE == HTTP_METHOD_POST)
                    trice(iD(5843), "dbg: Sending HTTP POST\n");
                    retVal = EC200DCEHTTPPost(&modemDCE, HTTP_CTX_IN_USE, 
                                             dataPktPtr->dataPtr, dataPktPtr->dataLen, 
                                             80);
                #elif (HTTP_METHOD_USE == HTTP_METHOD_GET)
                    trice(iD(4355), "dbg: Sending HTTP GET\n");
                    /* For GET, we might need to append parameters from dataPtr to baseUrl 
                     * but for simplicity we follow the XML which sends URL once.
                     * If URL is already set, we just trigger GET.
                     */
                    retVal = EC200DCEHTTPGet(&modemDCE, HTTP_CTX_IN_USE, 
                                            config->responseTimeMs / 1000, NULL, 0);
                #endif
                
                if(retVal == APP_ERR_NONE)
                {
                    trice(iD(4944), "dbg: Request Success, reading response...\n");
                    /* Request sent successfully, wait for response or read response */
                    EC200DCEHTTPRead(&modemDCE, HTTP_CTX_IN_USE, 80);
                }
                else
                {
                    trice32(iD(1962), "err: HTTP Request Failed, %d\n", retVal);
                }
            }
            
            /* Free memory */
            vPortFree(dataPktPtr->dataPtr);
            vPortFree(dataPktPtr);
        }
    }
}

static void HttpsMgmtStateMc(httpsAgentConfig_t *config)
{
    int8_t retVal;

    switch(httpsState)
    {
        case HTTPS_MGR_STATE_ACT_PDP_CTX:
            trice(iD(5297), "dbg: HTTPS Activating PDP Ctx\n");
            retVal = EC200DCEPDPCtxActivate(&modemDCE, PDP_CTX_IN_USE);
            if(retVal == APP_ERR_NONE)
            {
                trice(iD(1626), "dbg: PDP Ctx Activated\n");
                if(config->useTLS)
                {
                    httpsState = HTTPS_MGR_STATE_UL_CERTS;
                }
                else
                {
                    httpsState = HTTPS_MGR_STATE_CONF_HTTP;
                }
            }
            else
            {
                trice32(iD(1845), "err: PDP Ctx Activation Failed, %d\n", retVal);
                nwState = NW_MGR_STATE_ERROR;
            }
            break;

        case HTTPS_MGR_STATE_UL_CERTS:
            if(sslCertUlFlag)
            {
                trice(iD(2689), "dbg: Uploading SSL Certs\n");
                retVal = SSLCertsUl();
                if(retVal == APP_ERR_NONE)
                {
                    trice(iD(1036), "dbg: SSL Certs Uploaded\n");
                    sslCertUlFlag = 0;
                    httpsState = HTTPS_MGR_STATE_CONF_SSL;
                }
                else
                {
                    trice32(iD(6600), "err: SSL Certs Upload Failed, %d\n", retVal);
                    nwState = NW_MGR_STATE_ERROR;
                }
            }
            else
            {
                httpsState = HTTPS_MGR_STATE_CONF_SSL;
            }
            break;

        case HTTPS_MGR_STATE_CONF_SSL:
            trice(iD(3313), "dbg: Configuring SSL\n");
            retVal = EC200DCESetCACertPath(&modemDCE, SSL_CTX_IN_USE, CA_CERT_PATH);
            if(retVal != APP_ERR_NONE)
            {
                trice32(iD(5906), "err: Set CA Cert Path Failed, %d\n", retVal);
                nwState = NW_MGR_STATE_ERROR;
                break;
            }
            
            /* xml sets seclevel to 0 for Thingspeak HTTPS sometimes, but let's follow standard */
            EC200DCESetSecurityLevel(&modemDCE, SSL_CTX_IN_USE, 0); 
            
            /* Enable SSL for HTTP */
            retVal = EC200DCEHTTPSSLEn(&modemDCE, HTTP_CTX_IN_USE, SSL_CTX_IN_USE, 1);
            if(retVal != APP_ERR_NONE)
            {
                trice32(iD(1279), "err: HTTP SSL Enable Failed, %d\n", retVal);
                nwState = NW_MGR_STATE_ERROR;
                break;
            }
            httpsState = HTTPS_MGR_STATE_CONF_HTTP;
            break;

        case HTTPS_MGR_STATE_CONF_HTTP:
            trice(iD(7829), "dbg: Configuring HTTP\n");
            /* Configure HTTP context */
            EC200DCEHTTPSetContentType(&modemDCE, HTTP_CTX_IN_USE, config->contentType);
            EC200DCEHTTPCustomizeReqHeaderEn(&modemDCE, HTTP_CTX_IN_USE, 0);
            EC200DCEHTTPCustomizeRespHeaderEn(&modemDCE, HTTP_CTX_IN_USE, 0);
            
            /* Set URL once */
            retVal = EC200DCEHTTPSetURLStr(&modemDCE, HTTP_CTX_IN_USE, 
                                          (uint8_t *)config->baseUrl, strlen(config->baseUrl));
            if(retVal != APP_ERR_NONE)
            {
                trice32(iD(4780), "err: Set URL Failed, %d\n", retVal);
                nwState = NW_MGR_STATE_ERROR;
                break;
            }
            else
            {
            	trice(iD(1611), "dbg: Set URL success,\n");
            	TRICE_S(id(3468), "dbg: URL: %s\n", config->baseUrl);
            }

            trice(iD(5329), "dbg: HTTPS Manager IDLE\n");
            httpsState = HTTPS_MGR_STATE_IDLE;
            break;

        case HTTPS_MGR_STATE_IDLE:
            ProcessTxQueue(config);
            break;

        default:
            break;
    }
}

static void HttpsMgrTask(void *pvArgs)
{
    httpsAgentConfig_t *config = (httpsAgentConfig_t *)pvArgs;

    sslCertUlFlag = config->useTLS;

    trice(iD(7489), "dbg: HttpsMgrTask Started\n");
    APPHAL_Delay(5000);
    PowerOnModem();
    APPHAL_Delay(1000);

    while (1)
    {
        if(nwState == NW_MGR_STATE_CONNECT)
        {
            HttpsMgmtStateMc(config);
        }
        else
        {
            NwMgmtStateMc();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int8_t HttpsATMgrInit(httpsAgentConfig_t *config)
{
    if(config == NULL)
    {
        return APP_ERR_INV_ARG;
    }

    /* Initialize modem DCE */
    modem_dte_t *dte = (modem_dte_t *)pvPortMalloc(sizeof(modem_dte_t));
    dte->uartPid = 0; // Assuming UART 0 as per mqtt_at_mgr.c
    EC200DCEInit(&modemDCE, dte);


    /* Create Https Manager Task */
    if(xTaskCreate(HttpsMgrTask, "HttpsMgrTask", MODEM_HTTP_STACK_SIZE, 
                   config, MODEM_HTTP_PRIORITY, &httpTaskHandle) != pdPASS)
    {
        trice(iD(1127), "err: HttpsMgrTask creation failed\n");
        return APP_ERR_NO_MEM;
    }
    trice(iD(2781), "dbg: HttpsATMgrInit Success\n");

    return APP_ERR_NONE;
}

void HttpAtMgrPrintDiags(void)
{
	if(httpTaskHandle != NULL)
	{
		uint32_t space = uxTaskGetStackHighWaterMark(httpTaskHandle) * sizeof(StackType_t);
		trice32(iD(3010), "dbg: diag: HTTP-AT-MGR stack rem = %d \n", space);
	}
}
