# Need LLVM libraries
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Include the non-generated library files.
file(GLOB_RECURSE headers tempest/**/*.h)
file(GLOB_RECURSE sources tempest/**/*.cpp)

# Find the libraries that correspond to the LLVM components that we wish to use
llvm_map_components_to_libnames(llvm_libs support core scalaropts target x86info x86codegen)

add_library(tec ${sources} ${headers})
set_target_properties(tec PROPERTIES LINKER_LANGUAGE CXX)

add_executable(tempestc tempestc.cpp)
target_link_libraries(tempestc tec ${llvm_libs})
