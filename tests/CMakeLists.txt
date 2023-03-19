cmake_minimum_required(VERSION 3.10)

# set the project name
project(rmsockets)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)

# include folders
if (MSVC)
FILE(GLOB_RECURSE MY_HEADERS "../include/*.h*")
include_directories(../include)

# add the executable
add_executable(rmsockets xpunit.cpp test_rmsockets_h.cpp ../wepoll/wepoll.c ${MY_HEADERS} )
endif()

if (LINUX)
# add the executable
add_executable(rmsockets xpunit.h xpunit.cpp ../include/rmsockets.h ../wepoll/wepoll.h ../wepoll/wepoll.c test_rmsockets.cpp )
endif()