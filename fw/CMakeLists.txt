cmake_minimum_required(VERSION 3.13.1)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(BOARD esl)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app)

target_include_directories(app PRIVATE ${CMAKE_BINARY_DIR}/app/include src)

target_sources(app PRIVATE
  src/creep.c
  src/main.c)

target_sources_ifdef(CONFIG_UART_EPD app PRIVATE src/epd_serial.c)
target_sources_ifdef(CONFIG_UC8251D  app PRIVATE src/epd_driver.c)
