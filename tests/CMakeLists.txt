enable_testing()
find_package(GTest REQUIRED)

add_executable(AnyValueTests AnyValueTest.cpp)
target_link_libraries(AnyValueTests GTest::gtest GTest::gtest_main)