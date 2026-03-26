cmake_minimum_required(VERSION 3.10)

# 1. Set the project name here
project(Anudb VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)
# Source files for the core library (logic only, no main)
set(CORE_SOURCES 
    src/BPlusTree.cpp 
    src/Metadata.cpp 
    src/PageManager.cpp 
    src/Parser.cpp 
    src/Tokenizer.cpp 
    src/RowSerializer.cpp 
    src/Executor.cpp
    src/linenoise.c
)

# Main executable
add_executable(anudb src/main.cpp ${CORE_SOURCES})

# Tests
add_executable(test_bplustree tests/test_bplustree.cpp ${CORE_SOURCES})
add_executable(test_serializer tests/test_serializer.cpp ${CORE_SOURCES})
add_executable(billion_bench tests/billion_bench.cpp ${CORE_SOURCES})

if(MSVC)
    target_compile_options(anudb PRIVATE /W4)
    target_compile_options(test_bplustree PRIVATE /W4)
    target_compile_options(test_serializer PRIVATE /W4)
    target_compile_options(billion_bench PRIVATE /W4)
else()
    target_compile_options(anudb PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(test_bplustree PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(test_serializer PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(billion_bench PRIVATE -Wall -Wextra -pedantic)
endif()
