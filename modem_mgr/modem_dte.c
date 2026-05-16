/**
 * @file modem_dte.c
 * @author Mahesh Murty (mahesh@actchip.com)
 * @brief Modem DTE handler code.
 * @date 2024-06-08
 * 
 * @copyright Copyright (c) Actchip Pvt. Ltd. 2024
 * 
 */

#define PRINT_DBG_LOGS              0

#include "modem_dte.h"
#include "bsp/board.h"
#include "app_hal/app_hal_delay.h"
#include "app_hal/app_hal_uart.h"
#include "utils/trice/trice.h"
#include <string.h>
#if (PRINT_DBG_LOGS == 1)
/* TODO: Add trice header */
#endif
#define STATIC static

#define TAG                         "MODEM-DTE"
#define DEL_TIME_DUR_MS             10
#define MAX_RESP_HANDLERS           5

STATIC dte_resp_handler_t respHandlers[MAX_RESP_HANDLERS] = {};
static TaskHandle_t dteTaskHandle = NULL;

static int8_t ReadLine(modem_dte_t *dte, int32_t timeoutMs)
{
	int8_t retVal = APP_ERR_TIMEOUT;

    const int32_t maxTicks = (timeoutMs / DEL_TIME_DUR_MS);
    int32_t ticks = maxTicks;
	uint8_t ch = 0;

    dte->ind = 0;
    dte->buffer[0] = 0;

    uint8_t flag = 0;

	while (ticks > 0)
	{
        if(APPHAL_UART_ReadByte(dte->uartPid, &ch) == APP_ERR_NONE)
		{
            if(!flag)
            {
                /* Restore timeout ticks one time so that we do not end up
                 * discarding valid data due to rx - tx sync errors. */
                ticks = maxTicks;
                flag = 1;
            }
            /* Collect data in the buffer until you sense a LF */
            if (ch == '\n')
            {
                if(dte->ind >= 1)
                {
                    /* Terminating the string at the CR position. */
                    dte->buffer[dte->ind - 1] = '\0';
                    /* Decrement index to match data length. */
                    dte->ind--;
                    retVal = APP_ERR_NONE;
                    if(dte->ind > 0)
                    {
                        #if (PRINT_DBG_LOGS == 1)
                            /* TODO: Add trice dbg logs. */
                        #endif
                    }
                }
                else
                {
                    dte->ind = 0;
                    dte->buffer[0] = 0;
                    retVal = APP_ERR_INV_PACKET;
                }
                break;
            }
            dte->buffer[dte->ind] = ch;
            dte->ind++;

            /* Return error if there is no enough space in the rx buffer. */
            if(dte->ind >= MODEM_DTE_RX_BUFF_LEN)
            {
                retVal = APP_ERR_INV_MEM;
                break;
            }
		}
		else
		{
            APPHAL_Delay(DEL_TIME_DUR_MS);
            ticks--;
		}
	}

	return retVal;
}

static int8_t ProcessRxEvents(modem_dte_t *dte, int32_t timeoutMs)
{
    int8_t retVal;
    /* Read one line from the UART buffer. */
    retVal = ReadLine(dte, timeoutMs);

    if(retVal == APP_ERR_NONE)
    {
        /* Call registered event handlers. */
        for (uint8_t i = 0; i < MAX_RESP_HANDLERS; i++)
        {
            if(respHandlers[i] != NULL)
            {
                respHandlers[i]((char*)dte->buffer, dte->ind, NULL);
            }
        }
    }

    return retVal;
}

static int8_t AcquireLock(const modem_dte_t * const dte, int32_t timeoutMs)
{
    int8_t retVal = APP_ERR_TIMEOUT;

    if(xSemaphoreTake(dte->lock, pdMS_TO_TICKS(timeoutMs)) == pdTRUE)
    {
        retVal = APP_ERR_NONE;
    }

    return retVal;
}

static int8_t ReleaseLock(const modem_dte_t * const dte)
{
    xSemaphoreGive(dte->lock);
    return APP_ERR_NONE;
}

static void UartReader(void *param)
{
    modem_dte_t * const dte = (modem_dte_t *)param;

    while (1)
    {
        if(AcquireLock(dte, 1000) == APP_ERR_NONE)
        {
            ProcessRxEvents(dte, 50);
            
            ReleaseLock(dte);
        }
        /* This delay is requires because this task may have higher priority
         * than app tasks and this should not starve the lock. */
        APPHAL_Delay(50);
    }
}

/**
 * @brief Initializes the Modem DTE object and creates the UART handler task.
 *
 * @param dte Statically allocated modem DTE object to be initialized.
 * @return APP_ERR_NONE on success, else error code.
 */
int8_t ModemDTEInit(modem_dte_t *dte)
{
    /* Validate args. */

    /* NOTE: UART PID validation is very specific to ESP. We may need to
     * change it as per the platform. */
    if((dte == NULL) || (dte->uartPid > 2))
    {
        return APP_ERR_INV_ARG;
    }

    dte->lock = xSemaphoreCreateBinary();
    if(dte->lock == NULL)
    {
        return APP_ERR_NO_MEM;
    }
    /* Initialize the sem to 1. */
    xSemaphoreGive(dte->lock);

    /* TODO: Can we restrict creating multiple tasks with the same UART ID? */
    uint32_t status;
    status = xTaskCreate(UartReader, "DTE-UARTR", STACK_DTE_UART,
                        dte, PRIORITY_DTE_UART, &dte->uartTaskHdl);
    if(status != pdTRUE)
    {
        return APP_ERR_NO_MEM;
    }

    dteTaskHandle = dte->uartTaskHdl;
    return APP_ERR_NONE;
}

int8_t ModemDTESendWait(modem_dte_t *dte, modem_cmd_t *txCmd, int32_t timeoutMs)
{
    int8_t retVal = APP_ERR_INV_ARG;
    
    /* Validate args. */
    if((dte == NULL) || (txCmd == NULL) || (timeoutMs <= 0))
    {
        return retVal;
    }

    if(txCmd->cmd == NULL)
    {
        return retVal;
    }

    retVal = APP_ERR_TIMEOUT;
    if(AcquireLock(dte, timeoutMs) == APP_ERR_NONE)
    {
        /* Process any previous packets in the rx buffer.
         * Giving larger timeout here will un-necessarily block this function. */
        do
        {
            retVal = ProcessRxEvents(dte, 50);
        }while(retVal == APP_ERR_NONE);
        
        /* Send command. */
        #if (PRINT_DBG_LOGS == 1)
            ESP_LOGI(TAG, "SendingCmd: %s", txCmd->cmd);
        #endif
        retVal = APPHAL_UART_Write(dte->uartPid, (uint8_t*)txCmd->cmd, strlen(txCmd->cmd));
        /* If response is expected then read from rx buffer. */
        if(!txCmd->noResp)
        {
            if(retVal == APP_ERR_NONE)
            {
                int8_t err = APP_ERR_NONE;
                do
                {
                    /* Receive responses, call response handler. */
                    retVal = ReadLine(dte, timeoutMs);
                    if(retVal == APP_ERR_NONE)
                    {
                        if(txCmd->handler != NULL)
                        {
                            /* This will happen when echo is disabled and the modem 
                            * sends \r\n. Hence not forwarding this response to the app handler.
                            */
                            if(dte->ind == 0)
                            {
                                /* Modifying the error code here so that the
                                * loop doesn't break. */
                                err = APP_ERR_PROC_RESP;
                            }
                            else
                            {
                                err = txCmd->handler((char*)dte->buffer, dte->ind, txCmd->args);
                            }
                        }
                    }
                } while ((retVal == APP_ERR_NONE) && (err != APP_ERR_NONE)
                        && (err == APP_ERR_PROC_RESP));
                if(err != APP_ERR_NONE)
                {
                    retVal = APP_ERR_INV_RESP;
                }
            }
        }
        
        ReleaseLock(dte);
    }

    return retVal;
}

int8_t ModemDTESendData(modem_dte_t *dte, modem_data_t *txData, int32_t timeoutMs)
{
    int8_t retVal = APP_ERR_INV_ARG;
    
    /* Validate args. */
    if((dte == NULL) || (txData == NULL) || (timeoutMs <= 0))
    {
        return retVal;
    }

    if((txData->data == NULL) || (txData->dataLen == 0))
    {
        return retVal;
    }

    retVal = APP_ERR_TIMEOUT;
    if(AcquireLock(dte, timeoutMs) == APP_ERR_NONE)
    {   
        /* Transmit data. */
        /* TODO: Use printf to print below packet. */
        retVal = APPHAL_UART_Write(dte->uartPid, txData->data, txData->dataLen);
        /* If response is expected then read from rx buffer. */
        if(!txData->noResp)
        {
            if(retVal == APP_ERR_NONE)
            {
                int8_t err = APP_ERR_NONE;
                do
                {
                    /* Receive responses, call response handler. */
                    retVal = ReadLine(dte, timeoutMs);
                    if(retVal == APP_ERR_NONE)
                    {
                        if(txData->handler != NULL)
                        {
                            /* This will happen when echo is disabled and the modem 
                            * sends \r\n. Hence not forwarding this response to the app handler.
                            */
                            if(dte->ind == 0)
                            {
                                /* Modifying the error code here so that the
                                * loop doesn't break. */
                                err = APP_ERR_PROC_RESP;
                            }
                            else
                            {
                                err = txData->handler((char*)dte->buffer, dte->ind, txData->args);
                            }
                        }
                    }
                } while ((retVal == APP_ERR_NONE) && (err != APP_ERR_NONE)
                        && (err == APP_ERR_PROC_RESP));
                if(err != APP_ERR_NONE)
                {
                    retVal = APP_ERR_INV_RESP;
                }
            }
        }
        
        ReleaseLock(dte);
    }

    return retVal;
}

int8_t ModemDTEFlushRx(modem_dte_t *dte, int32_t timeoutMs)
{
    int8_t retVal = APP_ERR_INV_ARG;
    
    /* Validate args. */
    if((dte == NULL) || (timeoutMs <= 0))
    {
        return retVal;
    }

    ReadLine(dte, timeoutMs);

    return APP_ERR_NONE;   
}

int8_t ModemDTERegEvtHandler(modem_dte_t *dte, dte_resp_handler_t handler)
{
    int8_t retVal = APP_ERR_INV_ARG;
    
    /* Validate args. */
    if((dte == NULL) || (handler == NULL))
    {
        return retVal;
    }

    retVal = APP_ERR_TIMEOUT;
    
    if(AcquireLock(dte, 5000) == APP_ERR_NONE)
    {
        retVal = APP_ERR_NO_MEM;
        for (uint8_t i = 0; i < MAX_RESP_HANDLERS; i++)
        {
            if(respHandlers[i] != NULL)
            {
                if(respHandlers[i] == handler)
                {
                    /* Duplication detected, hence not adding it to the
                     * respHandlers table. */
                    retVal = APP_ERR_INV_ARG;
                    break;
                }
            }
            else
            {
                /* Free slot detected. */
                respHandlers[i] = handler;
                retVal = i;
                break;
            }
        }
        
        ReleaseLock(dte);
    }

    return retVal;
}

int8_t ModemDTEUnRegEvtHandler(modem_dte_t *dte, int8_t handle)
{
    int8_t retVal = APP_ERR_INV_ARG;
    
    /* Validate args. */
    if((dte == NULL) || (handle >= MAX_RESP_HANDLERS) || (handle < 0))
    {
        return retVal;
    }

    retVal = APP_ERR_TIMEOUT;
    
    if(AcquireLock(dte, 5000) == APP_ERR_NONE)
    {
        respHandlers[handle] = NULL;
        retVal = APP_ERR_NONE;
        
        ReleaseLock(dte);
    }

    return retVal;
}

void DTETaskPrintDiags(void)
{
	if(dteTaskHandle == NULL)
	{
		uint32_t space = uxTaskGetStackHighWaterMark(dteTaskHandle) * sizeof(StackType_t);
		trice32(iD(7449), "dbg: diag: DTE-Task-MGR stack rem = %d \n", space);
	}
}

#ifdef TEST

    void ClearEvtHandlerTable(void)
    {
        for (uint8_t i = 0; i < MAX_RESP_HANDLERS; i++)
        {
            respHandlers[i] = NULL;
        }  
    }

#endif
