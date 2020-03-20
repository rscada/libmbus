cmake_minimum_required(VERSION 3.8)

project(libmbus LANGUAGES CXX C)

set(PROJECT_VERSION "0.9.0")

if(CMAKE_BUILD_TYPE STREQUAL "")
  message(STATUS "CMAKE_BUILD_TYPE empty setting to Debug")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

option(LIBMBUS_BUILD_EXAMPLES "build examples" OFF)
option(LIBMBUS_BUILD_TESTS "build tests" OFF)
option(LIBMBUS_ENABLE_COVERAGE "Build with coverage support" ON)
option(LIBMBUS_RUN_CLANG_TIDY "Use Clang-Tidy for static analysis" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

# Append our module directory to CMake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

# Set the output of the libraries and executables.
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

#
# static analysis
#

if(LIBMBUS_RUN_CLANG_TIDY)
  find_program(
    CLANG_TIDY_EXE
    NAMES "clang-tidy"
    DOC "/usr/bin/clang-tidy")
  if(NOT CLANG_TIDY_EXE)
    message(WARNING "clang-tidy not found.")
  else()
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
  endif()
endif(LIBMBUS_RUN_CLANG_TIDY)

if(LIBMBUS_ENABLE_COVERAGE)
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(
      WARNING
        "Code coverage results with an optimised (non-Debug) build may be misleading"
    )
  endif(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  # https://discuss.circleci.com/t/problems-using-lcov-gcov-for-c-code/13898
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
    message(STATUS "Not doing coverage...")
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    message(STATUS "Building with code coverage...")
    set(CMAKE_BUILD_TYPE DEBUG)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -fPIC -ggdb3 -O0 --coverage -fprofile-arcs -ftest-coverage"
    )
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -fPIC -ggdb3 -O0 --coverage -fprofile-arcs -ftest-coverage "
    )
    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage ")
    set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage ")
    link_libraries(-lgcov)
  endif()
endif()

include(CheckIncludeFile)

check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(stdlib.h HAVE_STDINT_H)
check_include_file(stdint.h HAVE_STDLIB_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)

#
# library
#
set(PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")

set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(VERSION "${PROJECT_VERSION}")
configure_file(${CMAKE_CURRENT_LIST_DIR}/mbus/config.h.in
               ${CMAKE_CURRENT_LIST_DIR}/config.h)

add_library(
  ${PROJECT_NAME}
  ${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-protocol.c
  ${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-protocol.h
  ${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-tcp.c
  ${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-tcp.h
  ${CMAKE_CURRENT_LIST_DIR}/mbus/mbus.c
  ${CMAKE_CURRENT_LIST_DIR}/mbus/mbus.h
  ${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-protocol-aux.c
  ${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-protocol-aux.h
  ${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-serial.c
  ${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-serial.h)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(${PROJECT_NAME} PRIVATE m)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-pedantic)

if(CLANG_TIDY_EXE)
  set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY
                                                   "${DO_CLANG_TIDY}")
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

#
# examples
#

if(LIBMBUS_BUILD_EXAMPLES)
  message(STATUS "building examples")
  add_subdirectory(bin)
endif()

#
# tests
#

if(LIBMBUS_BUILD_TESTS)
  message(STATUS "building tests")
  enable_testing()
  add_subdirectory(test)
endif()

#
# install
#

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(LIBMBUS_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib)

install(
  EXPORT ${PROJECT_NAME}Targets
  DESTINATION ${LIBMBUS_CONFIG_INSTALL_DIR}
  NAMESPACE ${PROJECT_NAME}::
  COMPONENT dev)

configure_package_config_file(cmake/Config.cmake.in ${PROJECT_NAME}Config.cmake
                              INSTALL_DESTINATION ${LIBMBUS_CONFIG_INSTALL_DIR})
write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake
                                 COMPATIBILITY SameMajorVersion)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${LIBMBUS_CONFIG_INSTALL_DIR}
  COMPONENT dev)

# BUG: installing empty dirs https://gitlab.kitware.com/cmake/cmake/issues/17122
install(
  DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/mbus/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mbus/
  COMPONENT dev
  FILES_MATCHING
  PATTERN "*.h")
