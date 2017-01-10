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

	int ind, j, err = 0;
	int fd1    = open(argv[1], O_RDONLY);
	int fd2    = open(argv[2], O_RDONLY);
	int fd_out = open(argv[3], O_WRONLY | O_CREAT, 0644);
	if(fd1 < 0 || fd2 < 0 || fd_out < 0)
	{
		fprintf(stderr, "%s : Impossible d'ouvrir un fichier.\n", argv[0]);
		return 2;
	}

	unsigned nb_sections   = 1;
	Elf32_Off offset       = 0;
	Elf32_Ehdr *ehdr1      = read_elf_header(fd1);
	Elf32_Ehdr *ehdr2      = read_elf_header(fd2);
	Section_Table *secTab1 = read_sectionTable(fd1, ehdr1);
	Section_Table *secTab2 = read_sectionTable(fd2, ehdr2);
	symbolTable *st1       = read_symbolTable(fd1, secTab1);
	symbolTable *st2       = read_symbolTable(fd2, secTab2);
	Symtab_Struct *st_out    = read_symtab_struct(fd1, secTab1, SHT_SYMTAB); // En réalité, on duplique la table des symboles du premier fichier
	// symbolTable st_out est maintenant une Symtab_Struct (plus à propos puisque aucune action sur SymbolTable->dynsym)
	Data_Rel *drel1        = read_relocationTables(fd1, secTab1);
	Data_Rel *drel2        = read_relocationTables(fd2, secTab2);
	Data_Rel *drel_out     = read_relocationTables(fd1, secTab1);
	Elf32_Word symbsize    = secTab1->shdr[st1->symtab->strIndex]->sh_size;
	Fusion **fusion        = NULL;

	/* On parcours les sections PROGBITS du premier fichier */
	for(int i = 0; i < secTab1->nb_sections; i++)
	{
		if(secTab1->shdr[i]->sh_type != SHT_PROGBITS && secTab1->shdr[i]->sh_type != SHT_NOBITS)
			continue;

		/* Recherche si la section est présente dans le second fichier */
		for(j = 0; (j < secTab2->nb_sections) && strcmp(get_section_name(secTab1, i), get_section_name(secTab2, j)); j++);

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
		strcpy(fusion[ind]->section, get_section_name(secTab1, i));
		offset = fusion[ind]->size;
	}

	/* On recherche les sections PROGBITS du second fichier qui ne sont pas présentes dans le premier */
	for(int i = 0; i < secTab2->nb_sections; i++)
	{
		if(secTab2->shdr[i]->sh_type != SHT_PROGBITS && secTab2->shdr[i]->sh_type != SHT_NOBITS)
			continue;

		/* Recherche si la section est absente du premier fichier */
		for(j = 0; (j < secTab1->nb_sections) && strcmp(get_section_name(secTab1, j), get_section_name(secTab2, i)); j++);

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
			strcpy(fusion[ind]->section, get_section_name(secTab2, i));
			offset += fusion[ind]->size;
		}
	}

	//TODO: Calculer l'indice des nouvelles sections (toutes celles qui ne sont pas des PROGBITS)

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
		update_section_index_in_symbol(fusion, nb_sections, secTab1, st_out->tab[i]);

	for(int i = 1; i < st2->symtab->nbSymbol; i++)
	{

		char *buff = get_static_symbol_name(st2, i);
		const int shndx = find_index_in_name_table(st_out->symbolNameTable, buff, symbsize);

		if(shndx > 0)
		{
			/* On calcule l'indice correspondant au symbole déjà présent en se basant sur son nom */
			for(j = 0; (j < st_out->nbSymbol) && strcmp(get_symbol_name(st_out->tab, st_out->symbolNameTable, j), buff); j++);
			ind = j;
			const int is_global_st_out = (ELF32_ST_BIND(st_out->tab[ind]->st_info) == STB_GLOBAL);
			const int is_global_st2    = (ELF32_ST_BIND(st2->symtab->tab[i]->st_info)      == STB_GLOBAL);

			if(is_global_st_out && is_global_st2 && (st_out->tab[ind]->st_shndx != SHN_UNDEF) && (st2->symtab->tab[i]->st_shndx != SHN_UNDEF))
			{
				/* Un symbole global est défini deux fois, on abandonne... */
				remove(argv[3]);
				fprintf(stderr, "%s : le symbole « %s » est défini plus d'une fois !\n", argv[0], buff);
				err = 3;
				goto clean;
			}
			else if(is_global_st_out && is_global_st2 && (st_out->tab[ind]->st_shndx == SHN_UNDEF) && (st2->symtab->tab[i]->st_shndx != SHN_UNDEF))
			{
				/* On remplace le symbole global indéfini par le défini, sans toucher à st_name */
				st_out->tab[ind]->st_value = st2->symtab->tab[i]->st_value;
				st_out->tab[ind]->st_size  = st2->symtab->tab[i]->st_size;
				st_out->tab[ind]->st_info  = st2->symtab->tab[i]->st_info;
				st_out->tab[ind]->st_other = st2->symtab->tab[i]->st_other;
				st_out->tab[ind]->st_shndx = st2->symtab->tab[i]->st_shndx;
				update_section_index_in_symbol(fusion, nb_sections, secTab2, st_out->tab[ind]);
			}
			else if(!is_global_st_out && !is_global_st2)
			{
				/* Un autre symbole local a le même nom, on ajoute le symbole à la table des symboles uniquement */
				ind = add_symbol_in_table(st_out, st2->symtab->tab[i]);
				st_out->tab[ind]->st_name = shndx;
				update_section_index_in_symbol(fusion, nb_sections, secTab2, st_out->tab[ind]);

				/* On met à jour la valeur du nouveau symbole */
				for(j = 0; (j < secTab1->nb_sections) && strcmp(get_section_name(secTab1, j),
					get_section_name(secTab2, st2->symtab->tab[i]->st_shndx)); j++);
				if(j < secTab1->nb_sections)
					st_out->tab[ind]->st_value += secTab1->shdr[j]->sh_size;
			}
		}
		else
		{
			j = secTab1->nb_sections;
			if((ELF32_ST_BIND(st2->symtab->tab[i]->st_info) == STB_LOCAL) && st2->symtab->tab[i]->st_shndx < secTab2->nb_sections)
			{
				/* On cherche s'il s'agit d'un symbole de section et qu'il n'est pas déjà défini */
				for(j = 0; (j < secTab1->nb_sections) && strcmp(get_section_name(secTab1, j),
					get_section_name(secTab2, st2->symtab->tab[i]->st_shndx)); j++);
			}

			if(j == secTab1->nb_sections)
			{
				/* On ajoute le symbole à la nouvelle table */
				ind = add_symbol_in_table(st_out, st2->symtab->tab[i]);
				update_section_index_in_symbol(fusion, nb_sections, secTab2, st_out->tab[ind]);

				if(strlen(buff) > 0)
				{
					st_out->tab[ind]->st_name = symbsize;
					st_out->symbolNameTable = realloc(st_out->symbolNameTable, symbsize + strlen(buff) + 1);
					strcat(&(st_out->symbolNameTable[symbsize]), buff);
					symbsize += strlen(buff) + 1;
				}
			}
		}
	}

	//TODO: Tables des réimplantations
	/*for(int i = 0; i < drel2->nb_rel; i++)
	{
		printf("==> Section REL %i :\n", i);
		for(r = 0; r < drel2->e_rel[i]; r++)
		{
			printf("Réimplantations %i : %i %i\n", j, ELF32_R_TYPE(drel2->rel[i][r]->r_info), ELF32_R_SYM(drel2->rel[i][r]->r_info));
		}
		printf("\n");
	}*/

	sort_new_symbol_table(st_out);
	dump_symtab(st_out); // Pour debug

clean:
	close(fd1);
	close(fd2);
	close(fd_out);
	destroy_elf_header(ehdr1);
	destroy_elf_header(ehdr2);
	destroy_sectionTable(secTab1);
	destroy_sectionTable(secTab2);
	destroy_symbolTable(st1);
	destroy_symbolTable(st2);
	destroy_symtab_struct(st_out);
	destroy_relocationTables(drel1);
	destroy_relocationTables(drel2);
	destroy_relocationTables(drel_out);

	for(int i = 0; i < nb_sections; i++)
		free(fusion[i]);
	free(fusion);

	return err;
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
	for(i = 1; (i < nb_sections) && strcmp(fusion[i]->section, get_section_name(secTab, symbol->st_shndx)); i++);

	symbol->st_shndx = (i < nb_sections) ? i : -2;
}

int find_index_in_name_table(char *haystack, char *needle, Elf32_Word size)
{
	int i;

	if(strlen(needle) <= 0)
		return -1;
	for(i = 0; (i < size) && strcmp(&(haystack[i]), needle); i += strlen(&(haystack[i])) + 1);

	return (i < size) ? i : -1;
}

int add_symbol_in_table(Symtab_Struct *st, Elf32_Sym *symtab)
{
	int ind = st->nbSymbol;

	st->nbSymbol++;
	st->tab = realloc(st->tab, sizeof(Elf32_Sym*) * st->nbSymbol);
	st->tab[ind] = malloc(sizeof(Elf32_Sym));
	memcpy(st->tab[ind], symtab, sizeof(Elf32_Sym));

	return ind;
}

static inline void swap_symbols(Elf32_Sym *sym1, Elf32_Sym *sym2)
{
	Elf32_Sym *tmp = malloc(sizeof(Elf32_Sym));
	memcpy(tmp, sym2, sizeof(Elf32_Sym));
	memcpy(sym2, sym1, sizeof(Elf32_Sym));
	memcpy(sym1, tmp, sizeof(Elf32_Sym));
	free(tmp);
}

void sort_new_symbol_table(Symtab_Struct *st)
{
	int j, cpt = 0;

	for(int i = 1; i < st->nbSymbol; i++)
	{
		if(ELF32_ST_TYPE(st->tab[i]->st_info) == STT_SECTION)
		{
			cpt++;
			continue;
		}

		for(j = i; (j < st->nbSymbol) && ELF32_ST_TYPE(st->tab[j]->st_info) != STT_SECTION; j++);
		if(j == st->nbSymbol)
			continue;

		swap_symbols(st->tab[i], st->tab[j]);
		cpt++;
	}

	for(int i = cpt; i < st->nbSymbol; i++)
	{
		if(ELF32_ST_BIND(st->tab[i]->st_info) != STB_GLOBAL)
			continue;

		for(j = st->nbSymbol - 1; (j >= i) && ELF32_ST_BIND(st->tab[j]->st_info) == STB_GLOBAL; j--);
		if(j < i)
			continue;

		swap_symbols(st->tab[i], st->tab[j]);
	}
}
