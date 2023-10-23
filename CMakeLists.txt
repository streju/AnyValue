cmake_minimum_required(VERSION 3.2)

project(AnyValue)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wno-deprecated -Wall -Werror -ggdb -Wextra -Wno-unused-parameter -fPIC")

include_directories(
    ${PROJECT_SOURCE_DIR}/src
)

add_subdirectory(tests)