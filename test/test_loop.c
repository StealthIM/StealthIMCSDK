// test_loop_http.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "tools.h"

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET socket_t;
#else
  #include <unistd.h>
  #include <errno.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <time.h>
  #include <fcntl.h>
  typedef int socket_t;
  #ifndef INVALID_SOCKET
    #define INVALID_SOCKET (-1)
  #endif
  #ifndef SOCKET_ERROR
    #define SOCKET_ERROR   (-1)
  #endif
  #define closesocket close
#endif

#include "../include/stealthim/hal/async/loop.h"

static void socket_cb(loop_t *loop, void *userdata) {
    recv_data_t *data = (recv_data_t*)userdata;

    data->data[data->len] = '\0'; // 确保字符串结束

    printf("Received %lu bytes:\n", data->len);
    printf("%s\n", data->data);
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
        stealthim_loop_add_timer(loop, 300, timer3_cb, userdata);
    } else {
        printf("Timer3 fired: %d, done.\n", count);
    }
}


int test_loop() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif

    const char *host = "postman-echo.com";
    const char *path = "/get";

    // 解析域名
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int ret = getaddrinfo(host, "80", &hints, &res);
    if (ret != 0 || !res) {
        fprintf(stderr, "getaddrinfo failed: %s\n",
#ifdef _WIN32
            gai_strerrorA(ret)
#else
            gai_strerror(ret)
#endif
        );
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    // 创建 socket
    socket_t sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) {
        perror("socket failed");
#ifdef _WIN32
        WSACleanup();
#endif
        freeaddrinfo(res);
        return 1;
    }

    if (connect(sock, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
        perror("connect failed");
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        freeaddrinfo(res);
        return 1;
    }
    freeaddrinfo(res);

    // 设置非阻塞
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

    printf("Creating loop\n");

    loop_t *loop = stealthim_loop_create();
    printf("Created loop\n");

    stealthim_loop_register_handle(loop, (void*)(intptr_t)sock, socket_cb, NULL);


    stealthim_loop_add_timer(loop, 500, timer1_cb, "hello 500ms");
    stealthim_loop_add_timer(loop, 1500, timer2_cb, "bye 1500ms");
    int timer3_count = 0;
    stealthim_loop_add_timer(loop, 300, timer3_cb, &timer3_count);

    printf("Run\n");
    stealthim_loop_run(loop);
    printf("Ran\n");

    int i = 0;
    for (; i < 10; i++) {
        sleep_ms(1000);
        printf(".\n");
        if (i == 2) {
            char request[512];
            snprintf(request, sizeof(request),
                     "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
                     path, host);
            send(sock, request, (int)strlen(request), 0);
        }
    }

    stealthim_loop_destroy(loop);

    closesocket(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
