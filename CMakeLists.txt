# Minimum CMake version required
cmake_minimum_required(VERSION 3.20)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build.")

# Project information
project(Expection CXX)
# include_directories(include)

# Set the required C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Add doctest
include(FetchContent)
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG master
)
FetchContent_MakeAvailable(doctest)

# Add the /utf-8 flag for MSVC
if(MSVC)
    add_compile_options("/utf-8")
endif()

# Add executable targets
add_executable(testexpection Expection.hpp testexpection.cpp)
target_link_libraries(testexpection PRIVATE doctest)
target_include_directories(testexpection PRIVATE ${doctest_SOURCE_DIR}/doctest)

add_executable(example Expection.hpp example.cpp)

# Enable testing with CTest
enable_testing()
add_test(NAME testexpection COMMAND testexpection)
