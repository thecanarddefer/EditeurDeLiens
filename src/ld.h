#ifndef _LD_H_
#define _LD_H_

#include <elf.h>


typedef struct
{
	Elf32_Shdr *ptr_shdr1;
	Elf32_Shdr *ptr_shdr2;
	Elf32_Word size;
	Elf32_Off offset;
} Fusion;

/**
 * Écrit une section PROGBITS
 *
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 **/
int write_progbits_in_file(int fd_in, int fd_out, Elf32_Word size, Elf32_Off offset);


#endif
