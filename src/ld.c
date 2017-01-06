#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "elf_common.h"
#include "ld.h"


void free_all(Elf32_Ehdr *ehdr1, Elf32_Ehdr *ehdr2, Elf32_Shdr **shdr1, Elf32_Shdr **shdr2,
	unsigned nb_sections, Fusion **fusion)
{
	for(int i = 0; i < ehdr1->e_shnum; i++)
		free(shdr1[i]);
	free(shdr1);
	for(int i = 0; i < ehdr2->e_shnum; i++)
		free(shdr2[i]);
	free(shdr2);
	free(ehdr1);
	free(ehdr2);
	for(int i = 0; i < nb_sections; i++)
		free(fusion[i]);
	free(fusion);
}

int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		fprintf(stderr, "%s : FICHIER_ENTRÉE1 FICHIER_ENTRÉE2 FICHIER_SORTIE\n", argv[0]);
		return 1;
	}

	int ind, j;
	int fd1 = open(argv[1], O_RDONLY);
	int fd2 = open(argv[2], O_RDONLY);
	int fd_out = open(argv[3], O_WRONLY | O_CREAT, 0644);
	unsigned nb_sections = 0;

	Elf32_Off offset = 0;
	Elf32_Ehdr *ehdr1 = malloc(sizeof(Elf32_Ehdr));
	Elf32_Ehdr *ehdr2 = malloc(sizeof(Elf32_Ehdr));
	Elf32_Shdr **shdr1, **shdr2;
	Fusion **fusion = NULL;

	read_header(fd1, ehdr1);
	read_header(fd2, ehdr2);

	shdr1 = malloc(sizeof(Elf32_Shdr*) * ehdr1->e_shnum);
	shdr2 = malloc(sizeof(Elf32_Shdr*) * ehdr2->e_shnum);
	for(int i = 0; i < ehdr1->e_shnum; i++)
		shdr1[i] = malloc(sizeof(Elf32_Shdr));
	for(int i = 0; i < ehdr2->e_shnum; i++)
		shdr2[i] = malloc(sizeof(Elf32_Shdr));
	read_section_header(fd1, ehdr1, shdr1);
	read_section_header(fd2, ehdr2, shdr2);

	char *table1 = get_name_table(fd1, ehdr1->e_shstrndx, shdr1);
	char *table2 = get_name_table(fd2, ehdr2->e_shstrndx, shdr2);

	/* On parcours les sections PROGBITS du premier fichier */
	for(int i = 0; i < ehdr1->e_shnum; i++)
	{
		if(shdr1[i]->sh_type != SHT_PROGBITS && shdr1[i]->sh_type != SHT_NOBITS)
			continue;

		/* Recherche si la section est présente dans le second fichier */
		for(j = 0; (j < ehdr2->e_shnum) && strcmp(get_section_name(shdr1, table1, i), get_section_name(shdr2, table2, j)); j++);;

		if(offset == 0)
			offset = shdr1[i]->sh_offset;

		nb_sections++;
		ind = nb_sections - 1;
		fusion = realloc(fusion, sizeof(Fusion*) * nb_sections);
		fusion[ind] = malloc(sizeof(Fusion));
		fusion[ind]->ptr_shdr1 = shdr1[i];

		if(j != ehdr2->e_shnum)
		{
			/* La section est présente dans les deux fichiers */
			fusion[ind]->size = shdr1[i]->sh_size + shdr2[j]->sh_size;
			fusion[ind]->ptr_shdr2 = shdr2[j];
		}
		else
		{
			/* La section est présente uniquement dans le premier fichier */
			fusion[ind]->size = shdr1[i]->sh_size;
			fusion[ind]->ptr_shdr2 = NULL;
		}

		fusion[ind]->offset = offset;
		offset = fusion[ind]->size;
	}

	/* On recherche les sections PROGBITS du second fichier qui ne sont pas présentes dans le premier */
	for(int i = 0; i < ehdr2->e_shnum; i++)
	{
		if(shdr2[i]->sh_type != SHT_PROGBITS && shdr2[i]->sh_type != SHT_NOBITS)
			continue;

		/* Recherche si la section est absente du premier fichier */
		for(j = 0; (j < ehdr1->e_shnum) && strcmp(get_section_name(shdr1, table1, j), get_section_name(shdr2, table2, i)); j++);

		if(j == ehdr1->e_shnum)
		{
			nb_sections++;
			ind = nb_sections - 1;
			fusion = realloc(fusion, sizeof(Fusion*) * nb_sections);
			fusion[ind] = malloc(sizeof(Fusion));
			fusion[ind]->ptr_shdr1 = NULL;
			fusion[ind]->ptr_shdr2 = shdr2[j];
			fusion[ind]->size = shdr2[i]->sh_size;
			fusion[ind]->offset = offset;
			offset += fusion[ind]->size;
		}
	}
	free(table1);
	free(table2);

	/* On crée le fichier de sortie qui contient les sections PROGBITS fusionnées */
	lseek(fd_out, ehdr1->e_ehsize, SEEK_SET);
	for(int i = 0; i < nb_sections; i++)
	{
		if (fusion[i]->ptr_shdr1 != NULL)
		{
			/* On écrit la section du premier fichier */
			write_progbits_in_file(fd1, fd_out, fusion[i]->ptr_shdr1->sh_size, fusion[i]->ptr_shdr1->sh_offset);
			if (fusion[i]->ptr_shdr2 != NULL)
			{
				/* On écrit la section du second fichier */
				write_progbits_in_file(fd2, fd_out, fusion[i]->ptr_shdr2->sh_size, fusion[i]->ptr_shdr2->sh_offset);
			}
		}
		else
		{
			/* On écrit uniquement la section du second fichier */
			write_progbits_in_file(fd2, fd_out, fusion[i]->ptr_shdr2->sh_size, fusion[i]->ptr_shdr2->sh_offset);
		}
	}

	close(fd1);
	close(fd2);
	close(fd_out);
	free_all(ehdr1, ehdr2, shdr1, shdr2, nb_sections, fusion);

	return 0;
}

int write_progbits_in_file(int fd_in, int fd_out, Elf32_Word size, Elf32_Off offset)
{
		ssize_t r, w;
		unsigned char *buff = malloc(sizeof(unsigned char) * size);

		lseek(fd_in, offset, SEEK_SET);
		r = read(fd_in, buff, size);

		w = write(fd_out, buff, size);
		free(buff);

		return (r != w);
}
