cmake_minimum_required(VERSION 3.5)

project(
  libmbus
  LANGUAGES CXX C
  VERSION "0.9.0")

if(CMAKE_BUILD_TYPE STREQUAL "")
  message(STATUS "CMAKE_BUILD_TYPE empty setting to Debug")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# ##############################################################################
# default options -> changed with e.g. cd build && cmake ..
# -DLIBMBUS_BUILD_TESTS=ON
# ##############################################################################

option(LIBMBUS_BUILD_EXAMPLES "build examples" OFF)
option(LIBMBUS_BUILD_TESTS "build tests" OFF)
option(LIBMBUS_ENABLE_COVERAGE "build with coverage support" OFF)
option(LIBMBUS_RUN_CLANG_TIDY "use Clang-Tidy for static analysis" OFF)
option(LIBMBUS_PACKAGE_DEB "build debian package" OFF)
option(LIBMBUS_PACKAGE_RPM "build rpm package" OFF)
option(LIBMBUS_BUILD_DOCS "build documentation" OFF)
option(BUILD_SHARED_LIBS "build shared lib" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

# Append our module directory to CMake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_BINARY_DIR}")

# Set the output of the libraries and executables.
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# ##############################################################################
# static analysis
# ##############################################################################

if(LIBMBUS_RUN_CLANG_TIDY)
  find_program(
    CLANG_TIDY_EXE
    NAMES "clang-tidy"
    DOC "/usr/bin/clang-tidy")
  if(NOT CLANG_TIDY_EXE)
    message(WARNING "clang-tidy not found.")
  else()
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
  endif()
endif(LIBMBUS_RUN_CLANG_TIDY)

if(LIBMBUS_ENABLE_COVERAGE)
  if(NOT CMAKE_BUILD_TYPE MATCHES "(Debug)|(RelWithDebInfo)")
    message(WARNING "Code coverage results with an optimised (non-Debug) build may be misleading")
  endif()

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
    message(STATUS "Not doing coverage...")
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    message(STATUS "Building with code coverage...")
    set(CMAKE_BUILD_TYPE DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -ggdb3 -O0 --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -ggdb3 -O0 --coverage -fprofile-arcs -ftest-coverage ")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage ")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage ")
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

# ##############################################################################
# library
# ##############################################################################

set(PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")

set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(VERSION "${PROJECT_VERSION}")
configure_file(${CMAKE_CURRENT_LIST_DIR}/mbus/config.h.in ${CMAKE_CURRENT_LIST_DIR}/config.h @ONLY)

# list of source files
add_library(objlib OBJECT
  "${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-protocol.c"
  "${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-protocol.h"
  "${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-tcp.c"
  "${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-tcp.h"
  "${CMAKE_CURRENT_LIST_DIR}/mbus/mbus.c"
  "${CMAKE_CURRENT_LIST_DIR}/mbus/mbus.h"
  "${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-protocol-aux.c"
  "${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-protocol-aux.h"
  "${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-serial.c"
  "${CMAKE_CURRENT_LIST_DIR}/mbus/mbus-serial.h")

# shared and static libraries built from the same object files
add_library(${PROJECT_NAME} SHARED $<TARGET_OBJECTS:objlib>)
add_library(libmbus_static STATIC $<TARGET_OBJECTS:objlib>)

set_property(TARGET objlib PROPERTY POSITION_INDEPENDENT_CODE 1)
set_target_properties(libmbus_static PROPERTIES OUTPUT_NAME mbus)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>" "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
         "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Android")
  target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()
if(NOT MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-pedantic)
endif()

set_target_properties(${PROJECT_NAME}
   PROPERTIES PREFIX ""
   SOVERSION ${VERSION})

if(CLANG_TIDY_EXE)
  set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# ##############################################################################
# examples
# ##############################################################################

if(LIBMBUS_BUILD_EXAMPLES)
  message(STATUS "building examples")
  add_subdirectory(bin)
endif()

# ##############################################################################
# tests
# ##############################################################################

if(LIBMBUS_BUILD_TESTS)
  message(STATUS "building tests")
  enable_testing()
  add_subdirectory(test)
endif()

# ##############################################################################
# install
# ##############################################################################

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(INSTALL_PKGCONFIG_DIR
    "${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig"
    CACHE PATH "Installation directory for pkgconfig (.pc) files")
set(INSTALL_INC_DIR
    "${CMAKE_INSTALL_INCLUDEDIR}/mbus"
    CACHE PATH "Installation directory for headers")
set(INSTALL_LIB_DIR
    "${CMAKE_INSTALL_LIBDIR}"
    CACHE PATH "Installation directory for libraries")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libmbus.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libmbus.pc @ONLY)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/libmbus.pc
  DESTINATION "${INSTALL_PKGCONFIG_DIR}"
  COMPONENT dev)

set(LIBMBUS_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT lib
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT lib)
install(
  EXPORT ${PROJECT_NAME}Targets
  DESTINATION "${LIBMBUS_CONFIG_INSTALL_DIR}"
  NAMESPACE ${PROJECT_NAME}::
  COMPONENT dev)

configure_package_config_file(cmake/Config.cmake.in ${PROJECT_NAME}Config.cmake INSTALL_DESTINATION
                              "${LIBMBUS_CONFIG_INSTALL_DIR}")
write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake COMPATIBILITY SameMajorVersion)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION "${LIBMBUS_CONFIG_INSTALL_DIR}"
  COMPONENT dev)

install(
  DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/mbus/"
  DESTINATION "${INSTALL_INC_DIR}"
  COMPONENT dev
  FILES_MATCHING
  PATTERN "*.h")

install(
  TARGETS libmbus_static
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT dev)

# ##############################################################################
# package
# mkdir build ; cd build ; cmake .. -DLIBMBUS_PACKAGE_DEB=ON ; cpack ..
# ##############################################################################

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open source M-bus (Meter-Bus) library.")
set(CPACK_PACKAGE_DESCRIPTION
    "libmbus is an open source library for the M-bus (Meter-Bus) protocol.
The Meter-Bus is a standard for reading out meter data from electricity meters,
heat meters, gas meters, etc. The M-bus standard deals with both the electrical
signals on the M-Bus, and the protocol and data format used in transmissions
on the M-Bus. The role of libmbus is to decode/encode M-bus data, and to handle
the communication with M-Bus devices.

For more information see http://www.rscada.se/libmbus")

set(CPACK_PACKAGE_VENDOR "Raditex Control AB")
set(CPACK_PACKAGE_CONTACT "Stefan Wahren <info@lategoodbye.de>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/rscada/libmbus/")
set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_DEBUG ON)
set(CPACK_PACKAGE_RELEASE 1)

# create 2 components, libmbus, libmbus-dev
set(CPACK_COMPONENTS_ALL lib dev)
set(CPACK_COMPONENT_LIB_DESCRIPTION "FreeSCADA M-Bus Library.
    A free and open-source library for M-Bus (Meter Bus) from the rSCADA project.")
set(CPACK_DEBIAN_LIB_PACKAGE_SECTION libs)

set(CPACK_COMPONENT_DEVEL_DESCRIPTION
    "FreeSCADA M-Bus Library Development files.
A free and open-source library for M-Bus (Meter Bus) from the rSCADA project,
including development files.")
set(CPACK_DEBIAN_DEVEL_PACKAGE_SECTION libdevel)

set(CPACK_COMPONENT_DEVEL_DEPENDS lib)

set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}_${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

if(LIBMBUS_PACKAGE_DEB)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Stefan Wahren <info@lategoodbye.de>")
  set(CPACK_DEBIAN_PACKAGE_SECTION "Development/Languages/C and C++")
  set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
  set(CPACK_DEBIAN_PACKAGE_VERSION
      "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}"
  )
endif()

if(LIBMBUS_PACKAGE_RPM)
  set(CPACK_GENERATOR "RPM")
  set(CPACK_RPM_PACKAGE_LICENSE "BSD")
endif()

include(CPack)

# ##############################################################################
# doc
# mkdir build ; cd build ; cmake .. -DLIBMBUS_BUILD_DOCS=ON ; cmake --build . --target doc
# ##############################################################################

if(LIBMBUS_BUILD_DOCS)
  message(STATUS "building with documentation")
  # Generate targets for documentation
  # check if Doxygen is installed
  find_package(Doxygen)

  if(Doxygen_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    # note the option ALL which allows to build the docs together with the application
    add_custom_target(
      doc ALL
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)

    message(STATUS "Setup up the Doxygen documention build")

  else(Doxygen_FOUND)
    message(WARNING "Doxygen need to be installed to generate the doxygen documentation")
  endif(Doxygen_FOUND)
endif()
