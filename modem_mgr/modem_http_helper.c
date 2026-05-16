#include "modem_http_helper.h"
#include <stdio.h>
#include <string.h>

static const char * const hdr_val_strs[] = {
    "text/plain",
    "application/json",
    "application/javascript",
    "application/xml",
    "application/x-www-form-urlencoded",
    "keep-alive",
    "close",
    "QUECTEL_MODULE"
};

int8_t ModemHTTPHelperGetHeaderStr(http_hdr_type_t hdrType, http_hdr_val_t hdrVal, char *outStr, uint16_t maxLen)
{
    if (outStr == NULL || maxLen == 0 || hdrVal >= HTTP_HDR_VAL_MAX) {
        return -1;
    }

    const char *valStr = hdr_val_strs[hdrVal];

    switch (hdrType) {
        case HTTP_HDR_CONTENT_TYPE:
            snprintf(outStr, maxLen, STATIC_TEXT_MACRO_CONTENT_TYPE, valStr);
            break;
        case HTTP_HDR_ACCEPT:
            snprintf(outStr, maxLen, STATIC_TEXT_MACRO_ACCEPT, valStr);
            break;
        case HTTP_HDR_CONNECTION:
            snprintf(outStr, maxLen, STATIC_TEXT_MACRO_CONNECTION, valStr);
            break;
        case HTTP_HDR_USER_AGENT:
            snprintf(outStr, maxLen, STATIC_TEXT_MACRO_USER_AGENT, valStr);
            break;
        default:
            return -1;
    }
    return 0;
}
