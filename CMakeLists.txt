cmake_minimum_required(VERSION 3.20)
project(MyRTOS C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(MRT_BUILD_TESTS "Build MyRTOS host tests" ON)

add_library(myrtos_kernel STATIC
    src/kernel/mrt_assert.c
    src/kernel/mrt_list.c
    src/kernel/mrt_kernel.c
    src/kernel/mrt_heap.c
    src/kernel/mrt_memory_pool.c
    src/kernel/mrt_message_buffer.c
    src/kernel/mrt_event_group.c
    src/kernel/mrt_mutex.c
    src/kernel/mrt_priority.c
    src/kernel/mrt_queue.c
    src/kernel/mrt_semaphore.c
    src/kernel/mrt_stream_buffer.c
    src/kernel/mrt_task.c
    src/kernel/mrt_tickless.c
    src/kernel/mrt_trace.c
    src/kernel/mrt_timer.c
    src/portable/mock/mrt_port_mock.c
)
target_include_directories(myrtos_kernel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MRT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
