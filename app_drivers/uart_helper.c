/*
 * uart_helper.c
 *
 *  Created on: Sep 18, 2024
 *      Author: Mahesh Murty
 */

#include <stdio.h>
#include <string.h>
#include "uart_helper.h"

typedef enum _buffer_operations {
	BUFF_OP_DATA_ADD = 0,
	BUFF_OP_DATA_REM
}buff_operations_t;


/*!
 * \brief Variable holding reference of all the port configuration data structures.
 */
static uart_config_t *perConfig[APP_MAX_UARTS] = { NULL };

/*!
 * \brief Retrieve the configuration structure associated with a UART handle.
 * \param huart Pointer to the UART handle.
 * \return Pointer to the configuration structure or NULL if not found.
 */
static uart_config_t *GetConfig(UART_HandleTypeDef *huart)
{
	uart_config_t *config = NULL;

	for(uint8_t i = 0; i < APP_MAX_UARTS; i++)
	{
		if(perConfig[i]->huart == huart)
		{
			config = perConfig[i];
			break;
		}
	}

	return config;
}

static uint8_t GetPID(UART_HandleTypeDef *huart)
{
	uint8_t retVal = -1;

	for(uint8_t i = 0; i < APP_MAX_UARTS; i++)
	{
		if(perConfig[i]->huart == huart)
		{
			retVal = i;
			break;
		}
	}

	return retVal;
}

/*!
 * \brief Configure the Rx DMA based on the available buffer space.
 * \param config Pointer to the UART configuration structure.
 * \param opType Type of buffer operation (add or remove data).
 */
static void  ConfigureRxDMA(uart_config_t *config,
		buff_operations_t opType)
{
	/* Estimate remaining space in the buffer and start
	 * a new DMA transaction. */
	uint16_t dmaSize = 0;

	if(config->rxReadIndex == config->rxWriteIndex)
	{
		/* The read pointer and the write pointer are equal. If this function
		was called because data was added to the buffer, then there is no free
		space in the buffer remaining. If this function was called because data
		was removed from the buffer, then the space remaining is from the write
		pointer up to the end of the buffer. */
		if(opType == BUFF_OP_DATA_ADD)
		{
			dmaSize = 0;
		}
		else
		{
			dmaSize = config->rxBuffLen - config->rxWriteIndex;
		}
	}
	else if(config->rxReadIndex > config->rxWriteIndex)
	{
		/* The read pointer is ahead of the write pointer.  The space available
		is up to the write pointer to ensure unread data is not overwritten. */
		dmaSize = config->rxReadIndex - config->rxWriteIndex;
	}
	else
	{
		/* The write pointer is ahead of the read pointer so the space
		available is up to the end of the buffer. */
		dmaSize = config->rxBuffLen - config->rxWriteIndex;
	}

	if(dmaSize > 0)
	{
		HAL_UARTEx_ReceiveToIdle_DMA(config->huart,
			&config->rxBuffer[config->rxWriteIndex],
			dmaSize);
		/* Disable the DMA half transfer interrupt as we
		 * don't require that. */
		__HAL_DMA_DISABLE_IT(config->rxDmaHandle, DMA_IT_HT);
	}
}

/*!
 * \brief Callback function triggered upon UART Rx event.
 * \param huart Pointer to the UART handle.
 * \param Pos Number of bytes received.
 */
STATIC void UartRxEventCallback(UART_HandleTypeDef *huart, uint16_t Pos)
{
	uart_config_t *config = GetConfig(huart);

	if(config != NULL)
	{
		/* Update the receive byte count. */
		config->rxCount += Pos;
		/* Update the write pointer based on Pos. */
		config->rxWriteIndex += Pos;
		if(config->rxWriteIndex >= config->rxBuffLen)
		{
			config->rxWriteIndex = 0;
		}

		/* Start a new Rx DMA transaction over the remaining buffer. */
		ConfigureRxDMA(config, BUFF_OP_DATA_ADD);
	}
}

void UartErrCallback(UART_HandleTypeDef *huart)
{
	uint8_t pid;
	pid = GetPID(huart);
	UartHelperEnableRx(pid, false);
	UartHelperEnableRx(pid, true);
}

/*!
 * \brief Initialize the EUSCI_A peripheral in UART mode.
 * \param pid UART port identifier.
 * \param config Reference of the port configuration structure.
 * \note The driver will internally store the reference of the configuration structure
 * for future use. So make sure that this is allocated in a memory region that is valid
 * for the entire duration of the program.
 */
int8_t UartHelperInit(const uint8_t pid, uart_config_t* const config)
{
	if((pid > APP_MAX_UARTS) || (config == NULL) ||
	   (config->huart == NULL) || (config->rxDmaHandle == NULL))
	{
		/* Port not supported by this driver. */
		return APP_ERR_INV_ARG;
	}

    /* Do other error checks. */
    if(config->rxBuffer == NULL)
    {
    	config->rxBuffLen = 0;
    }
    else
    {
    	if(config->rxBuffLen == 0)
    	{
    		return APP_ERR_INV_ARG;
    	}
    }

    /* Save config variable for future reference. */
    perConfig[pid] = config;

    /* Reset the rx buffer index. */
    config->rxWriteIndex = 0;
    config->rxReadIndex = 0;
    config->rxCount = 0;

    HAL_UART_RegisterRxEventCallback(config->huart, UartRxEventCallback);
    HAL_UART_RegisterCallback(config->huart, HAL_UART_ERROR_CB_ID, UartErrCallback);

    return APP_ERR_NONE;
}

/*!
 * \brief De-Initialize the UART peripheral.
 * \param pid UART port identifier.
 * \retval APP_ERR_NONE on success, error code on failure.
 */
int8_t UartHelperDeInit(const uint8_t pid)
{
	int8_t retVal = APP_ERR_INV_ARG;

	if((pid < APP_MAX_UARTS) && (perConfig[pid] != NULL))
	{
		/* Abort the previous Rx DMA transaction (if any). */
		retVal = HAL_UART_AbortReceive(perConfig[pid]->huart);
		if(retVal == HAL_OK)
		{
			HAL_UART_UnRegisterRxEventCallback(perConfig[pid]->huart);
			HAL_UART_UnRegisterCallback(perConfig[pid]->huart, HAL_UART_ERROR_CB_ID);

			/* Clear the peripheral config. */
			perConfig[pid] = NULL;
			retVal = APP_ERR_NONE;
		}
		else
		{
			retVal = APP_ERR_STHAL;
		}
	}

	return retVal;
}

/*!
 * \brief Enable data reception on the UART port.
 * \param pid UART port identifier.
 * \param en Set true to enable UART reception, false otherwise.
 */
int8_t UartHelperEnableRx(const uint8_t pid, const bool en)
{
	int8_t retVal = APP_ERR_INV_ARG;
	/* Check if the UART peripheral ID does not exceed the
     * max allowed peripherals.
     */
    if(pid < APP_MAX_UARTS)
    {
    	uart_config_t *config = perConfig[pid];

    	if(config == NULL)
    		return retVal;

    	/* Reset the rx buffer index. */
		config->rxWriteIndex = 0;
		config->rxReadIndex = 0;
		config->rxCount = 0;

    	if(en)
		{
    		if((config->rxBuffer == NULL) || (config->rxBuffLen == 0))
			{
    			return retVal;
			}

    		/* Abort the previous Rx DMA transaction (if any) or simply return. */
    		retVal = HAL_UART_AbortReceive(perConfig[pid]->huart);

    		if(retVal == HAL_OK)
    		{
				/* Start a new Rx DMA transaction. */
				retVal = HAL_UARTEx_ReceiveToIdle_DMA(perConfig[pid]->huart,
											 perConfig[pid]->rxBuffer,
											 perConfig[pid]->rxBuffLen);
				/* Disable the DMA half transfer interrupt as we
				 * don't require that. */
				__HAL_DMA_DISABLE_IT(config->rxDmaHandle, DMA_IT_HT);
    		}
		}
		else
		{
			/* Abort the Rx DMA transaction. */
			retVal = HAL_UART_AbortReceive(perConfig[pid]->huart);
		}
    	if(retVal != HAL_OK)
		{
    		retVal = APP_ERR_STHAL;
		}
    	else
    	{
    		retVal = APP_ERR_NONE;
    	}
    }

    return retVal;
}

/*!
 * \brief Get the number of bytes received successfully by the UART peripheral.
 * \param pid UART port identifier.
 * \retval Count of total bytes received successfully by the UART driver.
 */
uint16_t UartHelperGetRxByteCount(const uint8_t pid)
{
    /* Check if the UART peripheral ID does not exceed the
     * max allowed peripherals.
     */
    if((pid < APP_MAX_UARTS) && (perConfig[pid] != NULL))
    {
    	return perConfig[pid]->rxCount;
    }
    else
    {
    	return 0;
    }
}

/*!
 * \brief Read a single byte from the UART ring buffer.
 * \param pid UART port identifier.
 * \param dataByte Address of the buffer where received data is to be copied.
 * \retval 1 if the byte was read successfully from the ring buffer, 0 otherwise.
 */
uint16_t UartHelperReadByte(const uint8_t pid, uint8_t* const dataByte)
{
	uint16_t retVal = 0;
	uint32_t primask_bit;

	/* Check if the UART peripheral ID does not exceed the
     * max allowed peripherals.
     */
	if((pid < APP_MAX_UARTS) && (perConfig[pid] != NULL) && (dataByte != NULL))
    {
    	/* Confirm whether the peripheral was initialized for
    	 * read operations. */
		if((perConfig[pid]->rxBuffer != NULL) && (perConfig[pid]->rxCount > 0))
		{
			/* Copy rx data to the destination buffer. */
			*dataByte = perConfig[pid]->rxBuffer[perConfig[pid]->rxReadIndex];

			/* Update the read pointer */
			perConfig[pid]->rxReadIndex++;
			if(perConfig[pid]->rxReadIndex >= perConfig[pid]->rxBuffLen)
			{
				perConfig[pid]->rxReadIndex = 0;
			}

			/* This indicates that the buffer was full and there is no
			 * ongoing DMA transaction. */
			if(perConfig[pid]->rxCount == perConfig[pid]->rxBuffLen)
			{
				ConfigureRxDMA(perConfig[pid], BUFF_OP_DATA_REM);
			}

			/* Enter critical section. */
			#ifndef TEST
			primask_bit = __get_PRIMASK();
		  	__disable_irq();
			#endif

		  	perConfig[pid]->rxCount--;

			/* Exit critical section: restore previous priority mask */
			#ifndef TEST
			__set_PRIMASK(primask_bit);
			#endif

			retVal = 1;
		}
    }

    return retVal;
}

/*!
 * \brief Send a byte stream over the UART data pins.
 * \param pid UART port identifier.
 * \param buff Address of the buffer where transmit data is stored.
 * \param len Size of the byte stream which is to be transmitted.
 * \note This is a blocking API.
 */
int8_t UartHelperSend(const uint8_t pid, const uint8_t* const buff, const uint16_t len)
{
	int8_t retVal = APP_ERR_INV_ARG;

    /* Validate all the arguments. */
	if((pid < APP_MAX_UARTS) && (perConfig[pid] != NULL) && (buff != NULL) && (len > 0))
	{
		/* Start an Interrupt based Tx transfer. */
		retVal = HAL_UART_Transmit_DMA(perConfig[pid]->huart, buff, len);

		if(retVal != HAL_OK)
		{
			retVal = APP_ERR_STHAL;
		}
		else
		{
			retVal = APP_ERR_NONE;
		}
	}

	return retVal;
}

uint8_t UartHelperIsTxReady(const uint8_t pid)
{
	uint8_t retVal = 0;

	if((pid < APP_MAX_UARTS) && (perConfig[pid] != NULL))
	{
		if(perConfig[pid]->huart->gState == HAL_UART_STATE_READY)
		{
			retVal = 1;
		}
	}

	return retVal;
}

/*!
 * \brief Get the reference of the UART config data structure for the specified port.
 * \param pid UART port identifier.
 * \retval Address of the configuration data structure for the specified port.
 */
uart_config_t *UartHelperGetConfigPtr(uint8_t pid)
{
    /* Check if the UART peripheral ID does not exceed the
     * max allowed peripherals.
     */
    if(pid < APP_MAX_UARTS)
    {
		return perConfig[pid];
    }

    return NULL;
}

#ifdef TEST
	void ClearConfig(void)
	{
		for(uint8_t i = 0; i < APP_MAX_UARTS; i++)
		{
			perConfig[i] = NULL;
		}
	}
#endif
