/**
 * @file modem_ec200u.h
 * @author Mahesh Murty (mahesh@actchip.com)
 * @brief Quectel EC200U specific DCE implementation.
 * @date 2024-06-08
 *
 * @copyright Copyright (c) Actchip Pvt. Ltd. 2026
 *
 */

#ifndef __MODEM_EC200U_H__
#define __MODEM_EC200U_H__ 1

#include "modem_dce.h"

#define EC200_PDP_CTX_ID_MIN 1
#define EC200_PDP_CTX_ID_MAX 7

#define EC200_SSL_CTX_ID_MIN 0
#define EC200_SSL_CTX_ID_MAX 5

#define EC200_MQTT_CTX_ID_MIN 0
#define EC200_MQTT_CTX_ID_MAX 5

#define EC200_MQTT_RX_BUFF_ID_MIN 0
#define EC200_MQTT_RX_BUFF_ID_MAX 4

#define EC200_CMD_TO_QIACT (150000)  /*!< Timeout value for QIACT command */
#define EC200_CMD_TO_QIDEACT (40000) /*!< Timeout value for QIDEACT command */
#define EC200_CMD_TO_QMTOPEN                                                   \
  (120000)                           /*!< Timeout value for QMTOPEN command    \
                                      */
#define EC200_CMD_TO_QMTCONN (5000)  /*!< Timeout value for QMTCONN command */
#define EC200_CMD_TO_QMTDISC (30000) /*!< Timeout value for QMTDISC command */
#define EC200_CMD_TO_QMTSUB (15000)  /*!< Timeout value for QMTSUB command */

#define EC200_QHTTP_INPUT_TIMEOUT_S                                            \
  (80) /*!< HTTP input timeout in seconds (wait for CONNECT) */
#define EC200_CMD_TO_QHTTP_INPUT                                               \
  (EC200_QHTTP_INPUT_TIMEOUT_S *                                               \
   1000) /*!< Timeout value for QHTTP input (ms) */

typedef struct _ec200_dce_t {
  modem_dce_t super;
} ec200_dce_t;

typedef enum _ec200_pdp_ctx_type_t {
  EC200_PDPCTX_IPV4 = 1,
  EC200_PDPCTX_IPV6,
  EC200_PDPCTX_IPV4V6
} ec200_pdp_ctx_type_t;

typedef enum _ec200_ssl_seclevel_t {
  EC200_SSL_SECLEVEL_NO_AUTH = 0,
  EC200_SSL_SECLEVEL_1, /*!< Performs server authentication. */
  EC200_SSL_SECLEVEL_2  /*!< Perform server and client authentication if
                           requested by the remote server  */
} ec200_ssl_seclevel_t;

typedef enum _ec200_mqtt_version_t {
  EC200_MQTT_VERSION_3_1 = 3,
  EC200_MQTT_VERSION_3_1_1,
} ec200_mqtt_version_t;

typedef struct _ec200_mqtt_pub_pkt_t {
  uint8_t mqttCtxId;
  uint16_t msgId;
  char *topic, *data;
  uint16_t dataLen;
  uint8_t qos, retain;
} ec200_mqtt_pub_pkt_t;

typedef struct _ec200_mqtt_rx_state_t {
  uint8_t mqttCtxId;
  uint8_t rxState;
} ec200_mqtt_rx_state_t;

typedef struct _ec200_file_upl_ctx_t {
  char *fileName;
  uint8_t *dataBuff;
  uint16_t dataLen;
} ec200_file_upl_ctx_t;

int8_t EC200DCEInit(ec200_dce_t *dce, modem_dte_t *dte);
int8_t EC200DCEPDPCtxConfig(ec200_dce_t *dce, uint8_t ctxId,
                            ec200_pdp_ctx_type_t ctxType,
                            const char *const apn);
int8_t EC200DCEPDPCtxActivate(ec200_dce_t *dce, uint8_t ctxId);
int8_t EC200DCEPDPCtxDeActivate(ec200_dce_t *dce, uint8_t ctxId);
int8_t EC200DCESetCACertPath(ec200_dce_t *dce, uint8_t sslCtxId,
                             const char *const path);
int8_t EC200DCESetClientCertPath(ec200_dce_t *dce, uint8_t sslCtxId,
                                 const char *const path);
int8_t EC200DCESetClientKeyPath(ec200_dce_t *dce, uint8_t sslCtxId,
                                const char *const path);
int8_t EC200DCESetSecurityLevel(ec200_dce_t *dce, uint8_t sslCtxId,
                                ec200_ssl_seclevel_t level);
int8_t EC200DCESetMQTTVersion(ec200_dce_t *dce, uint8_t mqttCtxId,
                              ec200_mqtt_version_t vsn);
int8_t EC200DCESetMQTTSSLEn(ec200_dce_t *dce, uint8_t mqttCtxId,
                            uint8_t sslCtxId, uint8_t en);
int8_t EC200DCESetMQTTCleanSessEn(ec200_dce_t *dce, uint8_t mqttCtxId,
                                  uint8_t en);
int8_t EC200DCESetMQTTRecvURCLenEn(ec200_dce_t *dce, uint8_t mqttCtxId,
                                   uint8_t en);
int8_t EC200DCEMQTTOpen(ec200_dce_t *dce, uint8_t mqttCtxId,
                        const char *const broker, uint16_t port);
int8_t EC200DCEMQTTConnect(ec200_dce_t *dce, uint8_t mqttCtxId,
                           const char *const clientId, const char *const uname,
                           const char *const pass);
int8_t EC200DCEMQTTSub(ec200_dce_t *dce, uint8_t mqttCtxId, uint16_t msgId,
                       const char *const topic, uint8_t qos);
int8_t EC200DCEMQTTUnSub(ec200_dce_t *dce, uint8_t mqttCtxId, uint16_t msgId,
                         const char *const topic);
int8_t EC200DCEMQTTPub(ec200_dce_t *dce, ec200_mqtt_pub_pkt_t *const pkt);
int8_t EC200DCEMQTTGetRxStatus(ec200_dce_t *dce,
                               ec200_mqtt_rx_state_t *rxState);
int8_t EC200DCEMQTTReadRxBuff(ec200_dce_t *dce, uint8_t mqttCtxId,
                              uint8_t rxBuffId);
int8_t EC200DCEMQTTDisConnect(ec200_dce_t *dce, uint8_t mqttCtxId);
int8_t EC200DCEMQTTClose(ec200_dce_t *dce, uint8_t mqttCtxId);
int8_t EC200DCEListFiles(ec200_dce_t *dce);
int8_t EC200DCEDeleteFile(ec200_dce_t *dce, const char *const fname);
int8_t EC200DCEUploadFile(ec200_dce_t *dce, ec200_file_upl_ctx_t *fileCtx);

/* TODO : Implement HTTPS implementation for EC200 */
int8_t EC200DCEHTTPSSLEn(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t sslCtxId,
                         uint8_t en);
int8_t EC200DCEHTTPCustomizeReqHeaderEn(ec200_dce_t *dce, uint8_t httpCtxId,
                                        uint8_t en);
int8_t EC200DCEHTTPCustomizeRespHeaderEn(ec200_dce_t *dce, uint8_t httpCtxId,
                                         uint8_t en);
int8_t EC200DCEHTTPSetContentType(ec200_dce_t *dce, uint8_t httpCtxId,
                                  uint8_t content_type);
int8_t EC200DCEHTTPGetContentType(ec200_dce_t *dce, uint8_t httpCtxId,
                                  uint8_t *content_type);
int8_t EC200DCEHTTPRespAutoOutEn(ec200_dce_t *dce, uint8_t httpCtxId,
                                 uint8_t en);
int8_t EC200DCEHTTPClosedIndcEn(ec200_dce_t *dce, uint8_t httpCtxId,
                                uint8_t en);
int8_t EC200DCEHTTPSetURLStr(ec200_dce_t *dce, uint8_t httpCtxId,
                             uint8_t *urlStr, uint16_t urlLen);
int8_t EC200DCEHTTPGetURLStr(ec200_dce_t *dce, uint8_t httpCtxId,
                             uint8_t *urlStr, uint16_t *urlLen);
int8_t EC200DCEHTTSetHeaderStr(ec200_dce_t *dce, uint8_t httpCtxId,
                               uint8_t *hdrStr, uint16_t hdrLen);
int8_t EC200DCEHTTGetHeaderStr(ec200_dce_t *dce, uint8_t httpCtxId,
                               uint8_t *hdrStr, uint16_t *hdrLen);
int8_t EC200DCEHTTSetCredsStr(ec200_dce_t *dce, uint8_t httpCtxId,
                              uint8_t *credStr, uint16_t credLen);
int8_t EC200DCEHTTGetCredsStr(ec200_dce_t *dce, uint8_t httpCtxId,
                              uint8_t *credStr, uint16_t *credLen);
int8_t EC200DCEHTTPGet(ec200_dce_t *dce, uint8_t httpCtxId, uint16_t rsptime,
                       uint8_t *data, uint16_t dataLen);
int8_t EC200DCEHTTPPost(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t *data,
                        uint16_t dataLen, uint16_t rsptime);
int8_t EC200DCEHTTPPut(ec200_dce_t *dce, uint8_t httpCtxId, uint8_t *data,
                       uint16_t dataLen, uint16_t rsptime);
int8_t EC200DCEHTTPRead(ec200_dce_t *dce, uint8_t httpCtxId, uint16_t waitTime);

/** eSIM related AT command APIs */
int8_t EC200DCEeSIMGetEID(ec200_dce_t *dce, char *eid, uint16_t maxLen);
int8_t EC200DCEeSIMListProfiles(ec200_dce_t *dce, uint8_t mode);
int8_t EC200DCEeSIMEnableProfile(ec200_dce_t *dce, const char *iccid);
int8_t EC200DCEeSIMDisableProfile(ec200_dce_t *dce, const char *iccid);
int8_t EC200DCEeSIMDeleteProfile(ec200_dce_t *dce, const char *iccid);
int8_t EC200DCEeSIMDownloadProfile(ec200_dce_t *dce, uint8_t nwMode,
                                   const char *activationCode,
                                   const char *confirmationCode);
int8_t EC200DCEeSIMSetProfileNickname(ec200_dce_t *dce, const char *iccid,
                                      const char *nickname);

/** GNSS related AT commands – see modem_ec200u_gnss.h */
#include "modem_ec200u_gnss.h"

#endif /* __MODEM_EC200U_H__ */
