#include "stealthim/stealthim.h"

#ifdef STEALTHIM_NETWORK_WIN32

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

stealthim_status_t stealthim_network_init() {
    WSADATA wsa;
    return (WSAStartup(MAKEWORD(2,2), &wsa) == 0) ? STEALTHIM_OK : STEALTHIM_ERR;
}

static int send_all(SOCKET sock, const char* buf, int len) {
    int sent = 0;
    while (sent < len) {
        int n = send(sock, buf + sent, len - sent, 0);
        if (n <= 0) return STEALTHIM_ERR;
        sent += n;
    }
    return STEALTHIM_OK;
}

// 辅助：从响应头解析 Content-Length
static int parse_content_length(const char* headers) {
    const char* p = strcasestr(headers, "Content-Length:");
    if (p) {
        return atoi(p + 15);
    }
    return -1;
}

// 辅助：检查是否 chunked
static int is_chunked(const char* headers) {
    return (strcasestr(headers, "Transfer-Encoding: chunked") != NULL);
}

stealthim_status_t stealthim_http_request(
    const char* method,
    const char* host,
    int port,
    const char* path,
    const char** headers,
    const char* body,
    char* response,
    int maxlen
) {
    SOCKET sock;
    struct sockaddr_in server;
    struct hostent *he;
    char request[2048];
    int total = 0;

    stealthim_log_debug("HTTP Request: %s %s:%d%s", method, host, port, path);

    he = gethostbyname(host);
    if (!he) return STEALTHIM_ERR;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return STEALTHIM_ERR;

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    memcpy(&server.sin_addr, he->h_addr, he->h_length);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        closesocket(sock);
        return STEALTHIM_ERR;
    }

    // 构造请求行
    int n = snprintf(request, sizeof(request), "%s %s HTTP/1.1\r\nHost: %s\r\n", method, path, host);

    // 添加自定义 headers
    if (headers) {
        for (int i = 0; headers[i]; i++) {
            n += snprintf(request + n, sizeof(request) - n, "%s\r\n", headers[i]);
        }
    }

    // 如果有 body，就添加 Content-Length
    if (body) {
        n += snprintf(request + n, sizeof(request) - n, "Content-Length: %zu\r\n", strlen(body));
    }

    // 默认保持连接，除非用户要求 close
    n += snprintf(request + n, sizeof(request) - n, "Connection: close\r\n");

    // 结束 header
    n += snprintf(request + n, sizeof(request) - n, "\r\n");

    // 发送请求头
    if (send_all(sock, request, n) != STEALTHIM_OK) {
        closesocket(sock);
        return STEALTHIM_ERR;
    }

    // 发送 body
    if (body) {
        if (send_all(sock, body, strlen(body)) != STEALTHIM_OK) {
            closesocket(sock);
            return STEALTHIM_ERR;
        }
    }

    // 接收响应（先读 header）
    int recv_size;
    int header_done = 0;
    char *header_end = NULL;
    while (!header_done && (recv_size = recv(sock, response + total, maxlen - total - 1, 0)) > 0) {
        total += recv_size;
        response[total] = '\0';
        header_end = strstr(response, "\r\n\r\n");
        if (header_end) header_done = 1;
    }

    if (!header_end) {
        closesocket(sock);
        return STEALTHIM_ERR; // 没收到完整头部
    }

    int header_len = (header_end - response) + 4;
    char* body_ptr = response + header_len;
    int body_bytes = total - header_len;

    // 判断响应体读取方式
    int content_length = parse_content_length(response);
    int chunked = is_chunked(response);

    if (content_length > 0) {
        // 读取固定长度
        while (body_bytes < content_length) {
            recv_size = recv(sock, body_ptr + body_bytes, maxlen - header_len - body_bytes - 1, 0);
            if (recv_size <= 0) break;
            body_bytes += recv_size;
        }
        body_ptr[body_bytes] = '\0';
        total = header_len + body_bytes;
    } else if (chunked) {
        // 读取所有 chunk 数据
        int received = body_bytes;
        while ((recv_size = recv(sock, body_ptr + received, maxlen - header_len - received - 1, 0)) > 0) {
            received += recv_size;
        }
        // 解码 chunked
        int decoded = decode_chunked(body_ptr, received, body_ptr, maxlen - header_len);
        if (decoded >= 0) total = header_len + decoded;
        else total = header_len + received; // fallback
    } else {
        // fallback: 一直读到 close
        while ((recv_size = recv(sock, body_ptr + body_bytes, maxlen - header_len - body_bytes - 1, 0)) > 0) {
            body_bytes += recv_size;
        }
        body_ptr[body_bytes] = '\0';
        total = header_len + body_bytes;
    }

    closesocket(sock);
    return (total > 0) ? STEALTHIM_OK : STEALTHIM_ERR;
}

void stealthim_network_cleanup() {
    WSACleanup();
}

#ifdef __cplusplus
}
#endif

#endif
