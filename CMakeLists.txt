cmake_minimum_required(VERSION 2.4)
project(EditeurDeLiens LANGUAGES C)


if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE)
set(C_FLAGS_EXTRA         "-Wall -Wextra -Wuninitialized -Wstrict-prototypes -Wno-sign-compare -std=c99")
set(CMAKE_C_FLAGS         "${CMAKE_C_FLAGS}         ${C_FLAGS_EXTRA}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${C_FLAGS_EXTRA}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

message(STATUS "Building ${PROJECT_NAME} with build type ${CMAKE_BUILD_TYPE}")
add_subdirectory(src)
add_subdirectory(tests)
