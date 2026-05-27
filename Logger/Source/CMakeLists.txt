cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(Logger)

add_executable(Logger main.c gpio.c gpiod.c dblogger.c)

target_link_libraries(Logger PJ_RPI mariadb gpiod)
