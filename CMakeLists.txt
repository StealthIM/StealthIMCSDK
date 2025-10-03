cmake_minimum_required(VERSION 3.10)
project(stealthim C)

link_libraries(ssl crypto)

add_library(stealthim STATIC
        src/hal/tools.c
        src/hal/web/network/win32.c
        src/hal/logging.c
        include/stealthim/hal/tools.h
        include/stealthim/stealthim.h
        src/hal/web/network/posix.c
        src/hal/web/network/common.c
        include/stealthim/hal/net/tls.h
        src/hal/web/tls/openssl.c
        include/stealthim/hal/net/websocket.h
        src/hal/web/websocket/none.c
        src/hal/web/websocket/win32.c
        src/hal/web/websocket/posix.c
        include/stealthim/hal/async/loop.h
        src/hal/async/loop/windows.c
        src/hal/async/loop/linux.c
        src/hal/async/future.c
        include/stealthim/hal/async/future.h
        include/stealthim/hal/async/task.h
        src/hal/async/task.c
        src/hal/async/loop/linux_select.c
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
        test/test_future.c
        test/tools.h
        test/test_task.c
)
target_link_libraries(stealthim_tests stealthim)

add_test(NAME test_http COMMAND stealthim_tests http)
add_test(NAME test_ws COMMAND stealthim_tests ws)
add_test(NAME test_loop COMMAND stealthim_tests loop)
add_test(NAME test_future COMMAND stealthim_tests future)
add_test(NAME test_task COMMAND stealthim_tests task)
