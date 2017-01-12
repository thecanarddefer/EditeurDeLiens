cmake_minimum_required(VERSION 2.4)
project(src LANGUAGES C)

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-wrap,read")

# 'elf_common' library
add_library(elf_common
    elf_common.c
    relocation.c
    section.c
    symbol.c
    util.c
    disp.c
)

# 'readelf' binary
add_executable(readelf readelf.c)
target_link_libraries(readelf elf_common)

# 'fusion' binary
add_executable(fusion fusion.c)
target_link_libraries(fusion elf_common)
