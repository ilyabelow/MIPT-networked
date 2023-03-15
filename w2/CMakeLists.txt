cmake_minimum_required(VERSION 3.24)
project(hw2)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lenet")

add_executable(client Client.cpp Client.h Message.h)
add_executable(lobby Lobby.cpp Lobby.h)
add_executable(server Server.cpp Server.h)
