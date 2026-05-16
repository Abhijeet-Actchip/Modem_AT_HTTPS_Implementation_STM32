/*
 * uart_helper.h
 *
 *  Created on: Sep 18, 2024
 *      Author: Mahesh Murty
 */

#ifndef __UART_HELPER_H__
#define __UART_HELPER_H__				1

#include "bsp/board.h"
#include <stdint.h>
#include <stdbool.h>

/*!
 * \brief Data structure to hold a UART port configuration.
 */
typedef struct uart_config_t
{
	UART_HandleTypeDef *huart;
	DMA_HandleTypeDef *rxDmaHandle;
	uint8_t *rxBuffer;/*!< Pointer to a pre-allocated buffer used during UART Rx. */
    uint16_t rxBuffLen;/*!< Size of the Rx buffer. */

    /* Private variables */
    volatile uint16_t rxWriteIndex, rxReadIndex;
    volatile uint16_t rxCount;
}uart_config_t;

/*!
 * \brief Initialize the UART helper library.
 * \param pid UART port identifier.
 * \param config Reference of the port configuration structure.
 * \note The driver will internally store the reference of the configuration structure
 * for future use. So make sure that this is allocated in a memory region that is valid
 * for the entire duration of the program.
 */
int8_t UartHelperInit(const uint8_t pid, uart_config_t* const config);

/*!
 * \brief De-Initialize the UART helper library.
 * \param pid UART port identifier.
 * \retval APP_ERR_NONE on success, error code on failure.
 */
int8_t UartHelperDeInit(const uint8_t pid);

/*!
 * \brief Enable data reception on the UART port.
 * \param pid UART port identifier.
 * \param en Set true to enable UART reception, false otherwise.
 */
int8_t UartHelperEnableRx(const uint8_t pid, const bool en);

/*!
 * \brief Get the number of bytes received successfully by the UART peripheral.
 * \param pid UART port identifier.
 * \retval Count of total bytes received successfully by the UART driver.
 */
uint16_t UartHelperGetRxByteCount(const uint8_t pid);

/*!
 * \brief Read a single byte from the UART ring buffer.
 * \param pid UART port identifier.
 * \param dataByte Address of the buffer where received data is to be copied.
 * \retval 1 if the byte was read successfully from the ring buffer, 0 otherwise.
 */
uint16_t UartHelperReadByte(const uint8_t pid, uint8_t* const dataByte);

/*!
 * \brief Send a byte stream over the UART data pins.
 * \param pid UART port identifier.
 * \param buff Address of the buffer where transmit data is stored.
 * \param len Size of the byte stream which is to be transmitted.
 */
int8_t UartHelperSend(const uint8_t pid, const uint8_t* const buff, const uint16_t len);

/*!
 * \brief Read UART Tx ready status.
 * \param pid UART port identifier.
 * \retval 1 if the peripheral is ready to transmit, 0 otherwise.
 */
uint8_t UartHelperIsTxReady(const uint8_t pid);

/*!
 * \brief Get the reference of the UART config data structure for the specified port.
 * \param pid UART port identifier.
 * \retval Address of the configuration data structure for the specified port.
 */
uart_config_t *UartHelperGetConfigPtr(uint8_t pid);


#endif /* __UART_HELPER_H__ */
