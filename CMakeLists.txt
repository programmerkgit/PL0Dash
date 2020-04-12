cmake_minimum_required(VERSION 3.15)
project(PL0Dash C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(PL0Dash
        codegen.c
        codegen.h
        compile.c
        compile.h
        getSource.c
        getSource.h
        main.c
        table.c
        table.h)
