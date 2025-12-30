cmake_minimum_required(VERSION 3.10)

# 1. Set the project name here
project(Anudb VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

# 2. Set the executable name here
add_executable(anudb ${SOURCES})

if(MSVC)
    target_compile_options(anudb PRIVATE /W4)
else()
    target_compile_options(anudb PRIVATE -Wall -Wextra -pedantic)
endif()
