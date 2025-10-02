#include "stealthim/hal/network.h"

#ifdef STEALTHIM_NETWORK_POSIX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "stealthim/hal/logging.h"

stealthim_status_t stealthim_network_init() {
    // Linux 不需要初始化 socket
    return STEALTHIM_OK;
}

static int send_all(int sock, const char* buf, int len) {
    int sent = 0;
    while (sent < len) {
        int n = send(sock, buf + sent, len - sent, 0);
        if (n <= 0) return STEALTHIM_ERR;
        sent += n;
    }
    return STEALTHIM_OK;
}

static int parse_content_length(const char* headers) {
    const char* p = strcasestr(headers, "Content-Length:");
    if (p) {
        return atoi(p + 15);
    }
    return -1;
}

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
    int sock;
    struct addrinfo hints, *res;
    char port_str[6];
    char request[2048];
    int total = 0;

    snprintf(port_str, sizeof(port_str), "%d", port);

    stealthim_log_debug("HTTP Request: %s %s:%d%s", method, host, port, path);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        stealthim_log_error("getaddrinfo failed");
        return STEALTHIM_ERR;
    }

    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        return STEALTHIM_ERR;
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        close(sock);
        freeaddrinfo(res);
        return STEALTHIM_ERR;
    }
    freeaddrinfo(res);

    // 构造请求行
    int n = snprintf(request, sizeof(request), "%s %s HTTP/1.1\r\nHost: %s\r\n", method, path, host);

    // 添加自定义 headers
    if (headers) {
        for (int i = 0; headers[i]; i++) {
            n += snprintf(request + n, sizeof(request) - n, "%s\r\n", headers[i]);
        }
    }

    // 如果有 body，添加 Content-Length
    if (body) {
        n += snprintf(request + n, sizeof(request) - n, "Content-Length: %zu\r\n", strlen(body));
    }

    // 默认使用 Connection: close
    n += snprintf(request + n, sizeof(request) - n, "Connection: close\r\n");

    // 结束 header
    n += snprintf(request + n, sizeof(request) - n, "\r\n");

    if (send_all(sock, request, n) != STEALTHIM_OK) {
        close(sock);
        return STEALTHIM_ERR;
    }

    if (body) {
        if (send_all(sock, body, strlen(body)) != STEALTHIM_OK) {
            close(sock);
            return STEALTHIM_ERR;
        }
    }

    // 先读取响应头
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
        close(sock);
        return STEALTHIM_ERR;
    }

    int header_len = (header_end - response) + 4;
    char* body_ptr = response + header_len;
    int body_bytes = total - header_len;

    int content_length = parse_content_length(response);
    int chunked = is_chunked(response);

    if (content_length > 0) {
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
        while ((recv_size = recv(sock, body_ptr + body_bytes, maxlen - header_len - body_bytes - 1, 0)) > 0) {
            body_bytes += recv_size;
        }
        body_ptr[body_bytes] = '\0';
        total = header_len + body_bytes;
    }

    close(sock);
    return (total > 0) ? STEALTHIM_OK : STEALTHIM_ERR;
}

stealthim_status_t stealthim_network_cleanup() {
    return STEALTHIM_OK;
}

#endif
