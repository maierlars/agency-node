cmake_minimum_required(VERSION 3.2)
project(agency-node CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(velocypack)
add_subdirectory(deserialize)


add_executable(test-tree agency-node-test.cpp helper-immut.h operation-deserializer.h node.h node.cpp helper-strings.h node-operations.h node-conditions.h store.h deserialize/deserializer.h deserialize/test/test-types.h)
target_link_libraries(test-tree asan)
target_link_libraries(test-tree pthread)
target_link_libraries(test-tree velocypack)

target_compile_options(test-tree PRIVATE "-fsanitize=address")
target_compile_options(test-tree PRIVATE "-fsanitize=undefined")
target_compile_options(test-tree PRIVATE "-ftemplate-backtrace-limit=0")

target_link_libraries(test-tree ubsan)

target_include_directories(test-tree PUBLIC immer)
