cmake_minimum_required(VERSION 3.10)

# set the project name
project(rmsockets)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 17)

# include folders
FILE(GLOB_RECURSE MY_HEADERS "../*.h*")

# add the executable
if(MSVC)
   include_directories(../include ./wepoll ./)
   add_executable(rmsockets rmunit.cpp ../wepoll/wepoll.c test_rmsockets.cpp ${MY_HEADERS} )
else()
   include_directories(../include ./)
   add_executable(rmsockets rmunit.cpp test_rmsockets.cpp )
endif()
