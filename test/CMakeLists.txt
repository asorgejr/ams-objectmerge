include(cmake/gtest.cmake)
include(GoogleTest)
get_gtest()


function(config_test TARGET_NAME)
  if(MSVC)
    target_compile_definitions(${TARGET_NAME} PRIVATE NOMINMAX)
  endif()
endfunction()


set(TEST_NAMES test_hams test_re_replace)
add_executable(test_hams test_hams.cpp)
config_test(test_hams)

file(GLOB TEST_DEPS
  "${CMAKE_SOURCE_DIR}/src/ams_utils.*")

target_link_libraries(test_hams
  PRIVATE
    gtest_main Houdini)

target_sources(test_hams
  PRIVATE
  ${TEST_DEPS}) # Add sources instead of linking project to avoid errors associated with missing Houdini runtime

gtest_discover_tests(${TEST_NAMES})


add_executable(binlib binlib.cpp)
target_link_libraries(binlib Houdini)
target_sources(binlib PRIVATE ${TEST_DEPS})