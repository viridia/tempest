# CMake build file for Tempest language

cmake_minimum_required(VERSION 2.8.4)
project(TEMPEST C CXX)

set(TEMPEST_VERSION 0)
set(TEMPEST_MAJOR_REVISION 1)
set(TEMPEST_MINOR_REVISION 0)

# Packages
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
find_package(ZLIB REQUIRED)

# Generate compile commands for editor
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Macros we'll need
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckCXXSourceCompiles)

# C Headers.
check_include_file(cxxabi.h TEMPEST_HAVE_CXXABI_H)
check_include_file(dlfcn.h TEMPEST_HAVE_DLFCN_H)
check_include_file(execinfo.h TEMPEST_HAVE_EXECINFO_H)
check_include_file(unistd.h TEMPEST_HAVE_UNISTD_H)

# C++ Headers.
check_include_file_cxx(csignal TEMPEST_HAVE_CSIGNAL)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Extra flags for GCC (C++ only)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_definitions(
      -pipe
      -Wall -Wextra -Werror -Wcast-align -Wpointer-arith
      -Wno-deprecated -Wno-unused
      -fmessage-length=0
  )
endif()

# Extra flags for GCC (Clang only)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions(
      -Wall
      -Wextra
      -Werror
      -Wcast-align
      -Wpointer-arith
      -Wno-deprecated
      -Wno-unused-parameter
      -fno-rtti
      -fPIC
  )
# Flags to enable coverage data
#      -fprofile-instr-generate
#      -fcoverage-mapping
#  set(CMAKE_EXE_LINKER_FLAGS -fprofile-instr-generate)
#  set(CMAKE_EXE_LINKER_FLAGS -lstdc++)
endif()

include_directories(
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/compiler)

# Generate the config.h file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_BINARY_DIR}/include/config.h)

add_subdirectory(compiler)
add_subdirectory(compiler_tests)
