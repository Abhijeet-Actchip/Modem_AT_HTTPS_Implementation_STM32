/**
 * @file modem_dce.c
 * @author Mahesh Murty (mahesh@actchip.com)
 * @brief Modem DCE handler code.
 * @date 2024-06-08
 * 
 * @copyright Copyright (c) Actchip Pvt. Ltd. 2026
 * 
 */

#include "modem_dce.h"
#include "bsp/board.h"
#include "app_hal/app_hal_delay.h"
#include <string.h>
#include <stdio.h>

#define TAG         "MODEM-DCE"
#define STATIC static

STATIC int8_t sync_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    /* You will get this response when echo is enabled. */
    if(strncmp(resp, "AT", 2) == 0)
    {
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        retVal = APP_ERR_NONE;
    }
    
    return retVal;
}

STATIC int8_t echo_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    /* You will get this response when echo is enabled. */
    if((strncmp(resp, "ATE0", 4) == 0) || (strncmp(resp, "ATE1", 4) == 0))
    {
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        retVal = APP_ERR_NONE;
    }
    
    return retVal;
}

/* All the below handlers assume that echo is disabled when implemented. */

static int16_t ConvertRssi(uint16_t intRssi)
{
	int16_t finalRssi;

	if(intRssi == 0)
	{
		finalRssi = -113;
	}
	else if(intRssi == 1)
	{
		finalRssi = -111;
	}
	else if((intRssi >=2) && (intRssi <=30))
	{
		finalRssi = (2 * (intRssi - 2)) - 109;
	}
	else if(intRssi == 31)
	{
		finalRssi = -51;
	}
	else
	{
		/* Unknown. */
		finalRssi = -1;
	}

	return finalRssi;
}

STATIC int8_t csq_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    uint16_t temp = 0;

    if(strncmp(resp, "+CSQ", 4) == 0)
    {
        if(args != NULL)
        {
            /* Convert and store result. */
            sscanf(resp, "%*s%hd", &temp);
            *((int16_t *)args) = ConvertRssi(temp);
        }
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        retVal = APP_ERR_NONE;
    }
    
    return retVal;
}

STATIC int8_t cgsn_cimi_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;

    /* We receive a number directly. */
    if((len > 10) && (len < 20))
    {
        if(args != NULL)
        {
            /* Store result. */
            strcpy(args, resp);
        }
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        retVal = APP_ERR_NONE;
    }
    
    return retVal;
}

STATIC int8_t creg_cgreg_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    uint16_t temp = -1;
    if((strncmp(resp, "+CREG", 5) == 0) || (strncmp(resp, "+CGREG", 6) == 0))
    {
        if(args != NULL)
        {
            /* Convert and store result. */
            sscanf(resp, "%*s%*d,%hd", &temp);
            *((uint8_t *)args) = temp;
        }
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        retVal = APP_ERR_NONE;
    }
    
    return retVal;
}

STATIC int8_t qlts_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;

    if((strncmp(resp, "+QLTS", 5) == 0) && (len > 9))
    {
        if(args != NULL)
        {
            /* Store result. */
            strcpy(args, &resp[7]);
        }
        /* Indicates we are still processing responses for this command. */
        retVal = APP_ERR_PROC_RESP;
    }
    else if(strncmp(resp, "OK", 2) == 0)
    {
        retVal = APP_ERR_NONE;
    }
    
    return retVal;
}

STATIC int8_t default_resp_handler(const char * const resp, const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;

    if(strncmp(resp, "OK", 2) == 0)
    {
        retVal = APP_ERR_NONE;
    }
    
    return retVal;
}

int8_t ModemDCEInit(modem_dce_t *dce, modem_dte_t *dte)
{
    int8_t retVal = APP_ERR_INV_ARG;

    if(dce == NULL)
    {
        return retVal;
    }

    retVal = ModemDTEInit(dte);
    if(retVal == APP_ERR_NONE)
    {
        dce->mode = MODEM_COMMAND_MODE;
        /* Save DTE instance pointer. */
        dce->dte = dte;
    }

    return retVal;
}

int8_t ModemDCESync(modem_dce_t *dce)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = {
        .cmd = "AT\r",
        .handler = sync_resp_handler
    };

    if(dce == NULL)
    {
        return retVal;
    }

    retVal = ModemDTESendWait(dce->dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t ModemDCEEcho(modem_dce_t *dce, uint8_t on)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = {
        .handler = echo_resp_handler
    };

    if(dce == NULL)
    {
        return retVal;
    }

    if(on)
    {
        txCmd.cmd = "ATE1\r";
    }
    else
    {
        txCmd.cmd = "ATE0\r";
    }

    retVal = ModemDTESendWait(dce->dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t ModemDCEGetRSSI(modem_dce_t *dce, int16_t *rssi)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = {
        .cmd = "AT+CSQ\r",
        .args = rssi,
        .handler = csq_resp_handler
    };

    if((dce == NULL) || (rssi == NULL))
    {
        return retVal;
    }

    retVal = ModemDTESendWait(dce->dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;    
}

int8_t ModemDCEGetIMEI(modem_dce_t *dce, char *imeiBuff)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = {
        .cmd = "AT+CGSN\r",
        .args = imeiBuff,
        .handler = cgsn_cimi_resp_handler
    };

    if((dce == NULL) || (imeiBuff == NULL))
    {
        return retVal;
    }

    retVal = ModemDTESendWait(dce->dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t ModemDCEGetIMSI(modem_dce_t *dce, char *imsiBuff)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = {
        .cmd = "AT+CIMI\r",
        .args = imsiBuff,
        .handler = cgsn_cimi_resp_handler
    };

    if((dce == NULL) || (imsiBuff == NULL))
    {
        return retVal;
    }

    retVal = ModemDTESendWait(dce->dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t ModemDCEGetNwStatus(modem_dce_t *dce, uint8_t *state)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = {
        .cmd = "AT+CREG?\r",
        .args = state,
        .handler = creg_cgreg_resp_handler
    };

    if((dce == NULL) || (state == NULL))
    {
        return retVal;
    }

    retVal = ModemDTESendWait(dce->dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t ModemDCEGetGPRSStatus(modem_dce_t *dce, uint8_t *state)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = {
        .cmd = "AT+CGREG?\r",
        .args = state,
        .handler = creg_cgreg_resp_handler
    };

    if((dce == NULL) || (state == NULL))
    {
        return retVal;
    }

    retVal = ModemDTESendWait(dce->dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t ModemDCEGetTime(modem_dce_t *dce, char *timeBuff)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = {
        .cmd = "AT+QLTS=1\r",
        .args = timeBuff,
        .handler = qlts_resp_handler
    };

    if((dce == NULL) || (timeBuff == NULL))
    {
        return retVal;
    }

    retVal = ModemDTESendWait(dce->dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t ModemDCEFlightMode(modem_dce_t *dce, uint8_t on)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = {
        .handler = default_resp_handler
    };

    if(dce == NULL)
    {
        return retVal;
    }

    if(on)
    {
        txCmd.cmd = "AT+CFUN=4,1\r";
    }
    else
    {
        txCmd.cmd = "AT+CFUN=1,1\r";
    }

    retVal = ModemDTESendWait(dce->dte, &txCmd, MODEM_CMD_TO_CFUN);

    return retVal;
}

int8_t ModemDCEHangUp(modem_dce_t *dce)
{
    int8_t retVal = APP_ERR_INV_ARG;
    modem_cmd_t txCmd = {
        .cmd = "+++",
        .noResp = 1
    };

    if(dce == NULL)
    {
        return retVal;
    }

    retVal = ModemDTESendWait(dce->dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;    
}

int8_t ModemDCESetNwScanMode(modem_dce_t *dce, uint8_t mode)
{
	int8_t retVal = APP_ERR_INV_ARG;
	char cmdStr[64];

	modem_cmd_t txCmd = {
		.cmd = cmdStr,
		.handler = default_resp_handler
	};

	if(dce == NULL)
	{
		return retVal;
	}

	switch(mode)
	{
		case 1:
		case 2:
		case 3:
			break;
		default:
			/* Setting automatic network scan mode. */
			mode = 0;
			break;
	}

	/* Form command. */
	snprintf(cmdStr, sizeof(cmdStr),
			"AT+QCFG=\"nwscanmode\",%d\r", mode);

	retVal = ModemDTESendWait(dce->dte, &txCmd, MODEM_CMD_TO_MODE_CHANGE);

	return retVal;
}
