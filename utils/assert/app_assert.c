/*
 * app_assert.c
 *
 *  Created on: 20-Sep-2023
 *      Author: Mahesh Murty
 */

#include "app_assert.h"
#include "cmsis_os.h"


void APP_ASSERT_DUMP_LOGS(void)
{
	if(osKernelGetState() == osKernelRunning)
	{
		osKernelLock();
	}
	portENABLE_INTERRUPTS();
	/* This will result in WDT getting triggered. */
	while(1)
	{
		/* These will flush all the log messages to the UART port. */
		TriceTransfer();
	}
}

