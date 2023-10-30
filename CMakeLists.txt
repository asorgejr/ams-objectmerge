cmake_minimum_required(VERSION 3.15)

## PROJECT CONFIGURATION ##
project(hams
    VERSION 1.1.0
    DESCRIPTION "A set of libraries and operators to enhance the capabilities of Houdini."
    LANGUAGES CXX C
)
set(CMAKE_CXX_STANDARD 14)

option(BUILD_TESTS "Build the tests." OFF)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Z7 /Od")
endif()

## FIND PACKAGES ##
# Locate Houdini's libraries and header files.
# Registers an imported library target named 'Houdini'.
find_package(Houdini REQUIRED)

add_subdirectory(src)

if(BUILD_TESTS)
  add_subdirectory(test)
endif()
