add_executable(deserializer-test test.cpp)

# why is there no error without this check in case
# the velocypack target is not defined.
if(NOT TARGET velocypack)
    message(FATAL_ERROR "velocypack not found")
endif()

target_link_libraries(deserializer-test
    deserializer-lib
    velocypack
)
target_compile_options(deserializer-test PRIVATE "-ftemplate-backtrace-limit=0")
