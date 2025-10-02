#include "stealthim/hal/tls.h"

#ifdef STEALTHIM_TLS_OPENSSL

#include "stealthim/hal/logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

struct stealthim_tls_ctx {
    SSL* ssl;
    SSL_CTX* ctx;
#ifdef _WIN32
    SOCKET sock;
#else
    int sock;
#endif
};

int stealthim_tls_init(void) {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    return 0;
}

int stealthim_tls_cleanup(void) {
    EVP_cleanup();
    return 0;
}

stealthim_tls_ctx_t* stealthim_tls_create(void) {
    stealthim_tls_ctx_t* t = (stealthim_tls_ctx_t*)calloc(1, sizeof(*t));
    if (!t) return NULL;

    t->ctx = SSL_CTX_new(TLS_client_method());
    if (!t->ctx) {
        free(t);
        return NULL;
    }
    return t;
}

void stealthim_tls_destroy(stealthim_tls_ctx_t* ctx) {
    if (!ctx) return;
    if (ctx->ssl) SSL_free(ctx->ssl);
    if (ctx->ctx) SSL_CTX_free(ctx->ctx);
#ifdef _WIN32
    if (ctx->sock) closesocket(ctx->sock);
#else
    if (ctx->sock >= 0) close(ctx->sock);
#endif
    free(ctx);
}

int stealthim_tls_connect(stealthim_tls_ctx_t* ctx, const char* host, int port) {
    if (!ctx) return -1;

    // 创建普通 TCP socket
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
    ctx->sock = socket(AF_INET, SOCK_STREAM, 0);
#else
    ctx->sock = -1;
#endif

    struct addrinfo hints, *res;
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &res) != 0) return -1;

#ifdef _WIN32
    if (connect(ctx->sock, res->ai_addr, res->ai_addrlen) < 0) { freeaddrinfo(res); return -1; }
#else
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) { freeaddrinfo(res); return -1; }
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) { close(sockfd); freeaddrinfo(res); return -1; }
    ctx->sock = sockfd;
#endif
    freeaddrinfo(res);

    // 创建 SSL 连接
    ctx->ssl = SSL_new(ctx->ctx);
    if (!ctx->ssl) return -1;
    SSL_set_fd(ctx->ssl, ctx->sock);
    SSL_set_tlsext_host_name(ctx->ssl, host);
    if (SSL_connect(ctx->ssl) <= 0) {
        SSL_free(ctx->ssl);
        ctx->ssl = NULL;
        return -1;
    }
    return 0;
}

void stealthim_tls_close(stealthim_tls_ctx_t* ctx) {
    if (!ctx) return;
    if (ctx->ssl) SSL_shutdown(ctx->ssl);
#ifdef _WIN32
    if (ctx->sock) closesocket(ctx->sock);
#else
    if (ctx->sock >= 0) close(ctx->sock);
#endif
    ctx->ssl = NULL;
    ctx->sock = 0;
}

int stealthim_tls_send(stealthim_tls_ctx_t* ctx, const char* buf, int len) {
    if (!ctx || !ctx->ssl) return -1;
    return SSL_write(ctx->ssl, buf, len);
}

int stealthim_tls_recv(stealthim_tls_ctx_t* ctx, char* buf, int maxlen) {
    if (!ctx || !ctx->ssl) return -1;
    return SSL_read(ctx->ssl, buf, maxlen);
}

#endif
