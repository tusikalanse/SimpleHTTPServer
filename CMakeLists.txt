cmake_minimum_required (VERSION 2.8)

project (HTTPSERVER)

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -pthread -g")

AUX_SOURCE_DIRECTORY(src DIR_SRCS)

add_executable(SERVER ${DIR_SRCS})
