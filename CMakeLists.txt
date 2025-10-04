cmake_minimum_required(VERSION 3.10)
project(stealthim C)

link_libraries(ssl crypto)

# ========== 配置开关 ==========
option(ENABLE_ASAN "Enable AddressSanitizer for tests" OFF)

# ========== 源与库 ==========
add_library(stealthim STATIC
        src/hal/tools.c
        src/hal/net/sync/network/win32.c
        src/hal/logging.c
        include/stealthim/hal/tools.h
        include/stealthim/stealthim.h
        src/hal/net/sync/network/posix.c
        src/hal/net/sync/network/common.c
        include/stealthim/hal/net/tls.h
        src/hal/net/tls/openssl.c
        include/stealthim/hal/net/sync/websocket.h
        src/hal/net/sync/websocket/none.c
        src/hal/net/sync/websocket/win32.c
        src/hal/net/sync/websocket/posix.c
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


# ========== 库导出/可执行文件/测试 ==========
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

# ========== AddressSanitizer 支持 ==========
if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled for build")
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling AddressSanitizer for GCC/Clang")
        target_compile_options(stealthim_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer -g -O0)
        target_link_options(stealthim_tests PRIVATE -fsanitize=address)
    else()
        message(WARNING "ASan requested but compiler is not GCC/Clang. Ignoring.")
    endif()
endif()

# 设置一些常用环境变量（ASAN/LSAN），可以根据需要修改或增加 suppress 文件

# 为每个测试使用包装命令，加入环境变量与 valgrind 前缀（如果有）
add_test(NAME test_http COMMAND stealthim_tests http)
add_test(NAME test_ws   COMMAND stealthim_tests ws)
add_test(NAME test_loop COMMAND stealthim_tests loop)
add_test(NAME test_future COMMAND stealthim_tests future)
add_test(NAME test_task COMMAND stealthim_tests task)

# END of CMakeLists.txt
