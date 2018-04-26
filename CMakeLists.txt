cmake_minimum_required(VERSION 3.7)
project(OSLAB-EXTRA)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES bottom.h bottom.c call.h test.c type.h src/call.c main.c)
add_executable(OSLAB-EXTRA ${SOURCE_FILES})