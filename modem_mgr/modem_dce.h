/**
 * @file modem_dce.h
 * @author Mahesh Murty (mahesh@actchip.com)
 * @brief Modem DCE handler code.
 * @date 2024-06-08
 * 
 * @copyright Copyright (c) Actchip Pvt. Ltd. 2024
 * 
 */

#ifndef __MODEM_DCE_H__
#define __MODEM_DCE_H__             1

#include "modem_dte.h"

#define MODEM_CMD_TO_DEF            (500)       /*!< Default timeout value for most commands */
#define MODEM_CMD_TO_OPERATOR       (75000)     /*!< Timeout value for getting operator status */
#define MODEM_CMD_TO_MODE_CHANGE    (3000)      /*!< Timeout value for changing working mode */
#define MODEM_CMD_TO_POWEROFF       (1000)      /*!< Timeout value for power down */
#define MODEM_CMD_TO_CFUN           (15000)     /*!< Timeout value for cfun command */

/**
 * @brief Working state of DCE
 *
 */
typedef enum _modem_state_t{
    MODEM_STATE_PROCESSING, /*!< In processing */
    MODEM_STATE_SUCCESS,    /*!< Process successfully */
    MODEM_STATE_FAIL        /*!< Process failed */
} modem_state_t;

/**
 * @brief DCE(Data Communication Equipment)
 *
 */
typedef struct _modem_dce_t
{
    modem_state_t state;    /*!< Modem working state */
    modem_mode_t mode;      /*!< Working mode */
    modem_dte_t *dte;       /*!< DTE which connect to DCE */
}modem_dce_t;

int8_t ModemDCEInit(modem_dce_t *dce, modem_dte_t *dte);
int8_t ModemDCESync(modem_dce_t *dce);
int8_t ModemDCEEcho(modem_dce_t *dce, uint8_t on);
int8_t ModemDCEGetRSSI(modem_dce_t *dce, int16_t *rssi);
int8_t ModemDCEGetIMEI(modem_dce_t *dce, char *imeiBuff);
int8_t ModemDCEGetIMSI(modem_dce_t *dce, char *imsiBuff);
int8_t ModemDCEGetNwStatus(modem_dce_t *dce, uint8_t *state);
int8_t ModemDCEGetGPRSStatus(modem_dce_t *dce, uint8_t *state);
int8_t ModemDCEFlightMode(modem_dce_t *dce, uint8_t on);
int8_t ModemDCEGetTime(modem_dce_t *dce, char *timeBuff);
int8_t ModemDCEHangUp(modem_dce_t *dce);
int8_t ModemDCESetNwScanMode(modem_dce_t *dce, uint8_t mode);

#endif /* __MODEM_DCE_H__ */
