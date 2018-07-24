# Need LLVM libraries
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Include the non-generated library files.
file(GLOB_RECURSE headers **/*.h **/*.hpp)
file(GLOB_RECURSE sources *.cpp **/*.cpp)

# Find the libraries that correspond to the LLVM components that we wish to use
llvm_map_components_to_libnames(llvm_libs support core scalaropts)

add_executable(compiler_tests ${sources} ${headers})
target_link_libraries(compiler_tests ${llvm_libs} tec)

enable_testing ()
add_test (NAME "Compiler" COMMAND compiler_tests)
