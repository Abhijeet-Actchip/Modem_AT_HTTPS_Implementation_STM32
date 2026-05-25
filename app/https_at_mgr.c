#include "https_at_mgr.h"
#include "FreeRTOS.h"
#include "app_hal/app_hal_delay.h"
#include "bsp/board.h"
#include "https_common.h"
#include "task.h"
#include "tls_cert.h"
#include "utils/trice/trice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "modem_mgr/modem_ec200u.h"
#include "modem_mgr/modem_ec200u_gnss.h"

/* ============================================================================
 * EC200 Direct API Usage
 * ============================================================================
 */

#define TAG "HTTPS-AT-MGR"

#define PDP_CTX_IN_USE 1
#define SSL_CTX_IN_USE 0
#define HTTP_CTX_IN_USE 0
#define NETWORK_REG_DELAY_SEC 120
#define CA_CERT_PATH "cacert.crt"

typedef enum _nw_mgr_states_t {
  NW_MGR_STATE_RESET = 0,
  NW_MGR_STATE_CHECK_SYNC,
  NW_MGR_STATE_ERROR,
  NW_MGR_STATE_SYNC_SUCCESS,
  NW_MGR_STATE_WAIT_NW_REG,
  NW_MGR_STATE_WAIT_DATA_REG,
  NW_MGR_STATE_CONNECT
} nw_mgr_states_t;

typedef enum _https_mgr_states_t {
  HTTPS_MGR_STATE_ACT_PDP_CTX = 0,
  HTTPS_MGR_STATE_UL_CERTS,
  HTTPS_MGR_STATE_CONF_SSL,
  HTTPS_MGR_STATE_CONF_HTTP,
  HTTPS_MGR_STATE_IDLE,
  HTTPS_MGR_STATE_SEND_REQ,
  HTTPS_MGR_STATE_WAIT_RESP,
  HTTPS_MGR_STATE_READ_RESP

} https_mgr_states_t;

typedef enum _gps_mgr_states_t {
  GPS_MGR_STATE_CFG = 0,  /*!< Configure GNSS parameters (AT+QGPSCFG) */
  GPS_MGR_STATE_ON,       /*!< Turn on GNSS engine      (AT+QGPS=1)   */
  GPS_MGR_STATE_WAIT_FIX, /*!< Poll for position fix    (AT+QGPSLOC)  */
  GPS_MGR_STATE_OFF,      /*!< Turn off GNSS engine     (AT+QGPSEND)  */
  GPS_MGR_STATE_DONE,     /*!< Location acquired, hand off to HTTPS   */
  GPS_MGR_STATE_ERROR     /*!< Fatal GPS error – skip to HTTPS anyway */
} gps_mgr_states_t;

static ec200_dce_t modemDCE;
static nw_mgr_states_t nwState = NW_MGR_STATE_CHECK_SYNC;
static https_mgr_states_t httpsState = HTTPS_MGR_STATE_ACT_PDP_CTX;
static gps_mgr_states_t gpsState = GPS_MGR_STATE_CFG;
static uint32_t sslCertUlFlag = 0;

/* GPS fix retry: one attempt per second, ~70 s window. */
#define GPS_FIX_RETRY_MAX 70
static int16_t gpsFixRetryCtr = GPS_FIX_RETRY_MAX;

/* Stores the last acquired GNSS location. */
static ec200_gnss_location_t gpsLocation;

static TaskHandle_t httpTaskHandle = NULL;

static void PowerOffModem(void) {
  /* We are doing this using MODEM_PWR_KEY */
  Board_CtrlModemPwrKey(1);
  APPHAL_Delay(3500);
  Board_CtrlModemPwrKey(0);

  APPHAL_Delay(10000);
  trice(iD(1703), "dbg: Power OFF logic done.\n");
}

static void PowerOnModem(void) {
  /* We are doing this using MODEM_PWR_KEY */
  Board_CtrlModemPwrKey(1);
  APPHAL_Delay(2500);
  Board_CtrlModemPwrKey(0);

  APPHAL_Delay(10000);
  trice(iD(2805), "dbg: Power ON logic done.\n");
}

void ResetModem(void) {
  PowerOffModem();
  APPHAL_Delay(pdMS_TO_TICKS(1000));
  PowerOnModem();
  trice(iD(2125), "dbg: MODEM Reset logic done.\n");
}

static int8_t SSLCertsUl(void) {
  int8_t retVal;
  retVal = EC200DCEDeleteFile(&modemDCE, CA_CERT_PATH);
  retVal = APP_ERR_NONE;
  /* For now, we assume CA cert is available or hardcoded.
   * In a real app, this would come from NVS or flash.
   */
  // const char *test_ca_cert = "..."; // User provided this in XML
  // For now we skip actual upload if not available, or use a placeholder.

  extern const char *cacert;
  uint16_t certLen = strlen(cacert);

  ec200_file_upl_ctx_t fUl = {.dataBuff = (uint8_t *)cacert,
                              .dataLen = certLen,
                              .fileName = CA_CERT_PATH};

  trice(iD(5775), "dbg: Uploading certificate\n");
  retVal = EC200DCEUploadFile(&modemDCE, &fUl);
  if (retVal != APP_ERR_NONE) {
    trice32(iD(5564), "err: Failed uploading cert, %d\n", retVal);
  } else {
    trice(iD(5004), "dbg: Upload success\n");
  }

  /* Placeholder for actual file upload logic if needed */
  return retVal;
}

static void NwMgmtStateMc(void) {
  int8_t retVal;
  static int16_t delayCtr;
  uint8_t status;

  switch (nwState) {
  case NW_MGR_STATE_RESET:
    trice(iD(6263), "dbg: NW State RESET\n");
    ResetModem();
    APPHAL_Delay(5000);
    nwState = NW_MGR_STATE_CHECK_SYNC;
    break;

  case NW_MGR_STATE_CHECK_SYNC:
    retVal = ModemDCESync((modem_dce_t *)&modemDCE);
    if (retVal == APP_ERR_NONE) {
      trice(iD(4262), "dbg: Sync Success\n");
      retVal = ModemDCEEcho((modem_dce_t *)&modemDCE, 0);
      if (retVal == APP_ERR_NONE) {
        nwState = NW_MGR_STATE_SYNC_SUCCESS;
        trice(iD(1262), "dbg: Echo Success\n");
      } else {
        nwState = NW_MGR_STATE_ERROR;
        trice32(iD(5439), "dbg: Echo Failed, %d\n", retVal);
      }
    } else {
      trice32(iD(6093), "dbg: Sync Failed, %d", retVal);
      nwState = NW_MGR_STATE_ERROR;
    }
    break;

  case NW_MGR_STATE_SYNC_SUCCESS:
    APPHAL_Delay(2000);
    /* Hardcoded APN for now, should come from config */
    retVal =
        EC200DCEPDPCtxConfig(&modemDCE, PDP_CTX_IN_USE, 1, "airtelgprs.com");
    if (retVal == APP_ERR_NONE) {
      trice(iD(4744), "dbg: PDP Ctx Config Success\n");
      nwState = NW_MGR_STATE_WAIT_NW_REG;
      delayCtr = NETWORK_REG_DELAY_SEC;
    } else {
      trice32(iD(3335), "dbg: PDP Ctx Config Failed, %d\n", retVal);
      nwState = NW_MGR_STATE_ERROR;
    }
    break;

  case NW_MGR_STATE_WAIT_NW_REG:
    APPHAL_Delay(1000);
    status = -1;
    retVal = ModemDCEGetNwStatus((modem_dce_t *)&modemDCE, &status);
    if (retVal == APP_ERR_NONE) {
      if ((status == 1) || (status == 5)) {
        trice8(iD(4358), "dbg: NW Registered, status %d\n", status);
        nwState = NW_MGR_STATE_WAIT_DATA_REG;
        delayCtr = NETWORK_REG_DELAY_SEC;
        break;
      }
    }
    delayCtr--;
    if (delayCtr <= 0) {
      trice(iD(7482), "err: NW Reg Timeout\n");
      nwState = NW_MGR_STATE_ERROR;
    }
    break;

  case NW_MGR_STATE_WAIT_DATA_REG:
    APPHAL_Delay(1000);
    status = -1;
    retVal = ModemDCEGetGPRSStatus((modem_dce_t *)&modemDCE, &status);
    if (retVal == APP_ERR_NONE) {
      if ((status == 1) || (status == 5)) {
        trice8(iD(1603), "dbg: Data Registered, status %d\n", status);
        nwState = NW_MGR_STATE_CONNECT;
        httpsState = HTTPS_MGR_STATE_ACT_PDP_CTX;
        break;
      }
    }
    delayCtr--;
    if (delayCtr <= 0) {
      trice(iD(5942), "err: Data Reg Timeout\n");
      nwState = NW_MGR_STATE_ERROR;
    }
    break;

  case NW_MGR_STATE_CONNECT:
    /* Network is up – arm the GPS state machine on first entry only.
     * After that the main loop drives GPSMgmtStateMc() directly and
     * NwMgmtStateMc() is no longer called, so this case is a no-op. */
    break;

  case NW_MGR_STATE_ERROR:
    trice(iD(1812), "err: NW Manager Error state, resetting...\n");
    APPHAL_Delay(10000);
    nwState = NW_MGR_STATE_RESET;
    httpsState = HTTPS_MGR_STATE_ACT_PDP_CTX;
    gpsState = GPS_MGR_STATE_CFG;
    gpsFixRetryCtr = GPS_FIX_RETRY_MAX;
    break;
  }
}

/* ============================================================================
 * GPS Management State Machine
 * Runs after NwMgmtStateMc() completes (nwState == NW_MGR_STATE_CONNECT).
 * Non-blocking: each call handles one tick; the 10 ms vTaskDelay in the
 * main loop provides the timing base. GPS_FIX_RETRY_MAX iterations at
 * ~1 s / iteration give a ~70 s fix window before giving up.
 * ============================================================================
 */
static void GPSMgmtStateMc(void) {
  int8_t retVal;

  switch (gpsState) {
  /* ------------------------------------------------------------------ */
  case GPS_MGR_STATE_CFG: {
    trice(iD(3601), "dbg: GPS CFG\n");

    ec200_gnss_cfg_t gpsCfg = {
        /* No unsolicited NMEA stream on any UART – we will poll via
         * AT+QGPSLOC instead. Default output port is usbnmea but we
         * set none to keep the AT port clean. */
        .outport = EC200_GNSS_OUTPORT_NONE,
        /* Enable acquisition via AT+QGPSGNMEA (nmeasrc=1) so the
         * modem keeps updating the internal NMEA cache. */
        .nmeasrcEn = 1,
        /* Suppress continuous NMEA sentence output. */
        .nmeatype = EC200_NMEA_NONE,
        /* All constellations for best fix speed. */
        .gnssConfig = EC200_GNSS_CFG_GPS_ONLY,
    };

    retVal = EC200DCEGPSConfig(&modemDCE, &gpsCfg);
    if (retVal == APP_ERR_NONE) {
      trice(iD(3602), "dbg: GPS CFG OK\n");
      gpsState = GPS_MGR_STATE_ON;
    } else {
      trice32(iD(3603), "err: GPS CFG Failed %d\n", retVal);
      gpsState = GPS_MGR_STATE_ERROR;
    }
    break;
  }

  /* ------------------------------------------------------------------ */
  case GPS_MGR_STATE_ON:
    trice(iD(3604), "dbg: GPS ON\n");
    retVal = EC200DCEGPSTurnOn(&modemDCE);
    if (retVal == APP_ERR_NONE) {
      trice(iD(3605), "dbg: GPS Engine ON\n");
      gpsFixRetryCtr = GPS_FIX_RETRY_MAX;
      gpsState = GPS_MGR_STATE_WAIT_FIX;
    } else {
      trice32(iD(3606), "err: GPS ON Failed %d\n", retVal);
      gpsState = GPS_MGR_STATE_ERROR;
    }
    break;

  /* ------------------------------------------------------------------ */
  case GPS_MGR_STATE_WAIT_FIX:
    /* Non-blocking poll: one QGPSLOC attempt per call.
     * The main loop calls us every ~10 ms but we add a 1 s delay
     * here so we do not hammer the modem.  Total window =
     * GPS_FIX_RETRY_MAX * 1 s = 70 s. */
    APPHAL_Delay(1000);
    retVal =
        EC200DCEGPSGetLocation(&modemDCE, EC200_GPSLOC_MODE_2, &gpsLocation);
    if (retVal == APP_ERR_NONE) {
      /* Fix acquired. */
      trice(iD(3607), "dbg: GPS Fix acquired!\n");
      trice32(iD(3608), "dbg: GPS Fix type %d, sats %d\n", gpsLocation.fixType,
              gpsLocation.numSats);
      gpsState = GPS_MGR_STATE_OFF;
    } else {
      /* APP_ERR_INV_RESP means CME 516 = not fixed yet.           */
      gpsFixRetryCtr--;
      trice8(iD(3609), "dbg: GPS No fix, retries left %d\n",
             (uint8_t)gpsFixRetryCtr);
      if (gpsFixRetryCtr <= 0) {
        trice(iD(3610), "err: GPS Fix timeout, continuing without GPS\n");
        gpsState = GPS_MGR_STATE_OFF;
      }
      /* else: stay in WAIT_FIX, will retry next call */
    }
    break;

  /* ------------------------------------------------------------------ */
  case GPS_MGR_STATE_OFF:
    trice(iD(3611), "dbg: GPS OFF\n");
    retVal = EC200DCEGPSTurnOff(&modemDCE);
    if (retVal == APP_ERR_NONE) {
      trice(iD(3612), "dbg: GPS Engine OFF\n");
    } else {
      trice32(iD(3613), "wrn: GPS OFF Failed %d (ignoring)\n", retVal);
    }
    /* Regardless of turn-off result, proceed to HTTPS. */
    gpsState = GPS_MGR_STATE_DONE;
    break;

  /* ------------------------------------------------------------------ */
  case GPS_MGR_STATE_ERROR:
    trice(iD(3614), "err: GPS State Error, skipping GPS\n");
    gpsState = GPS_MGR_STATE_DONE;
    break;

  /* ------------------------------------------------------------------ */
  case GPS_MGR_STATE_DONE:
    /* Nothing to do; main loop will switch to HttpsMgmtStateMc(). */
    break;
  }
}

static void ProcessTxQueue(httpsAgentConfig_t *config) {
  httpsDataPkt_t *dataPktPtr = NULL;

  if (HTTPSReadFromTxQ(&dataPktPtr, pdMS_TO_TICKS(100)) == APP_ERR_NONE) {
    trice(iD(2535), "dbg: Packet Rx from Q\n");
    if (dataPktPtr != NULL) {
      trice32(iD(7472), "dbg: Processing packet, len %d\n",
              dataPktPtr->dataLen);
      if (dataPktPtr->dataLen > 0) {
        int8_t retVal;
        uint16_t timeoutS = config->responseTimeMs / 1000;
        if (timeoutS == 0) {
          timeoutS = 10;
        }

        /* Dynamically set URL for every request because the modem clears it
         * post-transaction */
        retVal = EC200DCEHTTPSetURLStr(&modemDCE, HTTP_CTX_IN_USE,
                                       (uint8_t *)config->baseUrl,
                                       strlen(config->baseUrl));
        if (retVal == APP_ERR_NONE) {
#if (HTTP_METHOD_USE == HTTP_METHOD_POST)
          trice(iD(5843), "dbg: Sending HTTP POST\n");
          retVal =
              EC200DCEHTTPPost(&modemDCE, HTTP_CTX_IN_USE, dataPktPtr->dataPtr,
                               dataPktPtr->dataLen, timeoutS);
#elif (HTTP_METHOD_USE == HTTP_METHOD_GET)
          trice(iD(4355), "dbg: Sending HTTP GET\n");
          retVal =
              EC200DCEHTTPGet(&modemDCE, HTTP_CTX_IN_USE, timeoutS, NULL, 0);
#endif

          if (retVal == APP_ERR_NONE) {
            trice(iD(4944), "dbg: Request Success, reading response...\n");
            /* Request sent successfully, wait for response or read response */
            retVal = EC200DCEHTTPRead(&modemDCE, HTTP_CTX_IN_USE, timeoutS);
            if (retVal != APP_ERR_NONE) {
              trice32(iD(4268), "err: HTTP Read Failed, %d\n", retVal);
            }
          } else {
            trice32(iD(1962), "err: HTTP Request Failed, %d\n", retVal);
          }
        } else {
          trice32(iD(4780), "err: Set URL Failed, %d\n", retVal);
        }

        if (retVal != APP_ERR_NONE) {
          nwState = NW_MGR_STATE_RESET;
          httpsState = HTTPS_MGR_STATE_ACT_PDP_CTX;
        }
      }

      /* Free memory */
      vPortFree(dataPktPtr->dataPtr);
      vPortFree(dataPktPtr);
    }
  }
}

static void HttpsMgmtStateMc(httpsAgentConfig_t *config) {
  int8_t retVal;

  switch (httpsState) {
  case HTTPS_MGR_STATE_ACT_PDP_CTX:
    trice(iD(5297), "dbg: HTTPS Activating PDP Ctx\n");
    retVal = EC200DCEPDPCtxActivate(&modemDCE, PDP_CTX_IN_USE);
    if (retVal == APP_ERR_NONE) {
      trice(iD(1626), "dbg: PDP Ctx Activated\n");
      if (config->useTLS) {
        httpsState = HTTPS_MGR_STATE_UL_CERTS;
      } else {
        httpsState = HTTPS_MGR_STATE_CONF_HTTP;
      }
    } else {
      trice32(iD(1845), "err: PDP Ctx Activation Failed, %d\n", retVal);
      nwState = NW_MGR_STATE_ERROR;
    }
    break;

  case HTTPS_MGR_STATE_UL_CERTS:
    if (sslCertUlFlag) {
      trice(iD(2689), "dbg: Uploading SSL Certs\n");
      retVal = SSLCertsUl();
      if (retVal == APP_ERR_NONE) {
        trice(iD(1036), "dbg: SSL Certs Uploaded\n");
        sslCertUlFlag = 0;
        httpsState = HTTPS_MGR_STATE_CONF_SSL;
      } else {
        trice32(iD(6600), "err: SSL Certs Upload Failed, %d\n", retVal);
        nwState = NW_MGR_STATE_ERROR;
      }
    } else {
      httpsState = HTTPS_MGR_STATE_CONF_SSL;
    }
    break;

  case HTTPS_MGR_STATE_CONF_SSL:
    trice(iD(3313), "dbg: Configuring SSL\n");
    retVal = EC200DCESetCACertPath(&modemDCE, SSL_CTX_IN_USE, CA_CERT_PATH);
    if (retVal != APP_ERR_NONE) {
      trice32(iD(5906), "err: Set CA Cert Path Failed, %d\n", retVal);
      nwState = NW_MGR_STATE_ERROR;
      break;
    }

    /* xml sets seclevel to 0 for Thingspeak HTTPS sometimes, but let's follow
     * standard */
    EC200DCESetSecurityLevel(&modemDCE, SSL_CTX_IN_USE, 0);

    /* Enable SSL for HTTP */
    retVal = EC200DCEHTTPSSLEn(&modemDCE, HTTP_CTX_IN_USE, SSL_CTX_IN_USE, 1);
    if (retVal != APP_ERR_NONE) {
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
    trice32(iD(3127), "dbg: Set Content Type: %d\n", config->contentType);
    EC200DCEHTTPCustomizeReqHeaderEn(&modemDCE, HTTP_CTX_IN_USE, 0);
    EC200DCEHTTPCustomizeRespHeaderEn(&modemDCE, HTTP_CTX_IN_USE, 0);

    /* URL setting is handled dynamically before every request inside
     * ProcessTxQueue */

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

static void HttpsMgrTask(void *pvArgs) {
  httpsAgentConfig_t *config = (httpsAgentConfig_t *)pvArgs;

  sslCertUlFlag = config->useTLS;

  trice(iD(7489), "dbg: HttpsMgrTask Started\n");
  APPHAL_Delay(5000);
  PowerOnModem();
  APPHAL_Delay(1000);

  while (1) {
    if (nwState == NW_MGR_STATE_CONNECT) {
      if (gpsState != GPS_MGR_STATE_DONE) {
        /* Phase 2: acquire GPS fix before HTTP session. */
        GPSMgmtStateMc();
      } else {
        /* Phase 3: run HTTPS state machine. */
        HttpsMgmtStateMc(config);
      }
    } else {
      /* Phase 1: wait for network registration. */
      NwMgmtStateMc();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

int8_t HttpsATMgrInit(httpsAgentConfig_t *config) {
  if (config == NULL) {
    return APP_ERR_INV_ARG;
  }

  /* Initialize modem DCE */
  modem_dte_t *dte = (modem_dte_t *)pvPortMalloc(sizeof(modem_dte_t));
  dte->uartPid = 0; // Assuming UART 0 as per mqtt_at_mgr.c
  EC200DCEInit(&modemDCE, dte);

  /* Create Https Manager Task */
  if (xTaskCreate(HttpsMgrTask, "HttpsMgrTask", MODEM_HTTP_STACK_SIZE, config,
                  MODEM_HTTP_PRIORITY, &httpTaskHandle) != pdPASS) {
    trice(iD(1127), "err: HttpsMgrTask creation failed\n");
    return APP_ERR_NO_MEM;
  }
  trice(iD(2781), "dbg: HttpsATMgrInit Success\n");

  return APP_ERR_NONE;
}

void HttpAtMgrPrintDiags(void) {
  if (httpTaskHandle != NULL) {
    uint32_t space =
        uxTaskGetStackHighWaterMark(httpTaskHandle) * sizeof(StackType_t);
    trice32(iD(3010), "dbg: diag: HTTP-AT-MGR stack rem = %d \n", space);
  }
}
