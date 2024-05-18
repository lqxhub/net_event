cmake_minimum_required(VERSION 3.25)

project(netevent)

set(CMAKE_CXX_STANDARD 20)

ADD_SUBDIRECTORY(net)

INCLUDE_DIRECTORIES(net)


add_executable(netevent main.cpp)

TARGET_LINK_LIBRARIES(netevent net pthread)
