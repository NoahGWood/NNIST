cmake_minimum_required(VERSION 3.16)
project(NNIST LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(NNIST_BUILD_TESTS "Build unit tests" ON)
option(NNIST_BUILD_EXAMPLES "Build examples" ON)
option(NNIST_FREESTANDING "Disable heap/stdlib features" OFF)

add_subdirectory(nnist)

# ToDo: Add Examples
# if(NNIST_BUILD_EXAMPLES)
#     # add_subdirectory(examples)
# endif()