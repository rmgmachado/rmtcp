cmake_minimum_required(VERSION 3.10)

# set the project name
project(rmsockets)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)

add_subdirectory(tests)
