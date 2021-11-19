cmake_minimum_required(VERSION 3.19)
project(os_test C)

set(CMAKE_C_STANDARD 11)

add_executable(os_test
        OS-v6FileSystem/mod-v6.c)
