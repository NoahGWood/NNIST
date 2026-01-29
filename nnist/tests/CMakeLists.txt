file(GLOB_RECURSE TEST_SOURCES
  *.cpp
)
message("BUILDING TEST FILES: ${TEST_SOURCES}")
message("SOURCE DIR: " ${CMAKE_SOURCE_DIR})
add_executable(nnist_tests ${TEST_SOURCES})
target_include_directories(nnist_tests PRIVATE
  include
  ${CMAKE_SOURCE_DIR}/vendor/NTest/include
  ${CMAKE_SOURCE_DIR}/vendor/NTest/vendor/NCLI/include
)
target_link_libraries(nnist_tests PRIVATE nnist)
add_test(NAME nnist_tests COMMAND nnist_tests)
