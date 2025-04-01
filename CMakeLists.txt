cmake_minimum_required(VERSION 3.10)
project(SO2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_SOURCE_DIR}/include) # it's very possible that there will be nothing in include, but I'm doing this just in case anyway

set(SOURCES 
    ${CMAKE_SOURCE_DIR}/src/main.cpp
)

add_executable(SO2 ${SOURCES})