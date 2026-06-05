cmake_minimum_required(VERSION 3.28)
project(button_control C)

include(../../nanopi.cmake)

add_executable(led_daemon daemon.c)

# Link with the SSD1306 library
add_library(ssd1306 STATIC ssd1306.c)
target_link_libraries(led_daemon ssd1306)
