cmake_minimum_required(VERSION 3.9)
project(agency-node CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(velocypack)
add_subdirectory(deserialize)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

add_library(store-lib helper-immut.h operation-deserializer.h node.h node.cpp helper-strings.h node-operations.h node-conditions.h store.h deserialize/deserializer.h deserialize/test/test-types.h agent.h test-helper.cpp test-helper.h)

target_include_directories(store-lib PUBLIC immer)
target_link_libraries(store-lib velocypack)

add_executable(test-tree agency-node-test.cpp)
target_link_libraries(test-tree store-lib)
#target_link_libraries(test-tree asan)
target_link_libraries(test-tree pthread)

#target_compile_options(test-tree PRIVATE "-fsanitize=address")
#target_compile_options(test-tree PRIVATE "-fsanitize=undefined")
target_compile_options(test-tree PRIVATE "-ftemplate-backtrace-limit=0")
#target_link_libraries(test-tree ubsan)

add_executable(store-mem-test agency-store-mem-test.cpp)
target_link_libraries(store-mem-test store-lib)
target_link_libraries(store-mem-test pthread)
