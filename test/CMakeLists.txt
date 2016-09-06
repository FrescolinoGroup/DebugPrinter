#=================== setting up tests ===================
file(GLOB_RECURSE UnitTests "." "*.cpp")
add_executable(unittests ${UnitTests} unittests.cpp)
#~ target_link_libraries(unittests lib_name)
add_test(NAME unittests COMMAND unittests)
