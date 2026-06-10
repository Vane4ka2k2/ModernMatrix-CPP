cmake_minimum_required(VERSION 3.23)
project(NewMatrix LANGUAGES CXX)

# Собираем проект в режиме C++23 с отключением расширений компилятора.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing() # Включаем поддержку CTest для запуска модульных тестов.

add_executable(NewMatrix main.cpp)

target_compile_features(NewMatrix PUBLIC cxx_std_23)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(NewMatrix PRIVATE -Wall -Wextra -Wpedantic)
elseif (MSVC)
    target_compile_options(NewMatrix PRIVATE /W4 /permissive- /permissive- /Za)
endif()

target_include_directories(NewMatrix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Тестовый исполняемый файл для проверки корректности Matrix.
add_executable(matrix_tests tests/test_matrix.cpp)

target_compile_features(matrix_tests PUBLIC cxx_std_23)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(matrix_tests PRIVATE -Wall -Wextra -Wpedantic)
elseif (MSVC)
    target_compile_options(matrix_tests PRIVATE /W4 /permissive- /permissive- /Za)
endif()

target_include_directories(matrix_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Регистрируем тест, чтобы его можно было запускать через ctest.
add_test(NAME MatrixTests COMMAND matrix_tests)
