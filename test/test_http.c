#include <stdio.h>
#include "stim/stim.h"

int test_http() {
    stim_log_debug("Testing http");

    if (stim_network_init() != stim_OK) {
        stim_log_error("Network init failed");
        return -1;
    }
    stim_log_debug("Network inited");

    char buffer[8192];

    // 例子: GET
    const char* headers1[] = { "User-Agent: stim-C/0.1", "Accept: */*", NULL };
    if (stim_http_request("GET", "postman-echo.com", 443, "/get?foo1=bar1&foo2=bar2", headers1, NULL, buffer, sizeof(buffer)) == stim_OK) {
        printf("GET Response:\n%s\n", buffer);
    } else {
        return -1;
    }

    // 例子: POST JSON
    const char* headers2[] = { "User-Agent: stim-C/0.1", "Content-Type: application/json", NULL };
    const char* body = "{ \"msg\": \"hello\" }";
    if (stim_http_request("POST", "postman-echo.com", 443, "/post", headers2, body, buffer, sizeof(buffer)) == stim_OK) {
        printf("POST Response:\n%s\n", buffer);
    } else {
        return -1;
    }

    stim_network_cleanup();
    return 0;
}
