/**
 * @file modem_ec200u_gnss.c
 * @author Abhijeet (abhijeet@actchip.com)
 * @brief Quectel EC200U / EG912U-GL GNSS AT command implementation.
 *        Based on: Quectel EC200U Series & EG912U-GL GNSS Application Note v1.2
 * @date 2026-05-25
 *
 * @copyright Copyright (c) Actchip Pvt. Ltd. 2026
 *
 * ---- Command reference ----
 *  AT+QGPSCFG="outport","<port>"       Configure NMEA output port
 *  AT+QGPSCFG="nmeasrc",<en>           Enable/disable NMEA acquisition via AT
 *  AT+QGPSCFG="gpsnmeatype",<mask>     Select GPS NMEA sentence types (bitmask)
 *  AT+QGPSCFG="gnssconfig",<cfg>       Select GNSS constellation(s)
 *  AT+QGPSCFG="autogps",<en>           GNSS auto-start on power-up
 *  AT+QGPS=1                           Turn ON GNSS engine
 *  AT+QGPSEND                          Turn OFF GNSS engine
 *  AT+QGPSLOC=<mode>                   Acquire current location
 *  AT+QGPSGNMEA="<type>"               Read single NMEA sentence
 */

#include "modem_ec200u.h"       /* Full ec200_dce_t definition + ModemDTESendWait */
#include "modem_ec200u_gnss.h"  /* GNSS enums, structs, prototypes */
#include "config/appconfig.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define STATIC static

/* =========================================================================
 * Private response handlers
 * ========================================================================= */

/**
 * @brief Generic OK response handler — used for simple set commands.
 */
STATIC int8_t gnss_default_resp_handler(const char * const resp,
                                        const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;

    if (strncmp(resp, "OK", 2) == 0)
    {
        retVal = APP_ERR_NONE;
    }

    return retVal;
}

/**
 * @brief Response handler for AT+QGPSLOC.
 *
 * Expected success response (mode=2, decimal degrees):
 *   +QGPSLOC: <UTC>,<lat>,<lon>,<HDOP>,<alt>,<fix>,<COG>,<spkm>,<spkn>,<date>,<nsat>
 *
 * On no-fix the modem returns:
 *   +CME ERROR: 516
 */
STATIC int8_t qgpsloc_resp_handler(const char * const resp,
                                   const uint16_t len, void *args)
{
    int8_t retVal = APP_ERR_INV_RESP;
    ec200_gnss_location_t *loc = (ec200_gnss_location_t *)args;

    if (strncmp(resp, "+QGPSLOC:", 9) == 0)
    {
        if (loc != NULL)
        {
            /* Format: +QGPSLOC: HHMMSS.SS,lat,lon,HDOP,alt,fix,COG,spkm,spkn,DDMMYY,nsat */
            int  fix   = 0;
            int  nsat  = 0;
            char utcBuf[12]  = {0};
            char dateBuf[8]  = {0};

            int parsed = sscanf(resp,
                "+QGPSLOC: %11[^,],%f,%f,%f,%f,%d,%f,%f,%f,%7[^,],%d",
                utcBuf,
                &loc->latitude,
                &loc->longitude,
                &loc->hdop,
                &loc->altitude,
                &fix,
                &loc->cog,
                &loc->speedKmh,
                &loc->speedKnots,
                dateBuf,
                &nsat);

            if (parsed >= 6)
            {
                /* Copy string fields. */
                strncpy(loc->utc,  utcBuf,  sizeof(loc->utc)  - 1);
                strncpy(loc->date, dateBuf, sizeof(loc->date) - 1);
                loc->fixType  = (uint8_t)fix;
                loc->numSats  = (uint8_t)nsat;

                /* Processing – wait for the final OK. */
                retVal = APP_ERR_PROC_RESP;
            }
        }
        else
        {
            /* No storage provided, still continue to wait for OK. */
            retVal = APP_ERR_PROC_RESP;
        }
    }
    else if (strncmp(resp, "OK", 2) == 0)
    {
        retVal = APP_ERR_NONE;
    }
    else if (strncmp(resp, "+CME ERROR", 10) == 0)
    {
        /* CME ERROR: 516 = not fixed yet, 507 = invalid param, etc. */
        retVal = APP_ERR_INV_RESP;
    }

    return retVal;
}

/* =========================================================================
 * Private helpers
 * ========================================================================= */

/**
 * @brief Map ec200_gnss_outport_t enum to the AT command port string.
 */
STATIC const char *outport_to_str(ec200_gnss_outport_t port)
{
    switch (port)
    {
        case EC200_GNSS_OUTPORT_UART1:    return "uart1";
        case EC200_GNSS_OUTPORT_UART2:    return "uart2";
        case EC200_GNSS_OUTPORT_USBAT:    return "usbat";
        case EC200_GNSS_OUTPORT_USBMODEM: return "usbmodem";
        case EC200_GNSS_OUTPORT_USBNMEA:  return "usbnmea";
        default:                          return "none";
    }
}

/* =========================================================================
 * Public API implementations
 * ========================================================================= */

int8_t EC200DCEGPSConfig(ec200_dce_t *dce, const ec200_gnss_cfg_t *cfg)
{
    int8_t  retVal = APP_ERR_INV_ARG;
    char    cmdStr[64];

    modem_cmd_t txCmd = {
        .cmd     = cmdStr,
        .handler = gnss_default_resp_handler
    };

    if ((dce == NULL) || (cfg == NULL))
    {
        return retVal;
    }

    /* ---- 1. Configure NMEA output port ---------------------------------- */
    snprintf(cmdStr, sizeof(cmdStr),
             "AT+QGPSCFG=\"outport\",\"%s\"\r",
             outport_to_str(cfg->outport));

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
    if (retVal != APP_ERR_NONE)
    {
        return retVal;
    }

    /* ---- 2. Configure NMEA source (acquisition via AT port) ------------- */
    snprintf(cmdStr, sizeof(cmdStr),
             "AT+QGPSCFG=\"nmeasrc\",%d\r",
             cfg->nmeasrcEn ? 1 : 0);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
    if (retVal != APP_ERR_NONE)
    {
        return retVal;
    }

    /* ---- 3. Configure GPS NMEA sentence types (bitmask) ----------------- */
    snprintf(cmdStr, sizeof(cmdStr),
             "AT+QGPSCFG=\"gpsnmeatype\",%d\r",
             (int)cfg->nmeatype);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);
    if (retVal != APP_ERR_NONE)
    {
        return retVal;
    }

    /* ---- 4. Configure GNSS constellation -------------------------------- */
    snprintf(cmdStr, sizeof(cmdStr),
             "AT+QGPSCFG=\"gnssconfig\",%d\r",
             (int)cfg->gnssConfig);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t EC200DCEGPSTurnOn(ec200_dce_t *dce)
{
    int8_t retVal = APP_ERR_INV_ARG;

    modem_cmd_t txCmd = {
        .cmd     = "AT+QGPS=1\r",
        .handler = gnss_default_resp_handler
    };

    if (dce == NULL)
    {
        return retVal;
    }

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QGPS);

    return retVal;
}

int8_t EC200DCEGPSTurnOff(ec200_dce_t *dce)
{
    int8_t retVal = APP_ERR_INV_ARG;

    modem_cmd_t txCmd = {
        .cmd     = "AT+QGPSEND\r",
        .handler = gnss_default_resp_handler
    };

    if (dce == NULL)
    {
        return retVal;
    }

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QGPSEND);

    return retVal;
}

int8_t EC200DCEGPSGetLocation(ec200_dce_t *dce, ec200_gpsloc_mode_t mode,
                              ec200_gnss_location_t *loc)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char   cmdStr[32];

    modem_cmd_t txCmd = {
        .cmd     = cmdStr,
        .handler = qgpsloc_resp_handler,
        .args    = loc
    };

    if ((dce == NULL) || (loc == NULL))
    {
        return retVal;
    }

    if ((mode != EC200_GPSLOC_MODE_DMS) && (mode != EC200_GPSLOC_MODE_DEG))
    {
        return retVal;
    }

    /* Initialise output to safe values. */
    memset(loc, 0, sizeof(ec200_gnss_location_t));

    snprintf(cmdStr, sizeof(cmdStr), "AT+QGPSLOC=%d\r", (int)mode);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, EC200_CMD_TO_QGPSLOC);

    return retVal;
}

int8_t EC200DCEGPSAutoGPSEn(ec200_dce_t *dce, uint8_t en)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char   cmdStr[40];

    modem_cmd_t txCmd = {
        .cmd     = cmdStr,
        .handler = gnss_default_resp_handler
    };

    if (dce == NULL)
    {
        return retVal;
    }

    snprintf(cmdStr, sizeof(cmdStr),
             "AT+QGPSCFG=\"autogps\",%d\r",
             en ? 1 : 0);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t EC200DCEGPSSetNMEAType(ec200_dce_t *dce, ec200_nmea_type_t types)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char   cmdStr[48];

    modem_cmd_t txCmd = {
        .cmd     = cmdStr,
        .handler = gnss_default_resp_handler
    };

    if (dce == NULL)
    {
        return retVal;
    }

    snprintf(cmdStr, sizeof(cmdStr),
             "AT+QGPSCFG=\"gpsnmeatype\",%d\r",
             (int)types);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}

int8_t EC200DCEGPSDisableNMEA(ec200_dce_t *dce)
{
    /* Convenience: set gpsnmeatype = 0 to suppress all NMEA sentences. */
    return EC200DCEGPSSetNMEAType(dce, EC200_NMEA_NONE);
}

int8_t EC200DCEGPSGetNMEASentence(ec200_dce_t *dce, const char *type)
{
    int8_t retVal = APP_ERR_INV_ARG;
    char   cmdStr[32];

    /*
     * The NMEA response is delivered as a URC / raw data line.
     * We set noResp=1 so the caller processes it via the registered
     * DTE Rx event handler.
     */
    modem_cmd_t txCmd = {
        .cmd    = cmdStr,
        .noResp = 1
    };

    if ((dce == NULL) || (type == NULL))
    {
        return retVal;
    }

    snprintf(cmdStr, sizeof(cmdStr), "AT+QGPSGNMEA=\"%s\"\r", type);

    retVal = ModemDTESendWait(dce->super.dte, &txCmd, MODEM_CMD_TO_DEF);

    return retVal;
}
