/*
 * appconfig.h
 *
 *  Created on: May 15, 2026
 *      Author: abhij
 */

#ifndef APPCONFIG_H_
#define APPCONFIG_H_


/* Define Board-ID macros here. */
#define BOARD_ID_V1_0				1

/* Define application specific settings macros here. */
#define SELECT_BOARD                BOARD_ID_V1_0

/* Define stack size and priority macros for tasks. */
/* TODO: Add task stack and priorities */
#define STACK_TESTSUITE				(1024)
#define PRIORITY_TESTSUITE			(tskIDLE_PRIORITY + 1)

#define DIAG_SVC_STACK_SIZE			(512)
#define DIAG_SVC_PRIORITY			(tskIDLE_PRIORITY + 1)

#define MODEM_HTTP_STACK_SIZE		(512)
#define MODEM_HTTP_PRIORITY			(tskIDLE_PRIORITY + 5)

#define STACK_DTE_UART				(512)
#define PRIORITY_DTE_UART			(tskIDLE_PRIORITY + 5)

#define STACK_APPMGR				(256)
#define PRIORITY_APPMGR				(tskIDLE_PRIORITY + 4)

/* HTTPS configurations */
#define HTTP_METHOD_GET             0
#define HTTP_METHOD_POST            1

#define APP_TS_HTTPS_CHANNEL_ID     3373681
#define HTTP_METHOD_USE             HTTP_METHOD_POST
#define APP_HTTPS_URL               "https://api.thingspeak.com/update.json"
#define APP_HTTPS_WRITE_API_KEY     "LCSF5OF3SCQZOP6U"
#define APP_HTTPS_READ_API_KEY      "HYZJX14GWRIRBF4I"
#define HTTP_RX_QUEUE_SIZE          10
#define HTTP_TX_QUEUE_SIZE          10
#define APP_USE_TLS					0


/*!< Enable Watchdog timer. (Should be enabled on production builds
 * disabled when debugging.) */
#define APP_WDT_EN					0

/* Error codes returned by functions. */
#define APP_ERR_NONE                0
#define APP_ERR_INV_CMDID           -1
#define APP_ERR_UNKNOWN             -2
#define APP_ERR_DUP_VAL             -3
#define APP_ERR_INV_LEN             -4
#define APP_ERR_INV_ARG             -5
#define APP_ERR_NOT_FOUND			-6
#define APP_ERR_TIMEOUT             -7
#define APP_ERR_INV_VAL        		-8
#define APP_ERR_NO_MEM              -9
#define APP_ERR_RESP_INV			-12
#define APP_ERR_NO_ACK				-13
#define APP_ERR_STHAL				-14
#define APP_ERR_INV_STATE			-15
#define APP_ERR_BUS_BUSY			-16
#define APP_ERR_NO_RESP				-17
#define APP_ERR_INV_RESP			-18
#define APP_ERR_PROC_RESP			-19
#define APP_ERR_INV_PACKET			-20
#define APP_ERR_INV_MEM				-21

/******************** Advanced user settings below (DO NOT EDIT) ********************/

#define ARRAY_SIZE(foo)             (sizeof(foo)/sizeof(foo[0]))

#ifdef TEST
    #define STATIC
#else
    #define STATIC static
#endif

#ifndef SELECT_BOARD
    #error "Board not selected."
#endif


#endif /* APPCONFIG_H_ */
