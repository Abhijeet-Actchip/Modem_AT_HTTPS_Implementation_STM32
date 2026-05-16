#ifndef __MODEM_HTTP_HELPER_H__
#define __MODEM_HTTP_HELPER_H__

#include <stdint.h>

typedef enum _http_hdr_type_t {
    HTTP_HDR_CONTENT_TYPE,
    HTTP_HDR_ACCEPT,
    HTTP_HDR_CONNECTION,
    HTTP_HDR_USER_AGENT,
    HTTP_HDR_MAX
} http_hdr_type_t;

typedef enum _http_hdr_val_t {
    HTTP_HDR_VAL_TEXT_PLAIN,
    HTTP_HDR_VAL_APP_JSON,
    HTTP_HDR_VAL_APP_JS,
    HTTP_HDR_VAL_APP_XML,
    HTTP_HDR_VAL_APP_URLENCODED,
    HTTP_HDR_VAL_KEEP_ALIVE,
    HTTP_HDR_VAL_CLOSE,
    HTTP_HDR_VAL_QUECTEL_MODULE,
    HTTP_HDR_VAL_MAX
} http_hdr_val_t;

#define STATIC_TEXT_MACRO_CONTENT_TYPE "Content-Type: %s\r\n"
#define STATIC_TEXT_MACRO_ACCEPT       "Accept: %s\r\n"
#define STATIC_TEXT_MACRO_CONNECTION   "Connection: %s\r\n"
#define STATIC_TEXT_MACRO_USER_AGENT   "User-Agent: %s\r\n"

int8_t ModemHTTPHelperGetHeaderStr(http_hdr_type_t hdrType, http_hdr_val_t hdrVal, char *outStr, uint16_t maxLen);

#endif /* __MODEM_HTTP_HELPER_H__ */
