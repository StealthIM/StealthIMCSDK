#include <stdio.h>
#include <string.h>

#include "../include/stealthim/hal/net/websocket.h"
#include "../include/stealthim/hal/net/network.h"

int test_ws() {
    if (stealthim_network_init() != STEALTHIM_OK) {
        printf("WSA init failed\n");
        return -1;
    }

    stealthim_ws_init();

    stealthim_ws_t* ws = stealthim_ws_connect("wss://echo.websocket.org");
    if (!ws) {
        printf("WebSocket connect failed\n");
        stealthim_network_cleanup();
        return -1;
    }

    printf("Connected to WS server!\n");

    const char* msg = "Hello, WebSocket!";
    if (stealthim_ws_send(ws, msg, (int)strlen(msg), 1) != STEALTHIM_WS_OK) {
        printf("Error sending message\n");
        stealthim_ws_close(ws);
        stealthim_network_cleanup();
        return -1;
    }

    char buffer[1024];
    int is_text;
    int n = stealthim_ws_recv(ws, buffer, sizeof(buffer), &is_text);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Received (%s): %s\n", is_text ? "text" : "bin", buffer);
    }
    n = stealthim_ws_recv(ws, buffer, sizeof(buffer), &is_text);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Received (%s): %s\n", is_text ? "text" : "bin", buffer);
    }

    int i = 0;
    for (; i<10;i++) {
        // Generate random messages
        char message[63];
        snprintf(message, sizeof(message), "Message %d from client", i);
        if (stealthim_ws_send(ws, message, (int)strlen(message), 1) != STEALTHIM_WS_OK) {
            return -1;
        }
        n = stealthim_ws_recv(ws, buffer, sizeof(buffer), &is_text);
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

    stealthim_ws_close(ws);
    stealthim_network_cleanup();
    return 0;
}
