cmake_minimum_required(VERSION 3.20)
project(ModernMatrix-CPP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ModernMatrix-CPP
    src/main.cpp
    include/matrix.hpp
)

target_include_directories(ModernMatrix-CPP PRIVATE include)

if(BUILD_TESTING OR CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
        add_subdirectory(tests)
    endif()
endif()