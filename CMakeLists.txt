cmake_minimum_required(VERSION 3.7)
project(OSLAB-EXTRA)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES src/bottom.h src/bottom.c src/call.h src/test.c src/type.h src/call.c)
add_executable(OSLAB-EXTRA ${SOURCE_FILES})