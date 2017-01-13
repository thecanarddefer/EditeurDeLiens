cmake_minimum_required(VERSION 2.4)
project(tests NONE)


# Specify the cross compiler
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
SET(CMAKE_CROSSCOMPILING 1)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_FLAGS "-mno-thumb-interwork -g -O2 -nostdlib -Wl,--entry=0")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# 'fusion_result' file
add_library(files OBJECT file1.c file2.c)
add_executable(fusion_result $<TARGET_OBJECTS:files> eabi-none-lib.c)
target_link_libraries(fusion_result)

add_library(files_test OBJECT test_main.c test_add.c test_loop.c)
add_executable(test_fusion $<TARGET_OBJECTS:files_test> eabi-none-lib.c)
target_link_libraries(test_fusion)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mbig-endian")
#add_library(bigendian OBJECT bigendian.c)

# Copy object files in this directory
add_custom_command(TARGET fusion_result
	POST_BUILD
	COMMAND cp -v "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/*.dir/*.o" "${CMAKE_CURRENT_SOURCE_DIR}/"
	COMMENT "Copy object files into ${CMAKE_CURRENT_SOURCE_DIR}/ directory"
)

