cmake_minimum_required(VERSION 3.15)
project(StreamPipe C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(streampipe
    src/main.c
    src/input.c
    src/filter.c
    src/queue.c
    src/wal.c
    src/alert.c
    src/asm_crc32.c
)

target_include_directories(streampipe PRIVATE include)

target_link_libraries(streampipe PRIVATE pthread)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(streampipe PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS streampipe DESTINATION bin)
