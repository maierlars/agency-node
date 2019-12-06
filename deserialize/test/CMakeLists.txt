add_executable(deserializer-test test.cpp)

target_link_libraries(deserializer-test deserializer-lib)
target_link_libraries(deserializer-test velocypack)
