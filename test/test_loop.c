#include <stdio.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "stealthim/hal/loop.h"

static void socket_cb(loop_t *loop, void *userdata) {
    recv_data_t *data = (recv_data_t*)userdata;
    printf("Socket event: \n%s\n, data length: %lu\n", data->data, data->len);
}

static void timer1_cb(loop_t *loop, void *userdata) {
    printf("Timer1 fired: %s\n", (const char*)userdata);
}

static void timer2_cb(loop_t *loop, void *userdata) {
    printf("Timer2 fired: %s\n", (const char*)userdata);
}

static void timer3_cb(loop_t *loop, void *userdata) {
    int count = *(int*)userdata;
    if (count < 4) {
        printf("Timer3 fired: %d, rescheduling...\n", count);
        (*(int*)userdata)++;
        loop_add_timer(loop, 300, timer3_cb, userdata);
    } else {
        printf("Timer3 fired: %d, done.\n", count);
    }
}

int test_loop() {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in serverAddr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Connect failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    loop_t *loop = loop_create(NULL);
    //
    // loop_add_timer(loop, 500, timer1_cb, "hello 500ms");
    // loop_add_timer(loop, 1500, timer2_cb, "bye 1500ms");
    // int timer3_count = 0;
    // loop_add_timer(loop, 300, timer3_cb, &timer3_count);

    loop_register_handle(loop, (void*)sock, socket_cb, "&sock");

    loop_run(loop);

    printf("Main thread doing other work...\n");
    for (int i = 0; i < 10; i++) {
        Sleep(1000);
        printf(".\n");
        printf("%llu", loop_time_ms(loop));
        if (i==1) {
            char send_buf[] = "Hello, socket!\n";
            if (send(sock, send_buf, sizeof(send_buf)-1, 0) < 0) {
                printf("Send failed\n");
            }
        }
    }
    printf("\n");

    loop_destroy(loop);
    closesocket(sock);
    WSACleanup();
    return 0;
}
