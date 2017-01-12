#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>
#include <elf.h>
#include "elf_common.h"
#include "util.h"

Elf32_Ehdr *read_elf_header(int fd)
{
	Elf32_Ehdr *ehdr = malloc(sizeof(Elf32_Ehdr));

	read(fd, ehdr->e_ident, EI_NIDENT);
	if(ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 || ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3 || ehdr->e_ident[EI_CLASS] != ELFCLASS32)
	{
		fprintf(stderr, "Le fichier n'est pas de type ELF32.\n");
		exit(3);
	}

	if(ehdr->e_ident[EI_DATA] == ELFDATA2LSB)
		elf32_is_big = 0;
	else if(ehdr->e_ident[EI_DATA] == ELFDATA2MSB)
		elf32_is_big = 1;

	read(fd, &ehdr->e_type,      sizeof(ehdr->e_type));
	read(fd, &ehdr->e_machine,   sizeof(ehdr->e_machine));
	read(fd, &ehdr->e_version,   sizeof(ehdr->e_version));
	read(fd, &ehdr->e_entry,     sizeof(ehdr->e_entry));
	read(fd, &ehdr->e_phoff,     sizeof(ehdr->e_phoff));
	read(fd, &ehdr->e_shoff,     sizeof(ehdr->e_shoff));
	read(fd, &ehdr->e_flags,     sizeof(ehdr->e_flags));
	read(fd, &ehdr->e_ehsize,    sizeof(ehdr->e_ehsize));
	read(fd, &ehdr->e_phentsize, sizeof(ehdr->e_phentsize));
	read(fd, &ehdr->e_phnum,     sizeof(ehdr->e_phnum));
	read(fd, &ehdr->e_shentsize, sizeof(ehdr->e_shentsize));
	read(fd, &ehdr->e_shnum,     sizeof(ehdr->e_shnum));
	read(fd, &ehdr->e_shstrndx,  sizeof(ehdr->e_shstrndx));

	return ehdr;
}

void destroy_elf_header(Elf32_Ehdr *ehdr)
{
	free(ehdr);
}

int get_section_index(Section_Table *secTab, int shType) {
	int i = 0,
		idx = -1;
	while (i < secTab->nb_sections) {
		if (secTab->shdr[i]->sh_type == shType) {
			idx = i;
		}
		i++;
	}
	return idx;
}

char *get_name_table(int fd, int idxSection, Elf32_Shdr **shdr)
{
	char *table = (char *) malloc(sizeof(char) * shdr[idxSection]->sh_size);

	lseek(fd, shdr[idxSection]->sh_offset, SEEK_SET);
	read(fd, table, shdr[idxSection]->sh_size);

	return table;
}

char *get_section_name(Section_Table *secTab, unsigned index)
{
	return &(secTab->sectionNameTable[secTab->shdr[index]->sh_name]);
}

