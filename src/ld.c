#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "elf_common.h"
#include "symbolTable.h"
#include "ld.h"


int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		fprintf(stderr, "%s : FICHIER_ENTRÉE1 FICHIER_ENTRÉE2 FICHIER_SORTIE\n", argv[0]);
		return 1;
	}

	int ind, j;
	int fd1    = open(argv[1], O_RDONLY);
	int fd2    = open(argv[2], O_RDONLY);
	int fd_out = open(argv[3], O_WRONLY | O_CREAT, 0644);
	unsigned nb_sections   = 0;
	Elf32_Off offset       = 0;
	Elf32_Word symbsize    = 0;
	Elf32_Ehdr *ehdr1      = read_elf_header(fd1);
	Elf32_Ehdr *ehdr2      = read_elf_header(fd2);
	Section_Table *secTab1 = read_sectionTable(fd1, ehdr1);
	Section_Table *secTab2 = read_sectionTable(fd2, ehdr2);
	symbolTable *st1       = read_symbolTable(fd1, ehdr1, secTab1);
	symbolTable *st2       = read_symbolTable(fd2, ehdr2, secTab2);
	symbolTable *st_out    = malloc(sizeof(symbolTable));
	Fusion **fusion        = NULL;

	/* On parcours les sections PROGBITS du premier fichier */
	for(int i = 0; i < ehdr1->e_shnum; i++)
	{
		if(secTab1->shdr[i]->sh_type != SHT_PROGBITS && secTab1->shdr[i]->sh_type != SHT_NOBITS)
			continue;

		/* Recherche si la section est présente dans le second fichier */
		for(j = 0; (j < ehdr2->e_shnum) && strcmp(get_section_name(secTab1->shdr, secTab1->sectionNameTable, i), get_section_name(secTab2->shdr, secTab2->sectionNameTable, j)); j++);;

		if(offset == 0)
			offset = secTab1->shdr[i]->sh_offset;

		nb_sections++;
		ind = nb_sections - 1;
		fusion = realloc(fusion, sizeof(Fusion*) * nb_sections);
		fusion[ind] = malloc(sizeof(Fusion));
		fusion[ind]->ptr_shdr1 = secTab1->shdr[i];

		if(j != ehdr2->e_shnum)
		{
			/* La section est présente dans les deux fichiers */
			fusion[ind]->size = secTab1->shdr[i]->sh_size + secTab2->shdr[j]->sh_size;
			fusion[ind]->ptr_shdr2 = secTab2->shdr[j];
		}
		else
		{
			/* La section est présente uniquement dans le premier fichier */
			fusion[ind]->size = secTab1->shdr[i]->sh_size;
			fusion[ind]->ptr_shdr2 = NULL;
		}

		fusion[ind]->offset = offset;
		offset = fusion[ind]->size;
	}

	/* On recherche les sections PROGBITS du second fichier qui ne sont pas présentes dans le premier */
	for(int i = 0; i < ehdr2->e_shnum; i++)
	{
		if(secTab2->shdr[i]->sh_type != SHT_PROGBITS && secTab2->shdr[i]->sh_type != SHT_NOBITS)
			continue;

		/* Recherche si la section est absente du premier fichier */
		for(j = 0; (j < ehdr1->e_shnum) && strcmp(get_section_name(secTab1->shdr, secTab1->sectionNameTable, j), get_section_name(secTab2->shdr, secTab2->sectionNameTable, i)); j++);

		if(j == ehdr1->e_shnum)
		{
			nb_sections++;
			ind = nb_sections - 1;
			fusion = realloc(fusion, sizeof(Fusion*) * nb_sections);
			fusion[ind] = malloc(sizeof(Fusion));
			fusion[ind]->ptr_shdr1 = NULL;
			fusion[ind]->ptr_shdr2 = secTab2->shdr[j];
			fusion[ind]->size = secTab2->shdr[i]->sh_size;
			fusion[ind]->offset = offset;
			offset += fusion[ind]->size;
		}
	}
	free(secTab1->sectionNameTable);
	free(secTab2->sectionNameTable);

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

	/* Clonage de st1 en st_out */
	symbsize = secTab1->shdr[st1->symtabIndex]->sh_size;
	st_out->nbSymbol = st1->nbSymbol;
	st_out->symbolNameTable = malloc(symbsize);
	memcpy(st_out->symbolNameTable, st1->symbolNameTable, symbsize);
	st_out->symTableName = malloc(strlen(st1->symTableName) + 1);
	strcpy(st_out->symTableName, st1->symTableName);
	st_out->symtab = malloc(sizeof(Elf32_Sym*) * st_out->nbSymbol);
	for(int i = 0; i < st_out->nbSymbol; i++)
	{
		st_out->symtab[i] = malloc(sizeof(Elf32_Sym));
		memcpy(st_out->symtab[i], st1->symtab[i], sizeof(Elf32_Sym));
	}
	//TODO Faire la table dynamique

	/* On remplace les caractères '\0' par des espaces afin de pouvoir utiliser des fonctions de manipulation de chaines */
	for(int i = 0; i < symbsize; i++)
		if(st_out->symbolNameTable[i] == '\0')
			st_out->symbolNameTable[i] = ' ';
	st_out->symbolNameTable[symbsize] = '\0';

	for(int i = 1; i < st2->nbSymbol; i++)
	{
		char *buff = get_symbol_name(st2->symtab, st2->symbolNameTable, i);

		if(ELF32_ST_BIND(st2->symtab[i]->st_info) != STB_GLOBAL)
		{
			ind = add_symbol_in_table(st_out, st2->symtab[i]);
			if(strlen(buff) > 0)
			{
				/* On recherche si le symbole est déjà présent dans la nouvelle table */
				if(strstr(st_out->symbolNameTable, buff) == NULL)
				{
					/* On ajoute le symbole à la nouvelle table */
					st_out->symtab[ind]->st_name = symbsize;
					add_symbol_in_name_table(&st_out->symbolNameTable, buff, &symbsize);
				}
				else
				{
					/* On calcule l'indice correspondant au symbole déjà présent */
					st_out->symtab[ind]->st_name = strstr(st_out->symbolNameTable, buff) - st_out->symbolNameTable;
				}
			}
		}
		else
		{
			/* Symbole global */
			if(strlen(buff) == 0)
			{
				/* On ajoute le symbole à la nouvelle table */
				ind = add_symbol_in_table(st_out, st2->symtab[i]);
				st_out->symtab[ind]->st_name = symbsize;
				add_symbol_in_name_table(&st_out->symbolNameTable, buff, &symbsize);
			}
			else
			{
				/* On recherche si le symbole est déjà présent dans la nouvelle table */
				for(ind = 0; (ind < st1->nbSymbol) && strcmp(get_symbol_name(st1->symtab, st1->symbolNameTable, ind), buff); ind++);

				if(strstr(st_out->symbolNameTable, buff) == NULL)
				{
					/* On ajoute le symbole à la nouvelle table */
					ind = add_symbol_in_table(st_out, st2->symtab[i]);
					st_out->symtab[ind]->st_name = symbsize;
					add_symbol_in_name_table(&st_out->symbolNameTable, buff, &symbsize);
				}
				else if(st_out->symtab[ind]->st_shndx == SHN_UNDEF && st2->symtab[ind]->st_shndx != SHN_UNDEF)
				{
					/* On remplace le symbole indéfini par le défini, en conservant st_name */
					Elf32_Word name = st_out->symtab[ind]->st_name;
					memcpy(st_out->symtab[ind], st2->symtab[i], sizeof(Elf32_Sym));
					st_out->symtab[ind]->st_name = name;
				}
				else
				{
					/* Un symbole global est défini deux fois, on abandonne... */
					remove(argv[3]);
					fprintf(stderr, "%s : le symbole « %s » est défini plus d'une fois !\n", argv[0], buff);
					return 2;
				}
			}
		}
	}

	/* On remet les caractères '\0' à la place des espaces */
	for(int i = 0; i < symbsize; i++)
		if(st_out->symbolNameTable[i] == ' ')
			st_out->symbolNameTable[i] = '\0';

	// Pour debug
	dump_symtab(st_out->nbSymbol, st_out->symtab, st_out->symbolNameTable, st_out->symTableName);

	close(fd1);
	close(fd2);
	close(fd_out);
	destroy_elf_header(ehdr1);
	destroy_elf_header(ehdr2);
	destroy_sectionTable(secTab1);
	destroy_sectionTable(secTab2);
	destroy_symbolTable(st1);
	destroy_symbolTable(st1);
	for(int i = 0; i < nb_sections; i++)
		free(fusion[i]);
	free(fusion);

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

int add_symbol_in_table(symbolTable *st, Elf32_Sym *symtab)
{
	int ind = st->nbSymbol;

	st->nbSymbol++;
	st->symtab = realloc(st->symtab, sizeof(Elf32_Sym*) * st->nbSymbol);
	st->symtab[ind] = malloc(sizeof(Elf32_Sym));
	memcpy(st->symtab[ind], symtab, sizeof(Elf32_Sym));

	return ind;
}

void add_symbol_in_name_table(char **symbolNameTable, char *symbol, Elf32_Word *size)
{
	*size += strlen(symbol) + 1;
	*symbolNameTable = realloc(*symbolNameTable, *size);
	strcat(*symbolNameTable, symbol);
	(*symbolNameTable)[*size - 1] = ' ';
}
