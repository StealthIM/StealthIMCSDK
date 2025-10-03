#include "stealthim/hal/net/websocket.h"
#ifdef STEALTHIM_WS_NONE

#include <stdlib.h>

struct stealthim_ws_t { int dummy; };

stealthim_ws_t* stealthim_ws_connect(const char* url) {
    (void)url;
    return NULL; // 不支持
}

stealthim_ws_status_t stealthim_ws_send(stealthim_ws_t* ws, const void* data, int len, int is_text) {
    (void)ws; (void)data; (void)len; (void)is_text;
    return STEALTHIM_WS_ERR;
}

int stealthim_ws_recv(stealthim_ws_t* ws, void* buffer, int maxlen, int* is_text) {
    (void)ws; (void)buffer; (void)maxlen; (void)is_text;
    return STEALTHIM_WS_ERR;
}

void stealthim_ws_close(stealthim_ws_t* ws) {
    (void)ws;
}

#endif
