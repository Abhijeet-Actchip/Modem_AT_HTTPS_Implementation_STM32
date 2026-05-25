/**
 * @file modem_ec200u_gnss.h
 * @author Abhijeet (abhijeet@actchip.com)
 * @brief Quectel EC200U / EG912U-GL GNSS AT command API.
 *        Based on: Quectel EC200U Series & EG912U-GL GNSS Application Note v1.2
 * @date 2026-05-25
 *
 * @copyright Copyright (c) Actchip Pvt. Ltd. 2026
 *
 * ---- Typical GNSS usage flow ----
 *  1. EC200DCEGPSConfig()       - configure GNSS (outport, nmeasrc, constellation, NMEA types)
 *  2. EC200DCEGPSTurnOn()       - turn on GNSS engine  (AT+QGPS=1)
 *  3. Wait for a fix (typically 30-90 s cold-start, sky-view required)
 *  4. EC200DCEGPSGetLocation()  - read location        (AT+QGPSLOC=2)
 *  5. EC200DCEGPSTurnOff()      - turn off GNSS engine (AT+QGPSEND)
 *
 * Optional:
 *  - EC200DCEGPSAutoGPSEn()     - enable/disable GNSS auto-start on power-up
 *  - EC200DCEGPSSetNMEAType()   - configure which NMEA sentences are output
 *  - EC200DCEGPSDisableNMEA()   - suppress all NMEA output (gpsnmeatype=0)
 *  - EC200DCEGPSGetNMEASentence()- read a single NMEA sentence on-demand
 */

#ifndef __MODEM_EC200U_GNSS_H__
#define __MODEM_EC200U_GNSS_H__  1

/* Forward reference: ec200_dce_t is defined in modem_ec200u.h which includes
 * this file — include modem_dce.h here to avoid a circular dependency. */
#include "modem_dce.h"

/* ec200_dce_t wraps modem_dce_t; it is defined in modem_ec200u.h.
 * Declare it here so this header can be self-contained. */
typedef struct _ec200_dce_t ec200_dce_t;

/* -------------------------------------------------------------------------
 * Timeout constants
 * ------------------------------------------------------------------------- */

/** Max time (ms) to wait for AT+QGPSLOC response once fix is expected. */
#define EC200_CMD_TO_QGPSLOC        (5000)

/** Max time (ms) to wait for AT+QGPS (turn-on/off). */
#define EC200_CMD_TO_QGPS           (2000)

/** Max time (ms) to wait for AT+QGPSEND. */
#define EC200_CMD_TO_QGPSEND        (2000)

/* -------------------------------------------------------------------------
 * GNSS constellation configuration  (AT+QGPSCFG="gnssconfig",<val>)
 * As per GNSS Application Note v1.2, Chapter 2.3.1.8
 * NOTE: Value 5 meaning depends on module variant:
 *   EC200U-CN  -> GPS + BDS
 *   EC200U-AU, EC200U-EU, EG912U-GL -> GPS + BDS + Galileo
 * ------------------------------------------------------------------------- */
typedef enum _ec200_gnss_config_t {
    EC200_GNSS_CFG_GPS_ONLY                = 0, /*!< GPS only                                       */
    EC200_GNSS_CFG_GPS_GLONASS_GALILEO     = 3, /*!< GPS + GLONASS + Galileo                        */
    EC200_GNSS_CFG_GPS_GLONASS             = 4, /*!< GPS + GLONASS                                  */
    EC200_GNSS_CFG_GPS_BDS                 = 5, /*!< GPS + BDS (CN) / GPS + BDS + Galileo (AU/EU/GL)*/
    EC200_GNSS_CFG_GPS_GALILEO             = 6, /*!< GPS + Galileo                                  */
    EC200_GNSS_CFG_BDS_ONLY                = 7, /*!< BDS only                                       */
} ec200_gnss_config_t;

/* -------------------------------------------------------------------------
 * GPS NMEA type bitmask  (AT+QGPSCFG="gpsnmeatype",<bitmask>)
 * As per GNSS Application Note v1.2, Chapter 2.3.1.3
 *
 * OR together the values you want to enable, e.g.:
 *   EC200_NMEA_GGA | EC200_NMEA_RMC
 * Pass EC200_NMEA_NONE (0) to disable all GPS NMEA sentences.
 * Value 31 (0x1F) enables all five GPS sentence types.
 * NOTE: GLL is NOT a supported GPS NMEA type for this module.
 * ------------------------------------------------------------------------- */
typedef enum _ec200_nmea_type_t {
    EC200_NMEA_NONE = 0x00, /*!< Disable all GPS NMEA sentences (gpsnmeatype=0) */
    EC200_NMEA_GGA  = 0x01, /*!< GPGGA – fix data (pos quality, #sats, alt)     */
    EC200_NMEA_RMC  = 0x02, /*!< GPRMC – recommended minimum specific GNSS data */
    EC200_NMEA_GSV  = 0x04, /*!< GPGSV – satellites in view                     */
    EC200_NMEA_GSA  = 0x08, /*!< GPGSA – DOP and active satellites              */
    EC200_NMEA_VTG  = 0x10, /*!< GPVTG – course over ground and ground speed    */
    EC200_NMEA_ALL  = 0x1F, /*!< All five GPS NMEA sentence types               */
} ec200_nmea_type_t;

/* -------------------------------------------------------------------------
 * GNSS NMEA output port  (AT+QGPSCFG="outport","<port>")
 * As per GNSS Application Note v1.2, Chapter 2.3.1.1
 * ------------------------------------------------------------------------- */
typedef enum _ec200_gnss_outport_t {
    EC200_GNSS_OUTPORT_NONE     = 0, /*!< "none"     – close NMEA sentence output     */
    EC200_GNSS_OUTPORT_UART1    = 1, /*!< "uart1"    – output via UART1 port          */
    EC200_GNSS_OUTPORT_UART2    = 2, /*!< "uart2"    – output via UART2 port          */
    EC200_GNSS_OUTPORT_USBAT    = 3, /*!< "usbat"    – output via USB AT port         */
    EC200_GNSS_OUTPORT_USBMODEM = 4, /*!< "usbmodem" – output via USB Modem port      */
    EC200_GNSS_OUTPORT_USBNMEA  = 5, /*!< "usbnmea"  – output via USB NMEA port (default) */
} ec200_gnss_outport_t;

/* -------------------------------------------------------------------------
 * GLONASS NMEA type bitmask  (AT+QGPSCFG="glonassnmeatype",<bitmask>)
 * As per GNSS Application Note v1.2, Chapter 2.3.1.4
 * Takes effect after reboot; configuration saved automatically.
 * ------------------------------------------------------------------------- */
typedef enum _ec200_glonass_nmea_type_t {
    EC200_GLONASS_NMEA_NONE = 0x00, /*!< Disable GLONASS NMEA output */
    EC200_GLONASS_NMEA_GSV  = 0x01, /*!< GLGSV – GLONASS satellites in view */
} ec200_glonass_nmea_type_t;

/* -------------------------------------------------------------------------
 * Galileo NMEA type bitmask  (AT+QGPSCFG="galileonmeatype",<bitmask>)
 * As per GNSS Application Note v1.2, Chapter 2.3.1.5
 * Takes effect after reboot; configuration saved automatically.
 * ------------------------------------------------------------------------- */
typedef enum _ec200_galileo_nmea_type_t {
    EC200_GALILEO_NMEA_NONE = 0x00, /*!< Disable Galileo NMEA output */
    EC200_GALILEO_NMEA_GSV  = 0x01, /*!< GAGSV – Galileo satellites in view */
} ec200_galileo_nmea_type_t;

/* -------------------------------------------------------------------------
 * BDS (BeiDou) NMEA type bitmask  (AT+QGPSCFG="beidounmeatype",<bitmask>)
 * As per GNSS Application Note v1.2, Chapter 2.3.1.6
 * OR together the values you want to enable.
 * Value 31 (0x1F) enables all five BDS NMEA sentence types.
 * Takes effect after reboot; configuration saved automatically.
 * ------------------------------------------------------------------------- */
typedef enum _ec200_bds_nmea_type_t {
    EC200_BDS_NMEA_NONE = 0x00, /*!< Disable all BDS NMEA sentences */
    EC200_BDS_NMEA_GSA  = 0x01, /*!< PQGSA – DOP and active satellites */
    EC200_BDS_NMEA_GSV  = 0x02, /*!< PQGSV – satellites in view        */
    EC200_BDS_NMEA_GGA  = 0x04, /*!< PQGGA – fix data                  */
    EC200_BDS_NMEA_RMC  = 0x08, /*!< PQRMC – recommended minimum data  */
    EC200_BDS_NMEA_VTG  = 0x10, /*!< PQVTG – course over ground        */
    EC200_BDS_NMEA_ALL  = 0x1F, /*!< All five BDS NMEA sentence types  */
} ec200_bds_nmea_type_t;

/* -------------------------------------------------------------------------
 * Multi-constellation NMEA type bitmask  (AT+QGPSCFG="gnssnmeatype",<bitmask>)
 * As per GNSS Application Note v1.2, Chapter 2.3.1.7
 * OR together the values you want to enable.
 * Value 15 (0x0F) enables all four multi-constellation sentence types.
 * Takes effect after reboot; configuration saved automatically.
 * ------------------------------------------------------------------------- */
typedef enum _ec200_gnss_nmea_type_t {
    EC200_GNSS_NMEA_NONE = 0x00, /*!< Disable all multi-constellation NMEA sentences */
    EC200_GNSS_NMEA_GGA  = 0x01, /*!< GNGGA – fix data                               */
    EC200_GNSS_NMEA_RMC  = 0x02, /*!< GNRMC – recommended minimum data               */
    EC200_GNSS_NMEA_GSA  = 0x04, /*!< GNGSA – DOP and active satellites              */
    EC200_GNSS_NMEA_VTG  = 0x08, /*!< GNVTG – course over ground                     */
    EC200_GNSS_NMEA_ALL  = 0x0F, /*!< All four multi-constellation sentence types    */
} ec200_gnss_nmea_type_t;

/* -------------------------------------------------------------------------
 * BDS NMEA sentence prefix format  (AT+QGPSCFG="beidounmeaformat",<val>)
 * As per GNSS Application Note v1.2, Chapter 2.3.1.10
 * Takes effect after reboot; configuration saved automatically.
 * NOTE: When PQ format (0) is used, PQGSV contains NMEA system ID at end.
 * ------------------------------------------------------------------------- */
typedef enum _ec200_bds_nmea_format_t {
    EC200_BDS_NMEA_FMT_PQ = 0, /*!< Prefix "PQ" (default) */
    EC200_BDS_NMEA_FMT_GB = 1, /*!< Prefix "GB"           */
    EC200_BDS_NMEA_FMT_BD = 2, /*!< Prefix "BD"           */
} ec200_bds_nmea_format_t;

/* -------------------------------------------------------------------------
 * AP Flash quick hot-start mode  (AT+QGPSCFG="apflash",<val>)
 * As per GNSS Application Note v1.2, Chapter 2.3.1.11
 * Takes effect immediately; configuration NOT saved.
 * ------------------------------------------------------------------------- */
typedef enum _ec200_gnss_apflash_t {
    EC200_GNSS_APFLASH_DISABLE = 0, /*!< Disable AP Flash quick hot start */
    EC200_GNSS_APFLASH_ENABLE  = 1, /*!< Enable AP Flash quick hot start  */
} ec200_gnss_apflash_t;

/* -------------------------------------------------------------------------
 * GNSS assistance data delete type  (AT+QGPSDEL=<delete_type>)
 * As per GNSS Application Note v1.2, Chapter 2.3.2
 * Command can only be executed when GNSS is turned on.
 * ------------------------------------------------------------------------- */
typedef enum _ec200_gnss_del_type_t {
    EC200_GNSS_DEL_ALL  = 0, /*!< Delete all assistance data  → enforce cold start  */
    EC200_GNSS_DEL_NONE = 1, /*!< Delete nothing              → perform hot start    */
    EC200_GNSS_DEL_SOME = 2, /*!< Delete some related data    → perform warm start   */
} ec200_gnss_del_type_t;

/* -------------------------------------------------------------------------
 * AGPS enable/disable  (AT+QAGPS=<AGPS_mode>)
 * As per GNSS Application Note v1.2, Chapter 2.3.7
 * Takes effect immediately; configuration saved automatically.
 * ------------------------------------------------------------------------- */
typedef enum _ec200_agps_mode_t {
    EC200_AGPS_DISABLE = 0, /*!< Disable AGPS feature */
    EC200_AGPS_ENABLE  = 1, /*!< Enable AGPS feature  */
} ec200_agps_mode_t;

/* -------------------------------------------------------------------------
 * GNSS CME error codes  (Chapter 4 – Summary of Error Codes)
 * Returned as +CME ERROR: <errcode> on GNSS command failure.
 * ------------------------------------------------------------------------- */
#define EC200_GNSS_ERR_INVALID_PARAM      501  /*!< Invalid parameter(s)              */
#define EC200_GNSS_ERR_NOT_SUPPORTED      502  /*!< Operation not supported           */
#define EC200_GNSS_ERR_SUBSYSTEM_BUSY     503  /*!< GNSS subsystem busy               */
#define EC200_GNSS_ERR_SESSION_ONGOING    504  /*!< Session is ongoing                */
#define EC200_GNSS_ERR_SESSION_NOT_ACTIVE 505  /*!< Session not active (GNSS off)     */
#define EC200_GNSS_ERR_TIMEOUT            506  /*!< Operation timeout                 */
#define EC200_GNSS_ERR_NOT_ENABLED        507  /*!< Function not enabled (nmeasrc=0)  */
#define EC200_GNSS_ERR_TIME_INFO_ERR      508  /*!< Time information error            */
#define EC200_GNSS_ERR_VALIDITY_OOR       512  /*!< Validity time is out of range     */
#define EC200_GNSS_ERR_INTERNAL_RESOURCE  513  /*!< Internal resource error           */
#define EC200_GNSS_ERR_GNSS_LOCKED        514  /*!< GNSS locked                       */
#define EC200_GNSS_ERR_END_BY_E911        515  /*!< End by E911                       */
#define EC200_GNSS_ERR_NOT_FIXED          516  /*!< Not fixed now                     */
#define EC200_GNSS_ERR_CMUX_NOT_OPEN      517  /*!< CMUX port is not opened           */
#define EC200_GNSS_ERR_UNKNOWN            549  /*!< Unknown error                     */

/* -------------------------------------------------------------------------
 * QGPSLOC mode  (AT+QGPSLOC=<mode>)
 * As per GNSS Application Note v1.2, Chapter 2.3.5
 * ------------------------------------------------------------------------- */
typedef enum _ec200_gpsloc_mode_t {
    EC200_GPSLOC_MODE_0 = 0, /*!< ddmm.mmmmN/S, dddmm.mmmmE/W  (4 decimal places)   */
    EC200_GPSLOC_MODE_1 = 1, /*!< ddmm.mmmmmm,N/S, dddmm.mmmmmm,E/W (6 dec places)  */
    EC200_GPSLOC_MODE_2 = 2, /*!< (-)dd.ddddd, (-)ddd.ddddd — decimal degrees        */
} ec200_gpsloc_mode_t;

/* -------------------------------------------------------------------------
 * Location data structure populated by EC200DCEGPSGetLocation()
 * ------------------------------------------------------------------------- */
typedef struct _ec200_gnss_location_t {
    char    utc[12];        /*!< UTC time string  "HHMMSS.SSS"          */
    float   latitude;       /*!< Latitude  in decimal degrees (positive = N) */
    float   longitude;      /*!< Longitude in decimal degrees (positive = E) */
    float   hdop;           /*!< Horizontal dilution of precision (from GPGGA) */
    float   altitude;       /*!< Altitude above sea level, metres (from GPGGA) */
    uint8_t fixType;        /*!< GNSS positioning mode (from GPGSA): 2=2D, 3=3D */
    float   cog;            /*!< Course over ground, degrees true (from GPVTG) */
    float   speedKmh;       /*!< Speed over ground km/h (from GPVTG)           */
    float   speedKnots;     /*!< Speed over ground (knots)               */
    char    date[8];        /*!< Date string "DDMMYY"                    */
    uint8_t numSats;        /*!< Number of satellites used               */
} ec200_gnss_location_t;

/* -------------------------------------------------------------------------
 * GNSS configuration bundle (passed to EC200DCEGPSConfig())
 * ------------------------------------------------------------------------- */
typedef struct _ec200_gnss_cfg_t {
    ec200_gnss_outport_t outport;       /*!< NMEA output port            */
    uint8_t              nmeasrcEn;     /*!< 1 = enable NMEA source      */
    ec200_nmea_type_t    nmeatype;      /*!< GPS NMEA sentence bitmask   */
    ec200_gnss_config_t  gnssConfig;    /*!< Constellation configuration */
} ec200_gnss_cfg_t;

/* -------------------------------------------------------------------------
 * API prototypes
 * ------------------------------------------------------------------------- */

/**
 * @brief  Configure GNSS parameters (outport, nmeasrc, nmeatype, constellation).
 *         Sends multiple AT+QGPSCFG sub-commands in sequence.
 *         A module reboot may be required for some settings to take effect.
 *
 * @param  dce      Pointer to EC200 DCE object.
 * @param  cfg      Pointer to GNSS configuration struct.
 * @return APP_ERR_NONE on success, else error code.
 */
int8_t EC200DCEGPSConfig(ec200_dce_t *dce, const ec200_gnss_cfg_t *cfg);

/**
 * @brief  Turn ON the GNSS engine.  (AT+QGPS=1)
 *
 * @param  dce      Pointer to EC200 DCE object.
 * @return APP_ERR_NONE on success, else error code.
 */
int8_t EC200DCEGPSTurnOn(ec200_dce_t *dce);

/**
 * @brief  Turn OFF the GNSS engine.  (AT+QGPSEND)
 *
 * @param  dce      Pointer to EC200 DCE object.
 * @return APP_ERR_NONE on success, else error code.
 */
int8_t EC200DCEGPSTurnOff(ec200_dce_t *dce);

/**
 * @brief  Get the current GNSS location.  (AT+QGPSLOC=2)
 *         Parses the response and populates the @p loc structure.
 *         Returns APP_ERR_INV_RESP if no fix yet (CME ERROR: 516).
 *
 * @param  dce      Pointer to EC200 DCE object.
 * @param  mode     Location output format (use EC200_GPSLOC_MODE_DEG for decimal degrees).
 * @param  loc      Pointer to location output struct (must not be NULL).
 * @return APP_ERR_NONE on success, APP_ERR_INV_RESP if not fixed, else error code.
 */
int8_t EC200DCEGPSGetLocation(ec200_dce_t *dce, ec200_gpsloc_mode_t mode,
                              ec200_gnss_location_t *loc);

/**
 * @brief  Enable or disable GNSS auto-start on module power-up.
 *         (AT+QGPSCFG="autogps",<en>)
 *         Changes take effect after reboot.
 *
 * @param  dce      Pointer to EC200 DCE object.
 * @param  en       1 = enable auto-GPS, 0 = disable.
 * @return APP_ERR_NONE on success, else error code.
 */
int8_t EC200DCEGPSAutoGPSEn(ec200_dce_t *dce, uint8_t en);

/**
 * @brief  Configure which GPS NMEA sentence types are output.
 *         (AT+QGPSCFG="gpsnmeatype",<bitmask>)
 *         Use ec200_nmea_type_t values OR-ed together.
 *         Pass EC200_NMEA_NONE (0) to disable all sentences.
 *
 * @param  dce      Pointer to EC200 DCE object.
 * @param  types    Bitmask of ec200_nmea_type_t values.
 * @return APP_ERR_NONE on success, else error code.
 */
int8_t EC200DCEGPSSetNMEAType(ec200_dce_t *dce, ec200_nmea_type_t types);

/**
 * @brief  Disable all GNSS NMEA sentence output.
 *         Convenience wrapper: sets gpsnmeatype = 0.
 *
 * @param  dce      Pointer to EC200 DCE object.
 * @return APP_ERR_NONE on success, else error code.
 */
int8_t EC200DCEGPSDisableNMEA(ec200_dce_t *dce);

/**
 * @brief  Read a specific NMEA sentence on demand.
 *         (AT+QGPSGNMEA="<type>")
 *         Requires nmeasrc to be enabled via EC200DCEGPSConfig().
 *         Result is delivered via the DTE Rx event handler (noResp path).
 *
 * @param  dce      Pointer to EC200 DCE object.
 * @param  type     NMEA sentence type string, e.g. "RMC", "GGA", "GSV".
 * @return APP_ERR_NONE on success, else error code.
 */
int8_t EC200DCEGPSGetNMEASentence(ec200_dce_t *dce, const char *type);

#endif /* __MODEM_EC200U_GNSS_H__ */
