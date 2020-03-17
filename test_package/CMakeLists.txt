cmake_minimum_required(VERSION 3.1.2)

project(libmbus-app
        CXX)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(libmbus
             REQUIRED)

add_executable(libmbus-app example.cpp)
target_link_libraries(libmbus-app
                      libmbus::libmbus)
