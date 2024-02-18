cmake_minimum_required(VERSION 3.27)
project(CS342_Project1 C)

set(CMAKE_C_STANDARD 11)

add_executable(CS342_Project1 main.c
        comserver.c
        comcli.c
        msg_buffer.h)
