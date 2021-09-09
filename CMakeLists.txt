cmake_minimum_required (VERSION 3.5)

project (HTTPSERVER)

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -pthread -g -Werror -Wall")

AUX_SOURCE_DIRECTORY(src DIR_SRCS)

add_executable(MAIN src/main.cpp src/myserver.cpp src/room.cpp src/timequeue.cpp src/user.cpp)
add_executable(REPLICA src/replica.cpp src/myserver.cpp src/room.cpp src/timequeue.cpp src/user.cpp)
