cmake_minimum_required(VERSION 3.13.1)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

# Shortcut to not have to call `west -b`
if (NOT DEFINED ${BOARD})
  set(BOARD esl)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app)

target_include_directories(app PRIVATE ${CMAKE_BINARY_DIR}/app/include src)

target_sources(app PRIVATE
  src/creep.c
  src/framebuffer.c
  src/main.c)

target_sources_ifdef(CONFIG_UART_EPD app PRIVATE src/epd_serial.c)
target_sources_ifdef(CONFIG_UC8251D  app PRIVATE src/epd_driver.c)
target_sources_ifdef(CONFIG_SPI_NOR  app PRIVATE src/flash.c)
target_sources_ifdef(CONFIG_ADC      app PRIVATE src/adc.c)
