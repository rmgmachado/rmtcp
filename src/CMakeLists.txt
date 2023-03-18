cmake_minimum_required(VERSION 3.10)

# set the project name
project(rmtcplib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)

# include folders
FILE(GLOB_RECURSE MY_HEADERS "../include/*.h*")
include_directories(../include)

# add the executable
add_executable(rmtcplib main.cpp ../wepoll/wepoll.c ${MY_HEADERS} )