#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "elf_common.h"
#include "symbolTable.h"
#include "fusion.h"


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
	unsigned nb_sections   = 1;
	Elf32_Off offset       = 0;
	Elf32_Ehdr *ehdr1      = read_elf_header(fd1);
	Elf32_Ehdr *ehdr2      = read_elf_header(fd2);
	Section_Table *secTab1 = read_sectionTable(fd1, ehdr1);
	Section_Table *secTab2 = read_sectionTable(fd2, ehdr2);
	symbolTable *st1       = read_symbolTable(fd1, ehdr1, secTab1);
	symbolTable *st2       = read_symbolTable(fd2, ehdr2, secTab2);
	symbolTable *st_out    = read_symbolTable(fd1, ehdr1, secTab1); // En réalité, on duplique la table des symboles du premier fichier
	Elf32_Word symbsize    = secTab1->shdr[st1->symtabIndex]->sh_size;
	Fusion **fusion        = NULL;

	/* On parcours les sections PROGBITS du premier fichier */
	for(int i = 0; i < secTab1->nb_sections; i++)
	{
		if(secTab1->shdr[i]->sh_type != SHT_PROGBITS && secTab1->shdr[i]->sh_type != SHT_NOBITS)
			continue;

		/* Recherche si la section est présente dans le second fichier */
		for(j = 0; (j < secTab2->nb_sections) && strcmp(get_section_name(secTab1->shdr, secTab1->sectionNameTable, i), get_section_name(secTab2->shdr, secTab2->sectionNameTable, j)); j++);;

		if(offset == 0)
			offset = secTab1->shdr[i]->sh_offset;

		nb_sections++;
		ind = nb_sections - 1;
		fusion = realloc(fusion, sizeof(Fusion*) * nb_sections);
		fusion[ind] = malloc(sizeof(Fusion));
		fusion[ind]->ptr_shdr1 = secTab1->shdr[i];

		if(j != secTab2->nb_sections)
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
		strcpy(fusion[ind]->section, get_section_name(secTab1->shdr, secTab1->sectionNameTable, i));
		offset = fusion[ind]->size;
	}

	/* On recherche les sections PROGBITS du second fichier qui ne sont pas présentes dans le premier */
	for(int i = 0; i < secTab2->nb_sections; i++)
	{
		if(secTab2->shdr[i]->sh_type != SHT_PROGBITS && secTab2->shdr[i]->sh_type != SHT_NOBITS)
			continue;

		/* Recherche si la section est absente du premier fichier */
		for(j = 0; (j < secTab1->nb_sections) && strcmp(get_section_name(secTab1->shdr, secTab1->sectionNameTable, j), get_section_name(secTab2->shdr, secTab2->sectionNameTable, i)); j++);

		if(j == secTab1->nb_sections)
		{
			nb_sections++;
			ind = nb_sections - 1;
			fusion = realloc(fusion, sizeof(Fusion*) * nb_sections);
			fusion[ind] = malloc(sizeof(Fusion));
			fusion[ind]->ptr_shdr1 = NULL;
			fusion[ind]->ptr_shdr2 = secTab2->shdr[i];
			fusion[ind]->size = secTab2->shdr[i]->sh_size;
			fusion[ind]->offset = offset;
			strcpy(fusion[ind]->section, get_section_name(secTab2->shdr, secTab2->sectionNameTable, i));
			offset += fusion[ind]->size;
		}
	}

	/* On crée le fichier de sortie qui contient les sections PROGBITS fusionnées */
	lseek(fd_out, ehdr1->e_ehsize, SEEK_SET);
	for(int i = 1; i < nb_sections; i++)
	{
		if(fusion[i]->ptr_shdr1 != NULL)
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

	/* On met à jour l'indice de section des symboles du premier fichier (à ce stade là, st_out = st1) */
	for(int i = 1; i < st_out->nbSymbol; i++)
		update_section_index_in_symbol(fusion, nb_sections, secTab1, st_out->symtab[i]);

	/* On remplace les caractères '\0' par des espaces afin de pouvoir utiliser des fonctions de manipulation de chaines */
	for(int i = 0; i < symbsize; i++)
		if(st_out->symbolNameTable[i] == '\0')
			st_out->symbolNameTable[i] = ' ';
	st_out->symbolNameTable[symbsize] = '\0';

	for(int i = 1; i < st2->nbSymbol; i++)
	{
		char *buff = get_symbol_name(st2->symtab, st2->symbolNameTable, i);

		if((strlen(buff) > 0) && (strstr(st_out->symbolNameTable, buff) != NULL))
		{
			/* On calcule l'indice correspondant au symbole déjà présent en se basant sur son nom */
			for(j = 0; (j < st_out->nbSymbol) && strcmp(get_symbol_name(st_out->symtab, st1->symbolNameTable, j), buff); j++);
			ind = (j < st_out->nbSymbol) ? j : nb_sections - 1;

			const int is_global_st_out = (ELF32_ST_BIND(st_out->symtab[ind]->st_info) == STB_GLOBAL);
			const int is_global_st2    = (ELF32_ST_BIND(st2->symtab[i]->st_info)      == STB_GLOBAL);

			if(is_global_st_out && is_global_st2 && (st_out->symtab[ind]->st_shndx != SHN_UNDEF) && (st2->symtab[i]->st_shndx != SHN_UNDEF))
			{
				/* Un symbole global est défini deux fois, on abandonne... */
				remove(argv[3]);
				fprintf(stderr, "%s : le symbole « %s » est défini plus d'une fois !\n", argv[0], buff);
				return 2;
			}
			else if(is_global_st_out && is_global_st2 && (st_out->symtab[ind]->st_shndx == SHN_UNDEF) && (st2->symtab[i]->st_shndx != SHN_UNDEF))
			{
				/* On remplace le symbole global indéfini par le défini, sans toucher à st_name et st_shndx */
				st_out->symtab[ind]->st_value = st2->symtab[i]->st_value;
				st_out->symtab[ind]->st_size  = st2->symtab[i]->st_size;
				st_out->symtab[ind]->st_info  = st2->symtab[i]->st_info;
				st_out->symtab[ind]->st_other = st2->symtab[i]->st_other;
			}
			else
			{
				/* Un autre symbole local a le même nom, on ajoute le symbole à la table des symboles uniquement */
				ind = add_symbol_in_table(st_out, st2->symtab[i]);
				st_out->symtab[ind]->st_name = strstr(st_out->symbolNameTable, buff) - st_out->symbolNameTable;
				update_section_index_in_symbol(fusion, nb_sections, secTab2, st_out->symtab[ind]);
			}
		}
		else
		{
			j = secTab1->nb_sections;
			if((ELF32_ST_BIND(st2->symtab[i]->st_info) == STB_LOCAL) && st2->symtab[i]->st_shndx < secTab2->nb_sections)
			{
				for(j = 0; (j < secTab1->nb_sections) && strcmp(get_section_name(secTab1->shdr, secTab1->sectionNameTable, j),
					get_section_name(secTab2->shdr, secTab2->sectionNameTable, st2->symtab[i]->st_shndx)); j++);
			}

			if(j == secTab1->nb_sections)
			{
				/* On ajoute le symbole à la nouvelle table */
				ind = add_symbol_in_table(st_out, st2->symtab[i]);
				st_out->symtab[ind]->st_name = symbsize;
				add_symbol_in_name_table(&st_out->symbolNameTable, buff, &symbsize);
				update_section_index_in_symbol(fusion, nb_sections, secTab2, st_out->symtab[ind]);
			}
		}
	}

	/* On remet les caractères '\0' à la place des espaces */
	for(int i = 0; i < symbsize; i++)
		if(st_out->symbolNameTable[i] == ' ')
			st_out->symbolNameTable[i] = '\0';

	sort_new_symbol_table(st_out);
	dump_symtab(st_out->nbSymbol, st_out->symtab, st_out->symbolNameTable, st_out->symTableName); // Pour debug

	close(fd1);
	close(fd2);
	close(fd_out);
	destroy_elf_header(ehdr1);
	destroy_elf_header(ehdr2);
	destroy_sectionTable(secTab1);
	destroy_sectionTable(secTab2);
	destroy_symbolTable(st1);
	destroy_symbolTable(st2);
	destroy_symbolTable(st_out);
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

void update_section_index_in_symbol(Fusion **fusion, unsigned nb_sections, Section_Table *secTab, Elf32_Sym *symbol)
{
	int i;
	if(symbol->st_shndx >= secTab->nb_sections)
		return;

	for(i = 1; (i < nb_sections) && strcmp(fusion[i]->section, get_section_name(secTab->shdr, secTab->sectionNameTable, symbol->st_shndx)); i++);
	if(i < nb_sections)
		symbol->st_shndx = i;
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

static inline void swap_symbols(Elf32_Sym *sym1, Elf32_Sym *sym2)
{
	Elf32_Sym *tmp = malloc(sizeof(Elf32_Sym));
	memcpy(tmp, sym2, sizeof(Elf32_Sym));
	memcpy(sym2, sym1, sizeof(Elf32_Sym));
	memcpy(sym1, tmp, sizeof(Elf32_Sym));
	free(tmp);
}

void sort_new_symbol_table(symbolTable *st)
{
	int j, cpt = 0;

	for(int i = 1; i < st->nbSymbol; i++)
	{
		if(ELF32_ST_TYPE(st->symtab[i]->st_info) == STT_SECTION)
		{
			cpt++;
			continue;
		}

		for(j = i; (j < st->nbSymbol) && ELF32_ST_TYPE(st->symtab[j]->st_info) != STT_SECTION; j++);
		if(j == st->nbSymbol)
			continue;

		swap_symbols(st->symtab[i], st->symtab[j]);
		cpt++;
	}

	for(int i = cpt; i < st->nbSymbol; i++)
	{
		if(ELF32_ST_BIND(st->symtab[i]->st_info) != STB_GLOBAL)
			continue;

		for(j = st->nbSymbol - 1; (j >= i) && ELF32_ST_BIND(st->symtab[j]->st_info) == STB_GLOBAL; j--);
		if(j < i)
			continue;

		swap_symbols(st->symtab[i], st->symtab[j]);
	}
}
