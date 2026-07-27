cmake_minimum_required(VERSION 3.26)

project(occt_cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OCCT_CLI_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_executable(brepkit
    src/main.cpp
    src/cli.cpp
    src/step.cpp
    src/brp.cpp
)

target_include_directories(brepkit
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(brepkit
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external
)

# Find OCCT.
# In practice you will often point CMake at your OCCT install with:
#   -DOpenCASCADE_DIR=/path/to/occt/cmake
#
# We ask for the toolkits we actually use.
find_package(OpenCASCADE REQUIRED)

target_include_directories(brepkit SYSTEM PRIVATE ${OpenCASCADE_INCLUDE_DIR})

message(STATUS "OpenCASCADE_INCLUDE_DIR=${OpenCASCADE_INCLUDE_DIR}")
message(STATUS "OpenCASCADE_LIBRARIES=${OpenCASCADE_LIBRARIES}")

target_link_libraries(brepkit PRIVATE ${OpenCASCADE_LIBRARIES})

if (MSVC)
    target_compile_options(brepkit PRIVATE /W4 /permissive-)
    if (OCCT_CLI_WARNINGS_AS_ERRORS)
        target_compile_options(brepkit PRIVATE /WX)
    endif()
else()
    target_compile_options(brepkit PRIVATE -Wall -Wextra -Wpedantic -Wno-extra-semi)
    if (OCCT_CLI_WARNINGS_AS_ERRORS)
        target_compile_options(brepkit PRIVATE -Werror)
    endif()
endif()
