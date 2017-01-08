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

Elf32_Ehdr *read_elf_header(int fd)
{
	Elf32_Ehdr *ehdr = malloc(sizeof(Elf32_Ehdr));

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

	return ehdr;
}

void destroy_elf_header(Elf32_Ehdr *ehdr)
{
	free(ehdr);
}


/**************************** ÉTAPE 2 ****************************
*                Affichage de la table des sections
*****************************************************************/

Section_Table *read_sectionTable(int fd, Elf32_Ehdr *ehdr)
{
	Section_Table *secTab = malloc(sizeof(Section_Table));
	secTab->shdr = malloc(sizeof(Elf32_Shdr*) * ehdr->e_shnum);

	for(int i = 0; i < ehdr->e_shnum; i++)
	{
		secTab->shdr[i] = malloc(sizeof(Elf32_Shdr));
		lseek(fd, ehdr->e_shoff + i * ehdr->e_shentsize, SEEK_SET);
		read(fd, &secTab->shdr[i]->sh_name,      sizeof(secTab->shdr[i]->sh_name));
		read(fd, &secTab->shdr[i]->sh_type,      sizeof(secTab->shdr[i]->sh_type));
		read(fd, &secTab->shdr[i]->sh_flags,     sizeof(secTab->shdr[i]->sh_flags));
		read(fd, &secTab->shdr[i]->sh_addr,      sizeof(secTab->shdr[i]->sh_addr));
		read(fd, &secTab->shdr[i]->sh_offset,    sizeof(secTab->shdr[i]->sh_offset));
		read(fd, &secTab->shdr[i]->sh_size,      sizeof(secTab->shdr[i]->sh_size));
		read(fd, &secTab->shdr[i]->sh_link,      sizeof(secTab->shdr[i]->sh_link));
		read(fd, &secTab->shdr[i]->sh_info,      sizeof(secTab->shdr[i]->sh_info));
		read(fd, &secTab->shdr[i]->sh_addralign, sizeof(secTab->shdr[i]->sh_addralign));
		read(fd, &secTab->shdr[i]->sh_entsize,   sizeof(secTab->shdr[i]->sh_entsize));
	}

	secTab->nb_sections      = ehdr->e_shnum;
	secTab->sectionNameTable = get_name_table(fd, ehdr->e_shstrndx, secTab->shdr);

	return secTab;
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

void destroy_sectionTable(Section_Table *secTab)
{
	for(int i = 0; i < secTab->nb_sections; i++)
		free(secTab->shdr[i]);
	free(secTab->shdr);
	free(secTab->sectionNameTable);
	free(secTab);
}


/**************************** ÉTAPE 5 ****************************
*               Affichage des tables de réimplantation
*****************************************************************/

Data_Rel *read_relocationTables(int fd, Section_Table *secTab)
{
	unsigned ind, size;
	Data_Rel *drel = malloc(sizeof(Data_Rel));

	/* Initialisation */
	drel->nb_rel  = 0;
	drel->nb_rela = 0;
	drel->e_rel   = NULL;
	drel->e_rela  = NULL;
	drel->a_rel   = NULL;
	drel->a_rela  = NULL;
	drel->i_rel   = NULL;
	drel->i_rela  = NULL;
	drel->rel     = NULL;
	drel->rela    = NULL;

	for(int i = 0; i < secTab->nb_sections; i++)
	{
		lseek(fd, secTab->shdr[i]->sh_offset, SEEK_SET);
		if(secTab->shdr[i]->sh_type == SHT_REL)
		{
			drel->nb_rel++;
			ind  = drel->nb_rel - 1;
			size = secTab->shdr[i]->sh_size / sizeof(Elf32_Rel);

			/* Allocations */
			drel->e_rel    = realloc(drel->e_rel, sizeof(unsigned)   * drel->nb_rel);
			drel->a_rel    = realloc(drel->a_rel, sizeof(Elf32_Addr) * drel->nb_rel);
			drel->i_rel    = realloc(drel->i_rel, sizeof(unsigned)   * drel->nb_rel);
			drel->rel      = realloc(drel->rel,   sizeof(Elf32_Rel*) * drel->nb_rel);
			drel->rel[ind] = malloc(sizeof(Elf32_Rel*) * size);
			for(int j = 0; j < size; j++)
				drel->rel[ind][j] = malloc(sizeof(Elf32_Rel));

			drel->e_rel[ind] = size;
			drel->a_rel[ind] = secTab->shdr[i]->sh_offset;
			drel->i_rel[ind] = i;

			/* Récupération de la table des réimplantations */
			for(int j = 0; j < size; j++)
			{
				read(fd, &drel->rel[ind][j]->r_offset, sizeof(drel->rel[ind][j]->r_offset));
				read(fd, &drel->rel[ind][j]->r_info,   sizeof(drel->rel[ind][j]->r_info));
			}
		}
		else if(secTab->shdr[i]->sh_type == SHT_RELA)
		{
			drel->nb_rela++;
			ind  = drel->nb_rela - 1;
			size = secTab->shdr[i]->sh_size / sizeof(Elf32_Rela);

			/* Allocations */
			drel->e_rela    = realloc(drel->e_rela, sizeof(unsigned)    * drel->nb_rela);
			drel->a_rela    = realloc(drel->a_rela, sizeof(Elf32_Addr)  * drel->nb_rela);
			drel->i_rela    = realloc(drel->i_rela, sizeof(unsigned)    * drel->nb_rela);
			drel->rela      = realloc(drel->rela,   sizeof(Elf32_Rela*) * drel->nb_rela);
			drel->rela[ind] = malloc(sizeof(Elf32_Rela*) * size);
			for(int j = 0; j < size; j++)
				drel->rela[ind][j] = malloc(sizeof(Elf32_Rela));

			drel->e_rela[ind] = size;
			drel->a_rela[ind] = secTab->shdr[i]->sh_offset;
			drel->i_rela[ind] = i;

			/* Récupération de la table des réimplantations */
			for(int j = 0; j < size; j++)
			{
				read(fd, &drel->rela[ind][j]->r_offset, sizeof(drel->rela[ind][j]->r_offset));
				read(fd, &drel->rela[ind][j]->r_info,   sizeof(drel->rela[ind][j]->r_info));
				read(fd, &drel->rela[ind][j]->r_addend, sizeof(drel->rela[ind][j]->r_addend));
			}
		}
	}

	return drel;
}

void destroy_relocationTables(Data_Rel *drel)
{
	for(int i = 0; i < drel->nb_rel; i++)
	{
		for(int j = 0; j < drel->e_rel[i]; j++)
			free(drel->rel[i][j]);
		free(drel->rel[i]);
	}
	free(drel->rel);
	for(int i = 0; i < drel->nb_rela; i++)
	{
		for(int j = 0; j < drel->e_rela[i]; j++)
			free(drel->rela[i][j]);
		free(drel->rela[i]);
	}
	free(drel->rela);
	free(drel->e_rel);
	free(drel->e_rela);
	free(drel->a_rel);
	free(drel->a_rela);
	free(drel->i_rel);
	free(drel->i_rela);
	free(drel);
}
