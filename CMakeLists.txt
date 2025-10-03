cmake_minimum_required(VERSION 3.10)
project(stealthim C)

link_libraries(ssl crypto)

add_library(stealthim STATIC
        src/hal/tools.c
        src/hal/network/win32.c
        src/hal/logging.c
        include/stealthim/hal/tools.h
        include/stealthim/stealthim.h
        src/hal/network/posix.c
        src/hal/network/common.c
        include/stealthim/hal/tls.h
        src/hal/tls/openssl.c
        include/stealthim/hal/websocket.h
        src/hal/websocket/none.c
        src/hal/websocket/win32.c
        src/hal/websocket/posix.c
        include/stealthim/hal/loop.h
        src/hal/loop/windows.c
)

if(WIN32)
    target_link_libraries(stealthim PRIVATE ws2_32 crypt32)
endif()

target_include_directories(stealthim PUBLIC include)

enable_testing()

add_executable(stealthim_tests
        test/main.c
        test/test_http.c
        test/test_ws.c
        test/test_loop.c
)
target_link_libraries(stealthim_tests stealthim)

add_test(NAME test_http COMMAND stealthim_tests http)
add_test(NAME test_ws COMMAND stealthim_tests ws)
add_test(NAME test_loop COMMAND stealthim_tests loop)
