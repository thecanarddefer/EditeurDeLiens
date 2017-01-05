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

/**************************** ÉTAPE 1 ****************************
*                     Affichage de l’en-tête
*****************************************************************/

int read_header(int fd, Elf32_Ehdr *ehdr)
{
	read(fd, ehdr->e_ident, EI_NIDENT);
	if(ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 || ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3 || ehdr->e_ident[EI_CLASS] != ELFCLASS32)
	{
		fprintf(stderr, "Le fichier n'est pas de type ELF32.\n");
		exit(3);
	}

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

	return 0;
}


/**************************** ÉTAPE 2 ****************************
*                Affichage de la table des sections
*****************************************************************/

int read_section_header(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr)
{
	for(int i = 0; i < ehdr->e_shnum; i++)
	{
		lseek(fd, ehdr->e_shoff + i * ehdr->e_shentsize, SEEK_SET);
		read(fd, &shdr[i]->sh_name,      sizeof(shdr[i]->sh_name));
		read(fd, &shdr[i]->sh_type,      sizeof(shdr[i]->sh_type));
		read(fd, &shdr[i]->sh_flags,     sizeof(shdr[i]->sh_flags));
		read(fd, &shdr[i]->sh_addr,      sizeof(shdr[i]->sh_addr));
		read(fd, &shdr[i]->sh_offset,    sizeof(shdr[i]->sh_offset));
		read(fd, &shdr[i]->sh_size,      sizeof(shdr[i]->sh_size));
		read(fd, &shdr[i]->sh_link,      sizeof(shdr[i]->sh_link));
		read(fd, &shdr[i]->sh_info,      sizeof(shdr[i]->sh_info));
		read(fd, &shdr[i]->sh_addralign, sizeof(shdr[i]->sh_addralign));
		read(fd, &shdr[i]->sh_entsize,   sizeof(shdr[i]->sh_entsize));
	}

	return 0;
}

//TODO: char *get_name_table(int fd, Elf32_Word tableIndx, Elf32_shdr)
char *get_section_name_table(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr)
{
	int pos;
	const unsigned offset = shdr[ehdr->e_shstrndx]->sh_offset;
	char *table = (char *) malloc(sizeof(char) * offset);

	pos = lseek(fd, 0, SEEK_CUR);
	lseek(fd, offset, SEEK_SET);
	read(fd, table, offset);

	lseek(fd, pos, SEEK_SET);
	return table;
}

char *get_section_name(Elf32_Shdr **shdr, char *table, unsigned index)
{
	return &(table[shdr[index]->sh_name]);
}


/**************************** ÉTAPE 4 ****************************
*                  Affichage de la table des symboles
*****************************************************************/

// TODO: void get_section_index(int nbSections, Elf32_Shdr **shdr, int <SHT_SECTION>)

void get_symtab_index(int nbSections, Elf32_Shdr **shdr, int *idxSymTab, int *idxStrTab) {
	int i = 0;
	while (i < nbSections) {
		if (shdr[i]->sh_type == SHT_SYMTAB || shdr[i]->sh_type == SHT_DYNSYM) {
			*idxSymTab = i;
		}
		else if (shdr[i]->sh_type == SHT_STRTAB)
		{
			*idxStrTab = i;
		}
		i++;
	}
}

// void read_symbol(int fd, Elf32_Sym *symtab) {
// 	read(fd, &symtab->st_name, sizeof(symtab->st_name));
// 	read(fd, &symtab->st_value, sizeof(symtab->st_value));
// 	read(fd, &symtab->st_size, sizeof(symtab->st_size));
// 	read(fd, &symtab->st_info, sizeof(symtab->st_info));
// 	read(fd, &symtab->st_other, sizeof(symtab->st_other));
// 	read(fd, &symtab->st_shndx, sizeof(symtab->st_shndx));
// }

Elf32_Sym **read_symtab(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, int *nbElt, int *idxStrTab) {
	int i = -1;
	Elf32_Sym **symtab;

	get_symtab_index(ehdr->e_shnum,shdr, &i, idxStrTab);

	if(i != -1) {
		*nbElt = shdr[i]->sh_size / shdr[i]->sh_entsize; // Nombre de symboles dans la table.

		symtab = malloc(*nbElt * sizeof(Elf32_Sym*));
		for (int j = 0; j < *nbElt; ++j) {
			symtab[j] = malloc(sizeof(Elf32_Sym));

			lseek(fd, shdr[i]->sh_offset + j * shdr[i]->sh_entsize, SEEK_SET);

			read(fd, &symtab[j]->st_name, sizeof(symtab[j]->st_name));
			read(fd, &symtab[j]->st_value, sizeof(symtab[j]->st_value));
			read(fd, &symtab[j]->st_size, sizeof(symtab[j]->st_size));
			read(fd, &symtab[j]->st_info, sizeof(symtab[j]->st_info));
			read(fd, &symtab[j]->st_other, sizeof(symtab[j]->st_other));
			read(fd, &symtab[j]->st_shndx, sizeof(symtab[j]->st_shndx));
		}
	}
	return symtab;
}

char *get_symbol_name_table(int fd, int idxStrTab, Elf32_Shdr **shdr) {
	int pos;
	const unsigned offset = shdr[idxStrTab]->sh_offset;
	char *table = (char *) malloc(sizeof(char) * offset);

	pos = lseek(fd, 0, SEEK_CUR);
	lseek(fd, offset, SEEK_SET);
	read(fd, table, shdr[idxStrTab]->sh_size);

	lseek(fd, pos, SEEK_SET);
	return table;
}

char *get_symbol_name(Elf32_Sym **symtab, char *table, unsigned index){
	return &(table[symtab[index]->st_name]);
}


/**************************** ÉTAPE 5 ****************************
*               Affichage des tables de réimplantation
*****************************************************************/

int read_relocation_header(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, Data_Rel *drel)
{
	unsigned ind, size;

	for(int i = 0; i < ehdr->e_shnum; i++)
	{
		lseek(fd, shdr[i]->sh_offset, SEEK_SET);
		if(shdr[i]->sh_type == SHT_REL)
		{
			drel->nb_rel++;
			ind  = drel->nb_rel - 1;
			size = shdr[i]->sh_size / sizeof(Elf32_Rel);

			/* Allocations */
			drel->e_rel    = realloc(drel->e_rel, sizeof(unsigned) * drel->nb_rel);
			drel->a_rel    = realloc(drel->a_rel, sizeof(Elf32_Addr) * drel->nb_rel);
			drel->rel      = realloc(drel->rel,   sizeof(Elf32_Rel*) * drel->nb_rel);
			drel->rel[ind] = malloc(sizeof(Elf32_Rel*) * size);
			for(int j = 0; j < size; j++)
				drel->rel[ind][j] = malloc(sizeof(Elf32_Rel));

			drel->e_rel[ind] = size;
			drel->a_rel[ind] = shdr[i]->sh_offset;

			/* Récupération de la table des réimplantations */
			for(int j = 0; j < size; j++)
			{
				read(fd, &drel->rel[ind][j]->r_offset, sizeof(drel->rel[ind][j]->r_offset));
				read(fd, &drel->rel[ind][j]->r_info,   sizeof(drel->rel[ind][j]->r_info));
			}
		}
		else if(shdr[i]->sh_type == SHT_RELA)
		{
			drel->nb_rela++;
			ind  = drel->nb_rela - 1;
			size = shdr[i]->sh_size / sizeof(Elf32_Rela);

			/* Allocations */
			drel->e_rela    = realloc(drel->e_rela, sizeof(unsigned) * drel->nb_rela);
			drel->a_rela    = realloc(drel->a_rela, sizeof(Elf32_Addr) * drel->nb_rela);
			drel->rela      = realloc(drel->rela,   sizeof(Elf32_Rela*) * drel->nb_rela);
			drel->rela[ind] = malloc(sizeof(Elf32_Rela*) * size);
			for(int j = 0; j < size; j++)
				drel->rela[ind][j] = malloc(sizeof(Elf32_Rela));

			drel->e_rela[ind] = size;
			drel->a_rela[ind] = shdr[i]->sh_offset;

			/* Récupération de la table des réimplantations */
			for(int j = 0; j < size; j++)
			{
				read(fd, &drel->rela[ind][j]->r_offset, sizeof(drel->rela[ind][j]->r_offset));
				read(fd, &drel->rela[ind][j]->r_info,   sizeof(drel->rela[ind][j]->r_info));
				read(fd, &drel->rela[ind][j]->r_addend, sizeof(drel->rela[ind][j]->r_addend));
			}
		}
	}

	return 0;
}
