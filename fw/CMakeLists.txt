cmake_minimum_required(VERSION 3.13.1)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(BOARD esl)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app)

target_include_directories(app PRIVATE ${CMAKE_BINARY_DIR}/app/include src)

target_sources(app PRIVATE
  src/epd_main.c
  src/main.c)
