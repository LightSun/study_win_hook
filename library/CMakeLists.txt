cmake_minimum_required(VERSION 3.15)

project(InterceptPro LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(WIN_deps
    kernel32 advapi32 user32
    )
# GLOB_RECURSE GLOB
file(GLOB SRC_ALL *.h
    *.c
    *.cpp
    )

add_library(intercept_s
    ${SRC_ALL}
    )
add_library(intercept SHARED
    ${SRC_ALL}
    )

target_link_libraries(intercept PUBLIC ${WIN_deps})
target_link_libraries(intercept_s PUBLIC ${WIN_deps})

## handler os
set(HANDLER_OS_DIR D:/study/cpp/ENV/handler_os)
include_directories(${HANDLER_OS_DIR}/include)
set(HANDLER_LIB ${HANDLER_OS_DIR}/lib/libhandler_os.a pthread)

## test

add_executable(test_intercept
    test/WinHooker.h test/WinHooker.cpp
    test/WorkManager.h test/WorkManager.cpp
    test/work_config.h
    test/LockH.h
    test/_time.h

    test/utils.h
    test/utils.c
    test/x2y.cpp
    test/test2.cpp
    test/test_input.cpp
    test/test_listen_keyboard.cpp
    )

target_link_libraries(test_intercept PUBLIC intercept_s
    ${WIN_deps}
    ${HANDLER_LIB}
    )
