cmake_minimum_required (VERSION 3.5)

project (HTTPSERVER)

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -pthread -g -Werror -Wall")

AUX_SOURCE_DIRECTORY(src DIR_SRCS)

add_executable(SERVER ${DIR_SRCS})
