# Need LLVM libraries
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Include the non-generated library files.
file(GLOB_RECURSE headers **/*.h **/*.hpp)
file(GLOB_RECURSE sources *.cpp **/*.cpp)

# Find the libraries that correspond to the LLVM components that we wish to use
llvm_map_components_to_libnames(llvm_libs core support scalaropts x86info x86codegen)

add_executable(compiler_tests ${sources} ${headers})
target_link_libraries(compiler_tests ${llvm_libs} tec)

file(
  COPY ${CMAKE_CURRENT_SOURCE_DIR}/compilation
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  FILES_MATCHING PATTERN "*.te")

enable_testing ()
add_test (NAME "Compiler" COMMAND compiler_tests)
