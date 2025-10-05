#include "stim/hal/net/sync/websocket.h"
#ifdef stim_WS_NONE

#include <stdlib.h>

struct stim_ws_t { int dummy; };

stim_ws_t* stim_ws_connect(const char* url) {
    (void)url;
    return NULL; // 不支持
}

stim_ws_status_t stim_ws_send(stim_ws_t* ws, const void* data, int len, int is_text) {
    (void)ws; (void)data; (void)len; (void)is_text;
    return stim_WS_ERR;
}

int stim_ws_recv(stim_ws_t* ws, void* buffer, int maxlen, int* is_text) {
    (void)ws; (void)buffer; (void)maxlen; (void)is_text;
    return stim_WS_ERR;
}

void stim_ws_close(stim_ws_t* ws) {
    (void)ws;
}

#endif
