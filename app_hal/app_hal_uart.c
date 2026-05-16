/*
 * uart_hal.c
 *
 *  Created on: 11-April-2023
 *      Author: Mahesh Murty
 */

#include "config/appconfig.h"
#include "app_hal_uart.h"
#include "app_drivers/uart_helper.h"
#include <stdio.h>
/*!
 * \brief Reads a byte from the specified UART port.
 *
 * This function attempts to read a byte from the UART port identified by \p pid.
 * If successful, the received byte is stored in \p rxByte.
 *
 * \param pid Identifier for the UART port.
 * \param rxByte Pointer to store the received byte.
 * \return Error code indicating the success or failure of the read operation.
 */
int8_t APPHAL_UART_ReadByte(const uint8_t pid, uint8_t* const rxByte)
{
	int8_t retVal = APP_ERR_TIMEOUT;

	if(UartHelperReadByte(pid, rxByte))
	{
		retVal = APP_ERR_NONE;
	}

	return retVal;
}
/*!
 * \brief Writes data to the specified UART port.
 *
 * This function sends a buffer of data with length \p len to the UART port identified by \p pid.
 *
 * \param pid Identifier for the UART port.
 * \param buff Pointer to the data buffer to be transmitted.
 * \param len Length of the data buffer to be transmitted.
 * \return Error code indicating the success or failure of the write operation.
 */
int8_t APPHAL_UART_Write(const uint8_t pid, const uint8_t* const buff, const uint16_t len)
{
	return UartHelperSend(pid, buff, len);
}
/*!
 * \brief Flushes the receive buffer of the specified UART port.
 *
 * This function continuously reads bytes from the UART port identified by \p pid until the buffer is empty
 * or a maximum limit (MAX_LIMIT) is reached to avoid blocking.
 *
 * \param pid Identifier for the UART port.
 * \return Error code indicating the success of the flush operation.
 */
int8_t APPHAL_UART_FlushRx(const uint8_t pid)
{
	/* Limit continuous flushing to avoid blocking. */
	#define MAX_LIMIT			256

	uint8_t rxByte;

#ifdef MAX_LIMIT
	for (uint16_t i = MAX_LIMIT; i > 0; i--)
#else
	while(1)
#endif
	{
		if(!UartHelperReadByte(pid, &rxByte))
		{
			break;
		}
	}

	return APP_ERR_NONE;
}
/*!
 * \brief Checks the write status of the specified UART port.
 *
 * This function checks if the UART port identified by \p pid is ready to transmit data.
 *
 * \param pid Identifier for the UART port.
 * \return Error code indicating the write status of the UART port.
 */
int8_t APPHAL_UART_Get_WriteStatus(const uint8_t pid)
{
	int8_t retVal = APP_ERR_BUS_BUSY;

	if(UartHelperIsTxReady(pid))
	{
		retVal = APP_ERR_NONE;
	}

	return retVal;
}
