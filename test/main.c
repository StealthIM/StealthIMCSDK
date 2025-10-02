#include <stdio.h>
#include <string.h>

int test_http();
int test_ws();

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <testname>\n", argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "http") == 0) return test_http();
    if (strcmp(argv[1], "ws") == 0) return test_ws();

    printf("Unknown test: %s\n", argv[1]);
    return 1;
}
