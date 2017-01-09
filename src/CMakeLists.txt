cmake_minimum_required(VERSION 2.4)
project(src LANGUAGES C)


# 'elf_common' library
add_library(elf_common
	elf_common.c
	symbolTable.c
	util.c
)

# 'readelf' binary
add_executable(readelf readelf.c)
target_link_libraries(readelf elf_common)

# 'fusion' binary
add_executable(fusion fusion.c)
target_link_libraries(fusion elf_common)
