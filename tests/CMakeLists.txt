add_executable(test_matrix test_matrix.cpp)

target_include_directories(test_matrix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_test(NAME test_matrix COMMAND test_matrix)
