cmake_minimum_required(VERSION 3.20)
project(MyRTOS C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(MRT_BUILD_TESTS "Build MyRTOS host tests" ON)

add_library(myrtos_kernel STATIC
    src/kernel/mrt_list.c
    src/kernel/mrt_priority.c
    src/portable/mock/mrt_port_mock.c
)
target_include_directories(myrtos_kernel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MRT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
