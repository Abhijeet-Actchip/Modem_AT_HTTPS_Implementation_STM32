/*
 * board.h
 *
 *  Created on: May 15, 2026
 *      Author: abhij
 */

#ifndef BOARD_H_
#define BOARD_H_

#include "config/appconfig.h"
#include "main.h"
#include <stdbool.h>

#if (SELECT_BOARD == BOARD_ID_V1_0)
	#include "board_v1_0.h"
#endif

/* BSP Prototypes */
int8_t Board_InitPeripherals(void);
int8_t Board_CtrlUserLED2(bool en);
int8_t Board_CtrlUserLED1(bool en);
int8_t Board_Enable3V3LDO(bool en);
int8_t Board_CtrlModemPwrKey(bool en);
int8_t Board_EnableModemPower(bool en);
/* TODO: Add BSP API headers here */


#endif /* BOARD_H_ */
