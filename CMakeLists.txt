cmake_minimum_required(VERSION 3.27)
project(ccompiler C)

set(CMAKE_C_STANDARD 11)

add_executable(ccompiler main.c
        tokenize.c
        tokenize.h
        ast.h
        ast.c)
