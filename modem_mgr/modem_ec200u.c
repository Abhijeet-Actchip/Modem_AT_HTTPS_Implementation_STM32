/**
 * @file modem_ec200u.c
 * @author Mahesh Murty (mahesh@actchip.com)
 * @brief Quectel EC200U specific DCE implementation.
 * @date 2024-06-08
 * 
 * @copyright Copyright (c) Actchip Pvt. Ltd. 2026
 * 
 */

#include "modem_ec200u.h"
#include "config/appconfig.h"
#include <string.h>
#include <stdio.h>

#define STATIC static

STATIC int8_t default_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;

    if(strncmp(resp, "OK", 2) == 0)
    {
        retVal = APP_ERR_NONE;
    }
    
    return retVal;
}

STATIC int8_t qmtopen_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    /* Modem returns 0 for success, hence assigning a
     * different value. */
    uint16_t temp = -1;

    if(strncmp(resp, "+QMTOPEN", 8) == 0)
    {
        /* Convert and store result. */
        sscanf(resp, "%*s%*d,%hd", &temp);
        
        /* If received success response. */
        if(temp == 0)
        {
            retVal = APP_ERR_NONE;
        }
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    
    return retVal;
}

STATIC int8_t qmtconn_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    /* Modem returns 0 for success, hence assigning a
     * different value. */
    uint16_t temp1 = -1, temp2 = -1;

    if(strncmp(resp, "+QMTCONN", 8) == 0)
    {
        /* Convert and store result. */
        sscanf(resp, "%*s%*d,%hd,%hd", &temp1, &temp2);
        
        /* If received success response. */
        if((temp1 == 0) && (temp2 == 0))
        {
            retVal = APP_ERR_NONE;
        }
    }
    else if(strncmp(resp, "+QMTSTAT", 8) == 0)
    {
        /* Ignore this response.
         * Indicates we are still processing responses for this command.
         */
        retVal = APP_ERR_PROC_RESP;
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    
    return retVal;
}

STATIC int8_t qmtsub_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    /* Modem returns 0 for success, hence assigning a
     * different value. */
    uint16_t temp = -1;

    if(strncmp(resp, "+QMTSUB", 7) == 0)
    {
        /* Convert and store result. */
        sscanf(resp, "%*s%*d,%*d,%hd", &temp);
        
        /* If received success response. */
        if(temp == 0)
        {
            retVal = APP_ERR_NONE;
        }
        /* Retransmitting packet, so wait for response. */
        else if(temp == 1)
        {
            retVal = APP_ERR_PROC_RESP;
        }
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    
    return retVal;
}

STATIC int8_t qmtuns_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    /* Modem returns 0 for success, hence assigning a
     * different value. */
    uint16_t temp = -1;

    if(strncmp(resp, "+QMTUNS", 7) == 0)
    {
        /* Convert and store result. */
        sscanf(resp, "%*s%*d,%*d,%hd", &temp);
        
        /* If received success response. */
        if(temp == 0)
        {
            retVal = APP_ERR_NONE;
        }
        /* Retransmitting packet, so wait for response. */
        else if(temp == 1)
        {
            retVal = APP_ERR_PROC_RESP;
        }
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    
    return retVal;
}

STATIC int8_t qmtpubex_resp2_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    /* Modem returns 0 for success, hence assigning a
     * different value. */
    uint16_t temp = -1;

    if(strncmp(resp, "+QMTPUBEX", 9) == 0)
    {
        /* Convert and store result. */
        sscanf(resp, "%*s%*d,%*d,%hd", &temp);
        
        /* If received success response. */
        if(temp == 0)
        {
            retVal = APP_ERR_NONE;
        }
        /* Retransmitting packet, so wait for response. */
        else if(temp == 1)
        {
            retVal = APP_ERR_PROC_RESP;
        }
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    
    return retVal;
}

STATIC int8_t qmtdisc_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    /* Modem returns 0 for success, hence assigning a
     * different value. */
    int16_t temp = 100;

    if(strncmp(resp, "+QMTDISC", 8) == 0)
    {
        /* Convert and store result. */
        sscanf(resp, "%*s%*d,%hd", &temp);
        
        /* If received success response. */
        if(temp == 0)
        {
            retVal = APP_ERR_NONE;
        }
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    
    return retVal;
}

STATIC int8_t qmtrecv_read_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    uint8_t temp1 = -1, temp2 = 0, temp3 = 0, temp4 = 0, temp5 = 0, temp6 = 0;
    ec200_mqtt_rx_state_t *rxState = NULL;

    if(strncmp(resp, "+QMTRECV", 8) == 0)
    {
        if(args != NULL)
        {
            rxState = (ec200_mqtt_rx_state_t *)args;

            /* Convert and store result. */
            sscanf(resp, "%*s%c,%c,%c,%c,%c,%c", &temp1,
            &temp2, &temp3, &temp4, &temp5, &temp6);
            if(temp1 <= EC200_MQTT_CTX_ID_MAX)
            {
                rxState->mqttCtxId = temp1;
                rxState->rxState = 0;
                if(temp2)
                {
                    rxState->rxState |= (1<<0);
                }

                if(temp3)
                {
                    rxState->rxState |= (1<<1);
                }

                if(temp4)
                {
                    rxState->rxState |= (1<<2);
                }

                if(temp5)
                {
                    rxState->rxState |= (1<<3);
                }

                if(temp6)
                {
                    rxState->rxState |= (1<<4);
                }
            }
        }
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        retVal = APP_ERR_NONE;
    }
    
    return retVal;
}

STATIC int8_t qfupl_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;

    if(strncmp(resp, "CONNECT", 7) == 0)
    {
        retVal = APP_ERR_NONE;
    }
    
    return retVal;
}

STATIC int8_t qfupl_data_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;

    if(strncmp(resp, "+QFUPL", 6) == 0)
    {
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        retVal = APP_ERR_NONE;
    }
    
    return retVal;
}

int8_t EC200DCEInit(ec200_dce_t *dce, modem_dte_t *dte)
{
    int8_t retVal;

    retVal = ModemDCEInit(&dce->super, dte);

    return retVal;
}

int8_t EC200DCEPDPCtxConfig(ec200_dce_t *dce, uint8_t ctxId,
                            ec200_pdp_ctx_type_t ctxType,
                            const char * const apn)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[48];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = default_resp_handler
    };

    if((dce == NULL) || (apn == NULL) || (strlen(apn) > 20))
    {
        return retVal;
    }

    if((ctxId < EC200_PDP_CTX_ID_MIN) || (ctxId > EC200_PDP_CTX_ID_MAX))
    {
        return retVal;
    }

    if((ctxType < EC200_PDPCTX_IPV4) || (ctxType > EC200_PDPCTX_IPV4V6))
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr), "AT+QICSGP=%d,%d,\"%s\"\r",
            ctxId, ctxType, apn);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;   
}

int8_t EC200DCEPDPCtxActivate(ec200_dce_t *dce, uint8_t ctxId)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[16];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = default_resp_handler
    };

    if(dce == NULL)
    {
        return retVal;
    }

    if((ctxId < EC200_PDP_CTX_ID_MIN) || (ctxId > EC200_PDP_CTX_ID_MAX))
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr), "AT+QIACT=%d\r", ctxId);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QIACT);

    return retVal;
}

int8_t EC200DCEPDPCtxDeActivate(ec200_dce_t *dce, uint8_t ctxId)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[16];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = default_resp_handler
    };

    if(dce == NULL)
    {
        return retVal;
    }

    if((ctxId < EC200_PDP_CTX_ID_MIN) || (ctxId > EC200_PDP_CTX_ID_MAX))
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr), "AT+QIDEACT=%d\r", ctxId);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QIDEACT);

    return retVal;    
}

static int8_t SSLSetCertPath(ec200_dce_t *dce, uint8_t sslCtxId, const char * const arg, const char * const path)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[64];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = default_resp_handler
    };

    if((dce == NULL)|| (arg == NULL) || (path == NULL))
    {
        return retVal;
    }

    if(sslCtxId > EC200_SSL_CTX_ID_MAX)
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr),
            "AT+QSSLCFG=\"%s\",%d,\"%s\"\r",
            arg, sslCtxId, path);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t EC200DCESetCACertPath(ec200_dce_t *dce, uint8_t sslCtxId, const char * const path)
{
    return SSLSetCertPath(dce, sslCtxId, "cacert", path);
}

int8_t EC200DCESetClientCertPath(ec200_dce_t *dce, uint8_t sslCtxId, const char * const path)
{
    return SSLSetCertPath(dce, sslCtxId, "clientcert", path);
}

int8_t EC200DCESetClientKeyPath(ec200_dce_t *dce, uint8_t sslCtxId, const char * const path)
{
    return SSLSetCertPath(dce, sslCtxId, "clientkey", path);
}

int8_t EC200DCESetSecurityLevel(ec200_dce_t *dce, uint8_t sslCtxId, ec200_ssl_seclevel_t level)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[32];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = default_resp_handler
    };

    if(dce == NULL)
    {
        return retVal;
    }

    if(sslCtxId > EC200_SSL_CTX_ID_MAX)
    {
        return retVal;
    }

    if((level < EC200_SSL_SECLEVEL_NO_AUTH) || (level > EC200_SSL_SECLEVEL_2))
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr),
            "AT+QSSLCFG=\"seclevel\",%d,%d\r",
            sslCtxId, level);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;    
}

int8_t EC200DCESetMQTTVersion(ec200_dce_t *dce, uint8_t mqttCtxId, ec200_mqtt_version_t vsn)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[32];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = default_resp_handler
    };

    if(dce == NULL)
    {
        return retVal;
    }

    if(mqttCtxId > EC200_MQTT_CTX_ID_MAX)
    {
        return retVal;
    }

    if((vsn < EC200_MQTT_VERSION_3_1) || (vsn > EC200_MQTT_VERSION_3_1_1))
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr),
            "AT+QMTCFG=\"version\",%d,%d\r",
            mqttCtxId, vsn);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;    
}

int8_t EC200DCESetMQTTSSLEn(ec200_dce_t *dce, uint8_t mqttCtxId, uint8_t sslCtxId, uint8_t en)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[32];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = default_resp_handler
    };

    if(dce == NULL)
    {
        return retVal;
    }

    if(mqttCtxId > EC200_MQTT_CTX_ID_MAX)
    {
        return retVal;
    }

    if(sslCtxId > EC200_SSL_CTX_ID_MAX)
    {
        return retVal;
    }

    /* Form command. */
    if(en)
    {
        snprintf(cmdStr, sizeof(cmdStr),
            "AT+QMTCFG=\"ssl\",%d,1,%d\r",
            mqttCtxId, sslCtxId);
    }
    else
    {
        snprintf(cmdStr, sizeof(cmdStr),
            "AT+QMTCFG=\"ssl\",%d,0\r",
            mqttCtxId);
    }

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;    
}

int8_t EC200DCESetMQTTCleanSessEn(ec200_dce_t *dce, uint8_t mqttCtxId, uint8_t en)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[32];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = default_resp_handler
    };

    if(dce == NULL)
    {
        return retVal;
    }

    if(mqttCtxId > EC200_MQTT_CTX_ID_MAX)
    {
        return retVal;
    }

    if(en)
    {
        en = 1;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr),
            "AT+QMTCFG=\"session\",%d,%d\r",
            mqttCtxId, en);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;    
}

int8_t EC200DCESetMQTTRecvURCLenEn(ec200_dce_t *dce, uint8_t mqttCtxId, uint8_t en)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[32];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = default_resp_handler
    };

    if(dce == NULL)
    {
        return retVal;
    }

    if(mqttCtxId > EC200_MQTT_CTX_ID_MAX)
    {
        return retVal;
    }

    if(en)
    {
        en = 1;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr),
            "AT+QMTCFG=\"recv/mode\",%d,0,%d\r",
            mqttCtxId, en);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t EC200DCEMQTTOpen(ec200_dce_t *dce, uint8_t mqttCtxId,
                        const char * const broker, uint16_t port)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[128];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = qmtopen_resp_handler
    };

    if((dce == NULL) || (broker == NULL) || (!port))
    {
        return retVal;
    }

    if(mqttCtxId > EC200_MQTT_CTX_ID_MAX)
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr),
            "AT+QMTOPEN=%d,\"%s\",%d\r",
            mqttCtxId, broker, port);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QMTOPEN);

    return retVal;
}

int8_t EC200DCEMQTTConnect(ec200_dce_t *dce, uint8_t mqttCtxId,
                        const char * const clientId, const char * const uname,
                        const char * const pass)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[128];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = qmtconn_resp_handler
    };

    if((dce == NULL) || (clientId == NULL))
    {
        return retVal;
    }

    if(mqttCtxId > EC200_MQTT_CTX_ID_MAX)
    {
        return retVal;
    }

    /* Form command. */
    if((uname != NULL) && (pass != NULL))
    {
        snprintf(cmdStr, sizeof(cmdStr),
            "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"\r",
            mqttCtxId, clientId, uname, pass);
    }
    else
    {
        snprintf(cmdStr, sizeof(cmdStr),
            "AT+QMTCONN=%d,\"%s\"\r",
            mqttCtxId, clientId);
    }
    
    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QMTCONN);

    return retVal;       
}

int8_t EC200DCEMQTTSub(ec200_dce_t *dce, uint8_t mqttCtxId, uint16_t msgId,
                        const char * const topic, uint8_t qos)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[128];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = qmtsub_resp_handler
    };

    if((dce == NULL) || (topic == NULL) || (msgId == 0) || (qos > 2))
    {
        return retVal;
    }

    if(mqttCtxId > EC200_MQTT_CTX_ID_MAX)
    {
        return retVal;
    }

    if(qos > 2)
    {
        qos = 2;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr),
        "AT+QMTSUB=%d,%d,\"%s\",%d\r",
        mqttCtxId, msgId, topic, qos);
    
    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QMTSUB);

    return retVal;      
}

int8_t EC200DCEMQTTUnSub(ec200_dce_t *dce, uint8_t mqttCtxId, uint16_t msgId,
                        const char * const topic)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[128];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = qmtuns_resp_handler
    };

    if((dce == NULL) || (topic == NULL) || (msgId == 0))
    {
        return retVal;
    }

    if(mqttCtxId > EC200_MQTT_CTX_ID_MAX)
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr),
        "AT+QMTUNS=%d,%d,\"%s\"\r",
        mqttCtxId, msgId, topic);
    
    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QMTSUB);

    return retVal;
}

int8_t EC200DCEMQTTPub(ec200_dce_t *dce, ec200_mqtt_pub_pkt_t * const pkt)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[128];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .noResp = 1,
    };
    modem_data_t txData = {};

    if((dce == NULL) || (pkt == NULL))
    {
        return retVal;
    }

    if((pkt->topic == NULL) || (pkt->data == NULL) ||
       (pkt->msgId == 0) || (pkt->dataLen == 0))
    {
        return retVal;
    }

    if(pkt->mqttCtxId > EC200_MQTT_CTX_ID_MAX)
    {
        return retVal;
    }

    if(pkt->qos > 2)
    {
        pkt->qos = 2;
    }

    if(pkt->retain)
    {
        pkt->retain = 1;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr),
        "AT+QMTPUBEX=%d,%d,%d,%d,\"%s\",%d\r",
        pkt->mqttCtxId, pkt->msgId, pkt->qos,
        pkt->retain, pkt->topic, pkt->dataLen);
    
    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QMTSUB);

    if(retVal == APP_ERR_NONE)
    {
        /* Using this to ignore > .*/
        ModemDTEFlushRx(dce->super.dte, MODEM_CMD_TO_DEF);
        
        txData.data = (uint8_t *)pkt->data;
        txData.dataLen = pkt->dataLen;
        txData.handler = qmtpubex_resp2_handler;
        retVal = ModemDTESendData(dce->super.dte, &txData, EC200_CMD_TO_QMTSUB);
    }

    return retVal;
}

int8_t EC200DCEMQTTGetRxStatus(ec200_dce_t *dce, ec200_mqtt_rx_state_t *rxState)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = {
        .cmd = "AT+QMTRECV?\r",
        .args = rxState,
        .handler = qmtrecv_read_resp_handler
    };

    if((dce == NULL) || (rxState == NULL))
    {
        return retVal;
    }

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;   
}

int8_t EC200DCEMQTTReadRxBuff(ec200_dce_t *dce, uint8_t mqttCtxId, uint8_t rxBuffId)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[32];
    
    /* Read response to be handled by DTE Rx event. */
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .noResp = 1
    };

    if(dce == NULL)
    {
        return retVal;
    }

    if(mqttCtxId > EC200_MQTT_CTX_ID_MAX)
    {
        return retVal;
    }

    if(rxBuffId > EC200_MQTT_RX_BUFF_ID_MAX)
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr),
        "AT+QMTRECV=%d,%d\r", mqttCtxId, rxBuffId);
    
    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t EC200DCEMQTTDisConnect(ec200_dce_t *dce, uint8_t mqttCtxId)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[32];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = qmtdisc_resp_handler
    };

    if(dce == NULL)
    {
        return retVal;
    }

    if(mqttCtxId > EC200_MQTT_CTX_ID_MAX)
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr), "AT+QMTDISC=%d\r", mqttCtxId);
    
    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QMTDISC);

    return retVal;   
}

int8_t EC200DCEMQTTClose(ec200_dce_t *dce, uint8_t mqttCtxId)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[32];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = default_resp_handler
    };

    if(dce == NULL)
    {
        return retVal;
    }

    if(mqttCtxId > EC200_MQTT_CTX_ID_MAX)
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr), "AT+QMTCLOSE=%d\r", mqttCtxId);
    
    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QMTDISC);

    return retVal;   
}

int8_t EC200DCEListFiles(ec200_dce_t *dce)
{
    int8_t retVal = APP_ERR_INV_ARG;
    /* Listing files on UFS partition only. */
    modem_cmd_t txCmd = {
        .cmd = "AT+QFLST\r",
        .noResp = 1
    };

    if(dce == NULL)
    {
        return retVal;
    }
    
    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t EC200DCEDeleteFile(ec200_dce_t *dce, const char * const fname)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[32];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = default_resp_handler
    };

    if((dce == NULL) || (fname == NULL))
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr), "AT+QFDEL=\"%s\"\r", fname);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t EC200DCEUploadFile(ec200_dce_t *dce, ec200_file_upl_ctx_t *fileCtx)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[64];
    
    modem_cmd_t txCmd = {
        .cmd = cmdStr,
        .handler = qfupl_resp_handler
    };

    if((dce == NULL) || (fileCtx == NULL))
    {
        return retVal;
    }

    if((fileCtx->fileName == NULL) || (fileCtx->dataBuff == NULL))
    {
        return retVal;
    }

    if(fileCtx->dataLen == 0)
    {
        return retVal;
    }

    /* Form command. */
    snprintf(cmdStr, sizeof(cmdStr),
            "AT+QFUPL=\"%s\",%d\r", 
            fileCtx->fileName, fileCtx->dataLen);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
    if(retVal == APP_ERR_NONE)
    {
        #define FILE_UL_CHUNK_SIZE              256
        /* Start file upload. */
        uint16_t i = 0;
        int32_t remLen = fileCtx->dataLen;
        modem_data_t txData = { 
            .data = &fileCtx->dataBuff[i],
            .handler = qfupl_data_resp_handler,
            .noResp = 1,
            .dataLen = FILE_UL_CHUNK_SIZE
        };
        
        while((remLen > FILE_UL_CHUNK_SIZE) && (retVal == APP_ERR_NONE))
        {
            retVal = ModemDTESendData(dce->super.dte, &txData, MODEM_CMD_TO_DEF);
            if(retVal == APP_ERR_NONE)
            {
                /* Update state variables. */
                i += FILE_UL_CHUNK_SIZE;
                remLen -= FILE_UL_CHUNK_SIZE;
                txData.data = &fileCtx->dataBuff[i];
            }
            else
            {
                break;
            }
        }
        
        if(retVal == APP_ERR_NONE)
        {
            /* Single last packet to be tx. */
            txData.dataLen = remLen;
            txData.noResp = 0;
            retVal = ModemDTESendData(dce->super.dte, &txData, MODEM_CMD_TO_DEF);
        }
    }

    return retVal;
}

STATIC int8_t qhttp_read_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    if(strncmp(resp, "+QHTTPREAD", 10) == 0) {
        retVal = APP_ERR_NONE;
    } else if(strncmp(resp, "CONNECT", 7) == 0) {
        retVal = APP_ERR_PROC_RESP;
    } else if(strncmp(resp, "OK", 2) == 0) {
        retVal = APP_ERR_PROC_RESP;
    }
    return retVal;
}

STATIC int8_t qhttp_cmd_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    int err = -1;
    if(strncmp(resp, "+QHTTP", 6) == 0) {
        sscanf(resp, "%*[^:]:%d", &err);
        if(err == 0) {
            retVal = APP_ERR_NONE;
        }
    } else if(strncmp(resp, "OK", 2) == 0) {
        retVal = APP_ERR_PROC_RESP;
    }
    return retVal;
}

int8_t EC200DCEHTTPSSLEn(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t sslCtxId, uint8_t en)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[48];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = default_resp_handler };
    if(dce == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPCFG=\"sslctxid\",%d\r", sslCtxId);
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEHTTPCustomizeReqHeaderEn(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t en)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[48];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = default_resp_handler };
    if(dce == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPCFG=\"requestheader\",%d\r", en ? 1 : 0);
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEHTTPCustomizeRespHeaderEn(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t en)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[48];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = default_resp_handler };
    if(dce == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPCFG=\"responseheader\",%d\r", en ? 1 : 0);
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEHTTPSetContentType(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t content_type)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[48];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = default_resp_handler };
    if(dce == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPCFG=\"contenttype\",%d\r", content_type);
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

STATIC int8_t qhttpcfg_ctype_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    uint8_t *ctype = (uint8_t *)args;
    uint16_t val = 0;
    if (strncmp(resp, "+QHTTPCFG: \"contenttype\",", 25) == 0) {
        sscanf(resp, "%*[^,],%hd", &val);
        if (ctype) *ctype = (uint8_t)val;
        retVal = APP_ERR_PROC_RESP;
    } else if(strncmp(resp, "OK", 2) == 0) {
        retVal = APP_ERR_NONE;
    }
    return retVal;
}

int8_t EC200DCEHTTPGetContentType(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t *content_type)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = { .cmd = "AT+QHTTPCFG=\"contenttype\"\r", .handler = qhttpcfg_ctype_resp_handler, .args = content_type };
    if(dce == NULL || content_type == NULL) return retVal;
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEHTTPRespAutoOutEn(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t en)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[48];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = default_resp_handler };
    if(dce == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPCFG=\"rspout/auto\",%d\r", en ? 1 : 0);
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEHTTPClosedIndcEn(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t en)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[48];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = default_resp_handler };
    if(dce == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPCFG=\"closed/ind\",%d\r", en ? 1 : 0);
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEHTTPSetURLStr(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t *urlStr, uint16_t urlLen)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[48];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = qfupl_resp_handler };
    if(dce == NULL || urlStr == NULL || urlLen == 0) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPURL=%d,%d\r", urlLen, EC200_QHTTP_INPUT_TIMEOUT_S);
    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QHTTP_INPUT);
    if(retVal == APP_ERR_NONE) {
        modem_data_t txData = { .data = urlStr, .dataLen = urlLen, .noResp = 0, .handler = default_resp_handler };
        retVal = ModemDTESendData(dce->super.dte, &txData, 5000);
    }
    return retVal;
}

int8_t EC200DCEHTTPGetURLStr(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t *urlStr, uint16_t *urlLen)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = { .cmd = "AT+QHTTPCFG=\"url\"\r", .handler = default_resp_handler };
    if(dce == NULL) return retVal;
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEHTTSetHeaderStr(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t *hdrStr, uint16_t hdrLen)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[128];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = default_resp_handler };
    if(dce == NULL || hdrStr == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPCFG=\"header\",\"%s\"\r", hdrStr);
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEHTTGetHeaderStr(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t *hdrStr, uint16_t *hdrLen)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = { .cmd = "AT+QHTTPCFG=\"header\"\r", .handler = default_resp_handler };
    if(dce == NULL) return retVal;
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEHTTSetCredsStr(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t *credStr, uint16_t credLen)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[128];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = default_resp_handler };
    if(dce == NULL || credStr == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPCFG=\"auth\",\"%s\"\r", credStr);
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEHTTGetCredsStr(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t *credStr, uint16_t *credLen)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = { .cmd = "AT+QHTTPCFG=\"auth\"\r", .handler = default_resp_handler };
    if(dce == NULL) return retVal;
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEHTTPGet(ec200_dce_t *dce, uint8_t httpCtxId, uint16_t rsptime, uint8_t *data, uint16_t dataLen)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[64];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = qhttp_cmd_resp_handler };
    if(dce == NULL) return retVal;

    if(data != NULL && dataLen > 0) {
        txCmd.handler = qfupl_resp_handler;
        snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPGET=%d,%d\r", rsptime, dataLen);
        retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QHTTP_INPUT);
        if(retVal == APP_ERR_NONE) {
            modem_data_t txData = { .data = data, .dataLen = dataLen, .noResp = 0, .handler = qhttp_cmd_resp_handler };
            retVal = ModemDTESendData(dce->super.dte, &txData, rsptime * 1000);
        }
    } else {
        snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPGET=%d\r", rsptime);
        retVal = ModemDTESendWait(dce->super.dte, &txCmd, rsptime * 1000);
    }
    return retVal;
}

int8_t EC200DCEHTTPPost(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t *data, uint16_t dataLen, uint16_t rsptime)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[64];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = qfupl_resp_handler };
    if(dce == NULL || data == NULL || dataLen == 0) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPPOST=%d,%d,%d\r", dataLen, EC200_QHTTP_INPUT_TIMEOUT_S, rsptime);
    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QHTTP_INPUT);
    if(retVal == APP_ERR_NONE) {
        modem_data_t txData = { .data = data, .dataLen = dataLen, .noResp = 0, .handler = qhttp_cmd_resp_handler };
        retVal = ModemDTESendData(dce->super.dte, &txData, rsptime * 1000);
    }
    return retVal;
}

int8_t EC200DCEHTTPPut(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t *data, uint16_t dataLen, uint16_t rsptime)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[64];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = qfupl_resp_handler };
    if(dce == NULL || data == NULL || dataLen == 0) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPPUT=%d,%d,%d\r", dataLen, EC200_QHTTP_INPUT_TIMEOUT_S, rsptime);
    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QHTTP_INPUT);
    if(retVal == APP_ERR_NONE) {
        modem_data_t txData = { .data = data, .dataLen = dataLen, .noResp = 0, .handler = qhttp_cmd_resp_handler };
        retVal = ModemDTESendData(dce->super.dte, &txData, rsptime * 1000);
    }
    return retVal;
}

int8_t EC200DCEHTTPRead(ec200_dce_t *dce, uint8_t httpCtxId, uint16_t waitTime)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[48];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = qhttp_read_resp_handler };
    if(dce == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QHTTPREAD=%d\r", waitTime);
    retVal = ModemDTESendWait(dce->super.dte, &txCmd, waitTime * 1000);
    return retVal;
}

STATIC int8_t qesim_cmd_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    int result = -1;
    if(strncmp(resp, "+QESIM:", 7) == 0) {
        char cmd_name[32] = {0};
        if (sscanf(resp, "+QESIM: \"%[^\"]\",%d", cmd_name, &result) == 2) {
            if(result == 0) {
                retVal = APP_ERR_PROC_RESP;
            } else {
                retVal = APP_ERR_INV_RESP;
            }
        }
    } else if(strncmp(resp, "OK", 2) == 0) {
        retVal = APP_ERR_NONE;
    }
    return retVal;
}

STATIC int8_t qesim_eid_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    int result = -1;
    char *eid_out = (char *)args;
    
    if(strncmp(resp, "+QESIM:", 7) == 0) {
        char cmd_name[32] = {0};
        char eid[64] = {0};
        int parsed = sscanf(resp, "+QESIM: \"%[^\"]\",%d,\"%[^\"]\"", cmd_name, &result, eid);
        if(result == 0) {
            if(eid_out && parsed == 3) {
                strcpy(eid_out, eid);
            }
            retVal = APP_ERR_PROC_RESP;
        }
    } else if(strncmp(resp, "OK", 2) == 0) {
        retVal = APP_ERR_NONE;
    }
    return retVal;
}

int8_t EC200DCEeSIMGetEID(ec200_dce_t *dce, char *eid, uint16_t maxLen)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = { .cmd = "AT+QESIM=\"eid\"\r", .handler = qesim_eid_resp_handler, .args = eid };
    if(dce == NULL || eid == NULL) return retVal;
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEeSIMListProfiles(ec200_dce_t *dce, uint8_t mode)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[48];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = qesim_cmd_resp_handler };
    if(dce == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QESIM=\"list\",%d\r", mode);
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEeSIMEnableProfile(ec200_dce_t *dce, const char *iccid)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[64];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = qesim_cmd_resp_handler };
    if(dce == NULL || iccid == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QESIM=\"enable\",\"%s\"\r", iccid);
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEeSIMDisableProfile(ec200_dce_t *dce, const char *iccid)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[64];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = qesim_cmd_resp_handler };
    if(dce == NULL || iccid == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QESIM=\"disable\",\"%s\"\r", iccid);
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEeSIMDeleteProfile(ec200_dce_t *dce, const char *iccid)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[64];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = qesim_cmd_resp_handler };
    if(dce == NULL || iccid == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QESIM=\"delete\",\"%s\"\r", iccid);
    return ModemDTESendWait(dce->super.dte, &txCmd, 60000); // 60s max response time
}

int8_t EC200DCEeSIMDownloadProfile(ec200_dce_t *dce, uint8_t nwMode, const char *activationCode, const char *confirmationCode)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[256];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = default_resp_handler };
    if(dce == NULL || activationCode == NULL) return retVal;
    
    if(confirmationCode != NULL && strlen(confirmationCode) > 0) {
        snprintf(cmdStr, sizeof(cmdStr), "AT+QESIM=\"download\",%d,\"%s\",\"%s\"\r", nwMode, activationCode, confirmationCode);
    } else {
        snprintf(cmdStr, sizeof(cmdStr), "AT+QESIM=\"download\",%d,\"%s\"\r", nwMode, activationCode);
    }
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}

int8_t EC200DCEeSIMSetProfileNickname(ec200_dce_t *dce, const char *iccid, const char *nickname)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char cmdStr[128];
    modem_cmd_t txCmd = { .cmd = cmdStr, .handler = qesim_cmd_resp_handler };
    if(dce == NULL || iccid == NULL || nickname == NULL) return retVal;
    snprintf(cmdStr, sizeof(cmdStr), "AT+QESIM=\"nickname\",\"%s\",\"%s\"\r", nickname, iccid);
    return ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
}
