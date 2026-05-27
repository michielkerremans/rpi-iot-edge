cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(TempSensor)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

add_executable(TempSensor main.c gpio/gpio.c gpio/gpiod.c i2c/i2c1.c i2c/tc74.c util/logger.c mqtt/mqtt.c gtk/gui.c)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

target_link_libraries(TempSensor PJ_RPI gpiod paho-mqtt3c ${GTK3_LIBRARIES})
