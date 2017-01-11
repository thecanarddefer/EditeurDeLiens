#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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

	Elf32_Ehdr *ehdr1      = read_elf_header(fd1);
	Elf32_Ehdr *ehdr2      = read_elf_header(fd2);
	Section_Table *secTab1 = read_sectionTable(fd1, ehdr1);
	Section_Table *secTab2 = read_sectionTable(fd2, ehdr2);
	symbolTable *st1       = read_symbolTable(fd1, secTab1);
	symbolTable *st2       = read_symbolTable(fd2, secTab2);
	Symtab_Struct *st_out    = read_symtab_struct(fd1, secTab1, SHT_SYMTAB); // En réalité, on duplique la table des symboles du premier fichier
	Data_Rel *drel1        = read_relocationTables(fd1, secTab1);
	Data_Rel *drel2        = read_relocationTables(fd2, secTab2);
	Elf32_Word symbsize    = secTab1->shdr[st1->symtab->strIndex]->sh_size;
	Data_fusion *df = malloc(sizeof(Data_fusion));
	df->f = NULL;
	df->offset = 0;
	df->nb_sections = 1;

	/* On crée la nouvelle section n°0 de type NULL */
	ind = df->nb_sections - 1;
	df->f = realloc(df->f, sizeof(Fusion*) * df->nb_sections);
	df->f[ind] = malloc(sizeof(Fusion));
	df->f[ind]->ptr_shdr1 = secTab1->shdr[0];
	df->f[ind]->ptr_shdr2 = secTab2->shdr[0];
	df->f[ind]->size = secTab2->shdr[0]->sh_size;
	df->f[ind]->offset = df->offset;
	strcpy(df->f[ind]->section, get_section_name(secTab1, 0));
	df->offset += df->f[ind]->size;

	gather_sections(df, secTab1, secTab2, 2, SHT_PROGBITS, SHT_NOBITS);
	gather_sections(df, secTab1, secTab2, 2, SHT_REL, SHT_RELA);

	/* On crée le fichier de sortie qui contient les sections PROGBITS fusionnées */
	lseek(fd_out, ehdr1->e_ehsize, SEEK_SET);
	for(int i = 1; i < df->nb_sections; i++)
	{
		if(df->f[i]->ptr_shdr1 != NULL)
		{
			/* On écrit la section du premier fichier */
			write_progbits_in_file(fd1, fd_out, df->f[i]->ptr_shdr1->sh_size, df->f[i]->ptr_shdr1->sh_offset);
			if (df->f[i]->ptr_shdr2 != NULL)
			{
				/* On écrit la section du second fichier */
				write_progbits_in_file(fd2, fd_out, df->f[i]->ptr_shdr2->sh_size, df->f[i]->ptr_shdr2->sh_offset);
			}
		}
		else
		{
			/* On écrit uniquement la section du second fichier */
			write_progbits_in_file(fd2, fd_out, df->f[i]->ptr_shdr2->sh_size, df->f[i]->ptr_shdr2->sh_offset);
		}
	}

	/* On calcule les nouveaux indices de section */
	df->newsec1 = find_new_section_index(df->f, df->nb_sections, secTab1);
	df->newsec2 = find_new_section_index(df->f, df->nb_sections, secTab2);

	/* On met à jour l'indice de section des symboles du premier fichier (à ce stade là, st_out = st1) */
	for(int i = 1; i < st_out->nbSymbol; i++)
		update_section_index_in_symbol(st_out->tab[i], df->newsec1, df->nb_sections);

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
				update_section_index_in_symbol(st_out->tab[ind], df->newsec2, df->nb_sections);
			}
			else if(!is_global_st_out && !is_global_st2)
			{
				/* Un autre symbole local a le même nom, on ajoute le symbole à la table des symboles uniquement */
				ind = add_symbol_in_table(st_out, st2->symtab->tab[i]);
				st_out->tab[ind]->st_name = shndx;
				update_section_index_in_symbol(st_out->tab[ind], df->newsec2, df->nb_sections);

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
				update_section_index_in_symbol(st_out->tab[ind], df->newsec2, df->nb_sections);

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

	/* On met à jour le champ r_info des symboles des tables de réimplantations */
	update_relocations_info(drel1, df->newsec1);
	update_relocations_info(drel2, df->newsec2);

	/* On concatène les tables de réimplantations de drel2 dans drel1 */
	for(int i = 0; i < drel2->nb_rel; i++)
	{
		for(j = 0; (j < drel1->nb_rel) && df->newsec2[ drel2->i_rel[i] ] != df->newsec1[ drel1->i_rel[j] ]; j++);
		if(j < drel1->nb_rel)
		{
			/* La section REL était déjà présente dans le premier fichier, on ajoute à la suite celle-ci */
			ind = drel1->e_rel[j];
			drel1->e_rel[j] += drel2->e_rel[i];
			drel1->rel[j]    = realloc(drel1->rel[j], sizeof(Elf32_Rel*) * drel1->e_rel[j]);
			for(int k = 0; k < drel2->e_rel[i]; k++)
			{
				drel1->rel[j][ind] = malloc(sizeof(Elf32_Rel));
				memcpy(drel1->rel[j][ind], drel2->rel[i][k], sizeof(Elf32_Rel));
				//TODO: Mise à jour de r_offset
				ind++;
			}
		}
		else
		{
			/* La section est absente du premier fichier, on l'ajoute telle quelle */
			ind = drel1->nb_rel;
			drel1->nb_rel++;
			drel1->e_rel      = realloc(drel1->e_rel, sizeof(unsigned) * drel1->nb_rel);
			drel1->a_rel      = realloc(drel1->a_rel, sizeof(unsigned) * drel1->nb_rel);
			drel1->i_rel      = realloc(drel1->i_rel, sizeof(unsigned) * drel1->nb_rel);
			drel1->e_rel[ind] = drel2->e_rel[i];
			drel1->a_rel[ind] = drel2->a_rel[i];
			drel1->i_rel[ind] = drel2->i_rel[i];
			drel1->rel        = realloc(drel1->rel, sizeof(Elf32_Rel*) * drel1->nb_rel);
			drel1->rel[ind]   = malloc(sizeof(Elf32_Rel*) * drel1->e_rel[ind]);
			for(int k = 0; k < drel1->e_rel[ind]; k++)
			{
				drel1->rel[ind][k] = malloc(sizeof(Elf32_Rel));
				memcpy(drel1->rel[ind][k], drel2->rel[i][k], sizeof(Elf32_Rel));
			}
		}
	}

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
	destroy_data_fusion(df);

	return err;
}

Elf32_Section *find_new_section_index(Fusion **fusion, unsigned nb_sections, Section_Table *secTab)
{
	int j;
	Elf32_Section *newsec = malloc(sizeof(Elf32_Section) * secTab->nb_sections);

	for(int i = 0; i < secTab->nb_sections; i++)
	{
		for(j = 0; (j < nb_sections) && strcmp(fusion[j]->section, get_section_name(secTab, i)); j++);
		newsec[i] = (j < nb_sections) ? j : 0;
		if(j == nb_sections)
			fprintf(stderr, "ATTENTION : la section n°%i « %s » n'apparaît pas dans la nouvelle table des sections !\n", i, get_section_name(secTab, i));
	}

	return newsec;
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

void update_section_index_in_symbol(Elf32_Sym *symbol, Elf32_Section *newsec, unsigned nb_sections)
{
	if((symbol->st_shndx >= nb_sections) || (symbol->st_shndx == SHN_UNDEF) || (symbol->st_shndx == SHN_ABS))
		return;
	if(newsec[symbol->st_shndx] == 0)
		fprintf(stderr, "ATTENTION : le symbole qui pointait vers la section %i ne pointe plus vers de section !\n", symbol->st_shndx);
	symbol->st_shndx = newsec[symbol->st_shndx];
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

void update_relocations_info(Data_Rel *drel, Elf32_Section *newsec)
{
	for(int i = 0; i < drel->nb_rel; i++)
		for(int j = 0; j < drel->e_rel[i]; j++)
			drel->rel[i][j]->r_info = ELF32_R_INFO(newsec[ ELF32_R_SYM(drel->rel[i][j]->r_info) ], ELF32_R_TYPE(drel->rel[i][j]->r_info));
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

void gather_sections(Data_fusion *df, Section_Table *secTab1, Section_Table *secTab2, int nb_types, ...)
{
	int ind, j;
	Elf32_Word *types = malloc(sizeof(Elf32_Word) * nb_types);
	va_list aptr;

	/* Récupération des types recherchés */
	va_start(aptr, nb_types);
	for(int i = 0; i < nb_types; i++)
		types[i] = va_arg(aptr, Elf32_Word);
	va_end(aptr);

	/* On parcours les sections PROGBITS du premier fichier */
	for(int i = 1; i < secTab1->nb_sections; i++)
	{
		for(j = 0; (j < nb_types) && (secTab1->shdr[i]->sh_type) != types[j]; j++);
		if(j == nb_types)
			continue;

		/* Recherche si la section est présente dans le second fichier */
		for(j = 0; (j < secTab2->nb_sections) && strcmp(get_section_name(secTab1, i), get_section_name(secTab2, j)); j++);

		if(df->offset == 0)
			df->offset = secTab1->shdr[i]->sh_offset;

		df->nb_sections++;
		ind = df->nb_sections - 1;
		df->f = realloc(df->f, sizeof(Fusion*) * df->nb_sections);
		df->f[ind] = malloc(sizeof(Fusion));
		df->f[ind]->ptr_shdr1 = secTab1->shdr[i];

		if(j != secTab2->nb_sections)
		{
			/* La section est présente dans les deux fichiers */
			df->f[ind]->size = secTab1->shdr[i]->sh_size + secTab2->shdr[j]->sh_size;
			df->f[ind]->ptr_shdr2 = secTab2->shdr[j];
		}
		else
		{
			/* La section est présente uniquement dans le premier fichier */
			df->f[ind]->size = secTab1->shdr[i]->sh_size;
			df->f[ind]->ptr_shdr2 = NULL;
		}

		df->f[ind]->offset = df->offset;
		strcpy(df->f[ind]->section, get_section_name(secTab1, i));
		df->offset = df->f[ind]->size;
	}

	/* On recherche les sections PROGBITS du second fichier qui ne sont pas présentes dans le premier */
	for(int i = 1; i < secTab2->nb_sections; i++)
	{
		for(j = 0; (j < nb_types) && (secTab2->shdr[i]->sh_type) != types[j]; j++);
		if(j == nb_types)
			continue;

		/* Recherche si la section est absente du premier fichier */
		for(j = 0; (j < secTab1->nb_sections) && strcmp(get_section_name(secTab1, j), get_section_name(secTab2, i)); j++);

		if(j == secTab1->nb_sections)
		{
			df->nb_sections++;
			ind = df->nb_sections - 1;
			df->f = realloc(df->f, sizeof(Fusion*) * df->nb_sections);
			df->f[ind] = malloc(sizeof(Fusion));
			df->f[ind]->ptr_shdr1 = NULL;
			df->f[ind]->ptr_shdr2 = secTab2->shdr[i];
			df->f[ind]->size = secTab2->shdr[i]->sh_size;
			df->f[ind]->offset = df->offset;
			strcpy(df->f[ind]->section, get_section_name(secTab2, i));
			df->offset += df->f[ind]->size;
		}
	}

	free(types);
}

void destroy_data_fusion(Data_fusion *df)
{
	for(int i = 0; i < df->nb_sections; i++)
		free(df->f[i]);
	free(df->f);
	free(df->newsec1);
	free(df->newsec2);
	free(df);
}
