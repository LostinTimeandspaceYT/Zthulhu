cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zth_json_parser_tests)

set(ZTHULHU_APP_SRC ${CMAKE_CURRENT_LIST_DIR}/../../applications/app/src)

target_include_directories(app PRIVATE ${ZTHULHU_APP_SRC})
target_sources(app PRIVATE
  ${ZTHULHU_APP_SRC}/json_parser.c
  src/main.c
)
