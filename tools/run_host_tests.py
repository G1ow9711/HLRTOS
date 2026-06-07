import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "host-tests"
TESTS = [
    ("test_types_contract", ROOT / "tests" / "unit" / "test_types_contract.c"),
    ("test_config_defaults", ROOT / "tests" / "unit" / "test_config_defaults.c"),
    ("test_list", ROOT / "tests" / "unit" / "test_list.c"),
    ("test_priority_bitmap", ROOT / "tests" / "unit" / "test_priority_bitmap.c"),
    ("test_port_mock", ROOT / "tests" / "port_mock" / "test_port_mock.c"),
    ("test_kernel_tick", ROOT / "tests" / "unit" / "test_kernel_tick.c"),
    ("test_task_create_static", ROOT / "tests" / "sim" / "test_task_create_static.c"),
    ("test_scheduler_start", ROOT / "tests" / "sim" / "test_scheduler_start.c"),
    ("test_scheduler_round_robin", ROOT / "tests" / "sim" / "test_scheduler_round_robin.c"),
    ("test_task_delay", ROOT / "tests" / "sim" / "test_task_delay.c"),
    ("test_task_delay_overflow", ROOT / "tests" / "sim" / "test_task_delay_overflow.c"),
    ("test_queue_create_static", ROOT / "tests" / "unit" / "test_queue_create_static.c"),
    ("test_queue_send_receive", ROOT / "tests" / "unit" / "test_queue_send_receive.c"),
    ("test_queue_variants", ROOT / "tests" / "unit" / "test_queue_variants.c"),
    ("test_queue_isr", ROOT / "tests" / "unit" / "test_queue_isr.c"),
    ("test_queue_task_timeout", ROOT / "tests" / "coupling" / "test_queue_task_timeout.c"),
    ("test_queue_send_wakes_receiver", ROOT / "tests" / "coupling" / "test_queue_send_wakes_receiver.c"),
    ("test_semaphore_create_static", ROOT / "tests" / "unit" / "test_semaphore_create_static.c"),
    ("test_semaphore_take_give", ROOT / "tests" / "unit" / "test_semaphore_take_give.c"),
    ("test_semaphore_task_timeout", ROOT / "tests" / "coupling" / "test_semaphore_task_timeout.c"),
    ("test_semaphore_give_wakes_task", ROOT / "tests" / "coupling" / "test_semaphore_give_wakes_task.c"),
    ("test_semaphore_isr", ROOT / "tests" / "unit" / "test_semaphore_isr.c"),
    ("test_mutex_create_lock", ROOT / "tests" / "unit" / "test_mutex_create_lock.c"),
    ("test_mutex_priority_inheritance", ROOT / "tests" / "coupling" / "test_mutex_priority_inheritance.c"),
    ("test_mutex_recursive", ROOT / "tests" / "unit" / "test_mutex_recursive.c"),
    ("test_event_group_create_bits", ROOT / "tests" / "unit" / "test_event_group_create_bits.c"),
    ("test_event_group_wait_immediate", ROOT / "tests" / "unit" / "test_event_group_wait_immediate.c"),
    ("test_event_group_task_timeout", ROOT / "tests" / "coupling" / "test_event_group_task_timeout.c"),
    ("test_event_group_set_wakes_tasks", ROOT / "tests" / "coupling" / "test_event_group_set_wakes_tasks.c"),
    ("test_event_group_isr", ROOT / "tests" / "unit" / "test_event_group_isr.c"),
    ("test_task_notify_actions", ROOT / "tests" / "unit" / "test_task_notify_actions.c"),
    ("test_task_notify_wait_take", ROOT / "tests" / "coupling" / "test_task_notify_wait_take.c"),
    ("test_task_notify_isr", ROOT / "tests" / "unit" / "test_task_notify_isr.c"),
    ("test_timer_create_static", ROOT / "tests" / "unit" / "test_timer_create_static.c"),
    ("test_timer_control", ROOT / "tests" / "unit" / "test_timer_control.c"),
    ("test_timer_tick_expiry", ROOT / "tests" / "coupling" / "test_timer_tick_expiry.c"),
    ("test_timer_pending_function", ROOT / "tests" / "unit" / "test_timer_pending_function.c"),
    ("test_stream_buffer_create_static", ROOT / "tests" / "unit" / "test_stream_buffer_create_static.c"),
    ("test_stream_buffer_send_receive", ROOT / "tests" / "unit" / "test_stream_buffer_send_receive.c"),
    ("test_stream_buffer_isr", ROOT / "tests" / "unit" / "test_stream_buffer_isr.c"),
    ("test_stream_buffer_isr_wakes_reader", ROOT / "tests" / "coupling" / "test_stream_buffer_isr_wakes_reader.c"),
    ("test_message_buffer_create_static", ROOT / "tests" / "unit" / "test_message_buffer_create_static.c"),
    ("test_message_buffer_send_receive", ROOT / "tests" / "unit" / "test_message_buffer_send_receive.c"),
    ("test_message_buffer_isr", ROOT / "tests" / "unit" / "test_message_buffer_isr.c"),
    ("test_heap_initialize", ROOT / "tests" / "unit" / "test_heap_initialize.c"),
    ("test_heap_linear", ROOT / "tests" / "unit" / "test_heap_linear.c"),
    ("test_heap_free_list", ROOT / "tests" / "unit" / "test_heap_free_list.c"),
    ("test_heap_coalescing", ROOT / "tests" / "unit" / "test_heap_coalescing.c"),
]

KERNEL_SOURCES = [
    ROOT / "src" / "kernel" / "mrt_event_group.c",
    ROOT / "src" / "kernel" / "mrt_list.c",
    ROOT / "src" / "kernel" / "mrt_kernel.c",
    ROOT / "src" / "kernel" / "mrt_heap.c",
    ROOT / "src" / "kernel" / "mrt_message_buffer.c",
    ROOT / "src" / "kernel" / "mrt_mutex.c",
    ROOT / "src" / "kernel" / "mrt_priority.c",
    ROOT / "src" / "kernel" / "mrt_queue.c",
    ROOT / "src" / "kernel" / "mrt_semaphore.c",
    ROOT / "src" / "kernel" / "mrt_stream_buffer.c",
    ROOT / "src" / "kernel" / "mrt_task.c",
    ROOT / "src" / "kernel" / "mrt_timer.c",
    ROOT / "src" / "portable" / "mock" / "mrt_port_mock.c",
]


def run(command):
    completed = subprocess.run(command, cwd=ROOT)
    return completed.returncode


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    failures = 0
    for name, source in TESTS:
        exe = BUILD / f"{name}.exe"
        compile_command = [
            "gcc",
            "-std=c99",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-DMRT_TESTING=1",
            "-Iinclude",
            "-Itests/support",
            str(source),
            *[str(kernel_source) for kernel_source in KERNEL_SOURCES],
            "-o",
            str(exe),
        ]
        print(f"[build] {name}")
        if run(compile_command) != 0:
            failures += 1
            continue
        print(f"[test] {name}")
        if run([str(exe)]) != 0:
            failures += 1
    if failures != 0:
        print(f"[summary] {failures} test target(s) failed")
        return 1
    print(f"[summary] {len(TESTS)} test target(s) passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
