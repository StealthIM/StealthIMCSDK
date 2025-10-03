#include <stdio.h>
#include <string.h>

int test_http();
int test_ws();
int test_loop();
int test_future();
int test_task();

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 2) {
        printf("Usage: %s <testname>\n", argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "http") == 0) return test_http();
    if (strcmp(argv[1], "ws") == 0) return test_ws();
    if (strcmp(argv[1], "loop") == 0) return test_loop();
    if (strcmp(argv[1], "future") == 0) return test_future();
    if (strcmp(argv[1], "task") == 0) return test_task();

    printf("Unknown test: %s\n", argv[1]);
    return 1;
}
