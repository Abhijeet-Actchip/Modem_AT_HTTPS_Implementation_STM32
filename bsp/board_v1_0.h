/*
 * board_v1_0.h
 *
 *  Created on: May 15, 2026
 *      Author: abhij
 */

#ifndef BOARD_V1_0_H_
#define BOARD_V1_0_H_

#include "main.h"

#include "board.h"

#if (SELECT_BOARD == BOARD_ID_V1_0)

/*! \brief Maximum number of UART ports supported by the helper driver. */
#define APP_MAX_UARTS					2
/* UART port identifiers. */
#define BOARD_MODEM_UART_PID			0

#define BOARD_TRICE_UART_PID			1


/* SPI port identifier. */
#define BOARD_SPI1_PID					0
/* I2C port identifier */
#define BOARD_I2C1_PID					0

#endif /* SELECT_BOARD */


#endif /* BOARD_V1_0_H_ */
