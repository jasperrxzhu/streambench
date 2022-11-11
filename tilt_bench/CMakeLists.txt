cmake_minimum_required(VERSION 3.13.4)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(tilt_bench)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM 11.0.0 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_subdirectory(tilt/third_party/easy_jit)

include_directories(${LLVM_INCLUDE_DIRS} include tilt/tilt/include)
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs native orcjit mcjit objcarcopts)
set(easyjit_lib "${CMAKE_BINARY_DIR}/tilt/third_party/easy_jit/bin/EasyJitPass.so")

option(PRINT_REGION "Option description" OFF)
if(PRINT_REGION)
    message(STATUS "PRINT_REGION Flag ON")
    add_compile_definitions(_PRINT_REGION_)
endif()

# if this option is ON, region_t objects will have an additional variable
#   to record the period of the stream
#   otherwise, all vinstrs will always add 1 to the start ts to get end ts
option(REGION_PERIOD "Option description" OFF)
if(REGION_PERIOD)
    message(STATUS "REGION_PERIOD Flag ON")
    add_compile_definitions(_REGION_PERIOD_)
endif()

add_subdirectory(tilt/tilt)

add_executable(main main.cpp)
target_link_libraries(main tilt)
