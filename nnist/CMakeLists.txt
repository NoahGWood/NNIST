set(NIST_VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vendor")

file(GLOB_RECURSE NIST_SOURCES
  src/*.cpp
)
add_library(nnist STATIC ${NIST_SOURCES})
target_include_directories(nnist PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(nnist PUBLIC cxx_std_20)

if(NNIST_FREESTANDING)
    target_compile_definitions(nnist PUBLIC NNIST_FREESTANDING=1)
    target_compile_options(nnist PRIVATE -ffreestanding -fno-exceptions -fno-rtti)
endif()

include(../cmake/warnings.cmake)
apply_project_warnings(nnist)

if(NNIST_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

