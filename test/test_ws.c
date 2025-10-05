#include <stdio.h>
#include <string.h>

#include "../include/stim/hal/net/sync/websocket.h"
#include "../include/stim/hal/net/sync/network.h"

int test_ws() {
    if (stim_network_init() != stim_OK) {
        printf("WSA init failed\n");
        return -1;
    }

    stim_ws_init();

    stim_ws_t* ws = stim_ws_connect("wss://echo.websocket.org");
    if (!ws) {
        printf("WebSocket connect failed\n");
        stim_network_cleanup();
        return -1;
    }

    printf("Connected to WS server!\n");

    const char* msg = "Hello, WebSocket!";
    if (stim_ws_send(ws, msg, (int)strlen(msg), 1) != stim_WS_OK) {
        printf("Error sending message\n");
        stim_ws_close(ws);
        stim_network_cleanup();
        return -1;
    }

    char buffer[1024];
    int is_text;
    int n = stim_ws_recv(ws, buffer, sizeof(buffer), &is_text);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Received (%s): %s\n", is_text ? "text" : "bin", buffer);
    }
    n = stim_ws_recv(ws, buffer, sizeof(buffer), &is_text);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Received (%s): %s\n", is_text ? "text" : "bin", buffer);
    }

    int i = 0;
    for (; i<10;i++) {
        // Generate random messages
        char message[63];
        snprintf(message, sizeof(message), "Message %d from client", i);
        if (stim_ws_send(ws, message, (int)strlen(message), 1) != stim_WS_OK) {
            return -1;
        }
        n = stim_ws_recv(ws, buffer, sizeof(buffer), &is_text);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Received (%s): %s\n", is_text ? "text" : "bin", buffer);
        } else {
            return -1;
        }
        if (strcmp(message, buffer) != 0) {
            printf("Mismatch!\n");
            return -1;
        }
    }

    stim_ws_close(ws);
    stim_network_cleanup();
    return 0;
}
