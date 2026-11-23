cmake_minimum_required(VERSION 3.16)
project(minishell LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

set(SHELL_SOURCES
    src/main.cpp
    src/shell/shell.cpp
    src/parser/parser.cpp
    src/executor/executor.cpp
    src/process/job_manager.cpp
    src/process/terminal.cpp
    src/ipc/ipc.cpp
    src/signals/signal_handler.cpp
    src/monitor/process_monitor.cpp
    src/history/history.cpp
)

add_executable(minishell ${SHELL_SOURCES})
target_include_directories(minishell PRIVATE src)

add_executable(parser_tests tests/parser_tests.cpp src/parser/parser.cpp)
target_include_directories(parser_tests PRIVATE src)

add_executable(monitor_tests tests/monitor_tests.cpp src/monitor/process_monitor.cpp)
target_include_directories(monitor_tests PRIVATE src)

enable_testing()
add_test(NAME parser COMMAND parser_tests)
add_test(NAME monitor COMMAND monitor_tests)
