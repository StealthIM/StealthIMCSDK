#include <stdio.h>
#include "stealthim/stealthim.h"

int test_http() {
    stealthim_log_debug("Testing http");

    if (stealthim_network_init() != STEALTHIM_OK) {
        stealthim_log_error("Network init failed");
        return -1;
    }
    stealthim_log_debug("Network inited");

    char buffer[8192];

    // 例子: GET
    const char* headers1[] = { "User-Agent: StealthIM-C/0.1", "Accept: */*", NULL };
    if (stealthim_http_request("GET", "postman-echo.com", 443, "/get?foo1=bar1&foo2=bar2", headers1, NULL, buffer, sizeof(buffer)) == STEALTHIM_OK) {
        printf("GET Response:\n%s\n", buffer);
    } else {
        return -1;
    }

    // 例子: POST JSON
    const char* headers2[] = { "User-Agent: StealthIM-C/0.1", "Content-Type: application/json", NULL };
    const char* body = "{ \"msg\": \"hello\" }";
    if (stealthim_http_request("POST", "postman-echo.com", 80, "/post", headers2, body, buffer, sizeof(buffer)) == STEALTHIM_OK) {
        printf("POST Response:\n%s\n", buffer);
    } else {
        return -1;
    }

    stealthim_network_cleanup();
    return 0;
}
