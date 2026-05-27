cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(Read)

add_executable(Read main.c)

target_link_libraries(Read PJ_RPI)
