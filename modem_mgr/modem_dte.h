/**
 * @file modem_dte.h
 * @author Mahesh Murty (mahesh@actchip.com)
 * @brief Modem DTE handler code.
 * @date 2024-06-08
 * 
 * @copyright Copyright (c) Actchip Pvt. Ltd. 2024
 * 
 */

#ifndef __MODEM_DTE_H__
#define __MODEM_DTE_H__             1

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define MODEM_DTE_RX_BUFF_LEN       1024

/**
 * @brief Working mode of Modem
 *
 */
typedef enum _modem_mode_t{
    MODEM_COMMAND_MODE = 0, /*!< Command Mode */
    MODEM_PPP_MODE          /*!< PPP Mode */
} modem_mode_t;

typedef struct _modem_dte_t
{
    uint32_t uartPid;                           /*!< UART port identifier. */
    uint8_t buffer[MODEM_DTE_RX_BUFF_LEN];      /*!< Internal buffer to store response lines/data from DCE */
    uint16_t ind;
    TaskHandle_t uartTaskHdl;                   /*!< UART event task handle */
    SemaphoreHandle_t lock;                     /*!< Semaphore to lock the peripheral when it is busy. */
}modem_dte_t;

typedef int8_t (*dte_resp_handler_t)(const char * const resp, const uint16_t len, void * args);

typedef struct _modem_cmd_t
{
    char *cmd;
    uint8_t noResp;
    void *args;
    dte_resp_handler_t handler;
}modem_cmd_t;

typedef struct _modem_data_t
{
    uint8_t *data;
    uint16_t dataLen;
    uint8_t noResp;
    void *args;
    dte_resp_handler_t handler;
}modem_data_t;

/**
 * @brief Initializes the Modem DTE object and creates the UART handler task.
 *
 * @param dte Statically allocated modem DTE object to be initialized.
 * @return APP_ERR_NONE on success, else error code.
 */
int8_t ModemDTEInit(modem_dte_t *dte);
int8_t ModemDTESendWait(modem_dte_t *dte, modem_cmd_t *txCmd, int32_t timeoutMs);
int8_t ModemDTESendData(modem_dte_t *dte, modem_data_t *txData, int32_t timeoutMs);
int8_t ModemDTEFlushRx(modem_dte_t *dte, int32_t timeoutMs);
int8_t ModemDTERegEvtHandler(modem_dte_t *dte, dte_resp_handler_t handler);
int8_t ModemDTEUnRegEvtHandler(modem_dte_t *dte, int8_t handle);

#endif /* __MODEM_DTE_H__ */
