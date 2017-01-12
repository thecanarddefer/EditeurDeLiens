#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "elf_common.h"
#include "util.h"
#include "section.h"
#include "symbol.h"
#include "relocation.h"
#include "disp.h"

#include "fusion.h"


int main(int argc, char *argv[])
{
	int err = 0;

	if(argc < 4)
	{
		fprintf(stderr, "%s : FICHIER_ENTRÉE1 FICHIER_ENTRÉE2 FICHIER_SORTIE\n", argv[0]);
		return 1;
	}

	/* Ouverture des fichiers passés en argument */
	int fd_in1, fd_in2, fd_out;
	if(open_files(argv, &fd_in1, &fd_in2, &fd_out))
		return 2;

	/* Initialisation des structures */
	Elf32_Ehdr *ehdr1      = read_elf_header(fd_in1);
	Elf32_Ehdr *ehdr2      = read_elf_header(fd_in2);
	Section_Table *secTab1 = read_sectionTable(fd_in1, ehdr1);
	Section_Table *secTab2 = read_sectionTable(fd_in2, ehdr2);
	symbolTable *st1       = read_symbolTable(fd_in1, secTab1);
	symbolTable *st2       = read_symbolTable(fd_in2, secTab2);
	Symtab_Struct *st_out  = read_symtab_struct(fd_in1, secTab1, SHT_SYMTAB); // En réalité, on duplique la table des symboles du premier fichier
	Data_Rel *drel1        = read_relocationTables(fd_in1, secTab1);
	Data_Rel *drel2        = read_relocationTables(fd_in2, secTab2);
	Data_fusion *df        = malloc(sizeof(Data_fusion));
	df->f = NULL;
	df->offset = 0;
	df->nb_sections = 0;
	df->nb_written  = 1;

	/* On crée la nouvelle section n°0 de type NULL */
	gather_sections(df, secTab1, secTab2, ONLY1, 1, SHT_NULL);

	/* On crée le fichier de sortie qui contient les sections PROGBITS fusionnées */
	print_debug(BOLD "==> Étape de fusion des sections PROGBITS\n" RESET);
	gather_sections(df, secTab1, secTab2, MERGE, 2, SHT_PROGBITS, SHT_NOBITS);
	df->file_offset = ehdr1->e_ehsize;
	merge_and_write_sections_in_file(df, fd_in1, fd_in2, fd_out);

	/* On recherche les sections de type REL */
	print_debug(BOLD "\n==> Étape de récupération des sections REL(A)\n" RESET);
	gather_sections(df, secTab1, secTab2, MERGE, 2, SHT_REL, SHT_RELA);

	/* On recherche les sections spécifiques à ARM */
	print_debug(BOLD "\n==> Étape de récupération des sections ARM\n" RESET);
	df->nb_written = df->nb_sections;
	gather_sections(df, secTab1, secTab2, ONLY1, 3, SHT_ARM_EXIDX, SHT_ARM_PREEMPTMAP, SHT_ARM_ATTRIBUTES);
	write_only_sections_in_file(df, fd_in1, fd_out);

	/* En enfin, on recherche toutes les autres sections */
	print_debug(BOLD "\n==> Étape de récupération des autres sections\n" RESET);
	gather_sections(df, secTab1, secTab2, MERGE_NOT_IN, 8, SHT_NULL, SHT_PROGBITS, SHT_NOBITS,
		SHT_REL, SHT_RELA, SHT_ARM_EXIDX, SHT_ARM_PREEMPTMAP, SHT_ARM_ATTRIBUTES);

	/* On calcule les nouveaux indices de section */
	print_debug(BOLD "\n==> Étape de création des tables de correspondance\n" RESET);
	df->newsec1 = find_new_section_index(df, secTab1);
	df->newsec2 = find_new_section_index(df, secTab2);
	print_debug(BOLD "\n==> Affichage de la fusion des sections\n" RESET);
	for(int i = 1; i < df->nb_sections; i++)
		print_debug("Section n°%2i : %-20s %#08x  %#08x\n", i, df->f[i]->section, df->f[i]->offset, df->f[i]->size);

	/* On met à jour l'indice de section des sections */
	print_debug(BOLD "\n==> Étape de mise à jour des indices de section des sections\n" RESET);
	for(int i = 1; i < df->nb_sections; i++)
		update_section_index_in_section(df->f[i]->shdr, df->newsec1, df->nb_sections);

	/* On met à jour l'indice de section des symboles du premier fichier */
	print_debug(BOLD "\n==> Étape de mise à jour des indices de section des symboles\n" RESET);
	for(int i = 1; i < st_out->nbSymbol; i++)
		update_section_index_in_symbol(st_out->tab[i], df->newsec1, df->nb_sections);

	/* On fusionne et corrige les symboles */
	print_debug(BOLD "\n==> Étape de fusion des tables de symboles\n" RESET);
	err = merge_and_fix_symbols(df, secTab1, secTab2, st1, st2, st_out);
	if(err)
		goto clean;
	sort_new_symbol_table(st_out);
	write_new_symbol_table_in_file(fd_out, df, st_out);
	if(print_debug(BOLD "\n==> Affichage de la fusion des symboles\n" RESET))
		dump_symtab(st_out);

	/* On met à jour le champ r_info des symboles des tables de réimplantations */
	print_debug(BOLD "\n==> Étape de fusion des tables de réimplantations\n" RESET);
	update_relocations_info(drel1, df->newsec1);
	update_relocations_info(drel2, df->newsec2);
	merge_and_fix_relocations(df, fd_in2, secTab1, secTab2, drel1, drel2);
	//merge_and_write_sections_in_file(df, fd_in1, fd_in2, fd_out);

	print_debug(BOLD "\n==> Étape d'écriture du nouvel en-tête ELF\n" RESET);
	ehdr1->e_shoff += ehdr2->e_shoff;
	write_elf_header_in_file(fd_out, ehdr1, df);
	write_new_section_table_in_file(fd_out, ehdr1, df);

clean:
	close(fd_in1);
	close(fd_in2);
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

	if(err)
		remove(argv[3]);

	return err;
}

#define CHECK_OPEN(f, i) if((f) < 0) { fprintf(stderr, "%s : Impossible d'ouvrir le fichier '%s'.\n", argv[0], argv[i]); return 1; }
static int open_files(char *argv[], int *fd_in1, int *fd_in2, int *fd_out)
{
	*fd_in1 = open(argv[1], O_RDONLY);
	CHECK_OPEN(*fd_in1, 1);
	*fd_in2 = open(argv[2], O_RDONLY);
	CHECK_OPEN(*fd_in2, 2);
	*fd_out = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	CHECK_OPEN(*fd_out, 3);
	return 0;
}

static void gather_sections(Data_fusion *df, Section_Table *secTab1, Section_Table *secTab2, Gather_Mode mode, int nb_types, ...)
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
	for(int i = 0; i < secTab1->nb_sections; i++)
	{
		for(j = 0; (j < nb_types) && (secTab1->shdr[i]->sh_type) != types[j]; j++);
		if(((mode == MERGE_NOT_IN) && (j != nb_types)) || ((mode != MERGE_NOT_IN) && (j == nb_types)))
			continue;

		/* Recherche si la section est présente dans le second fichier */
		for(j = 0; (j < secTab2->nb_sections) && strcmp(get_section_name(secTab1, i), get_section_name(secTab2, j)); j++);

		if(df->offset == 0)
			df->offset = secTab1->shdr[i]->sh_offset;

		ind = df->nb_sections;
		df->nb_sections++;
		df->f = realloc(df->f, sizeof(Fusion*) * df->nb_sections);
		df->f[ind] = malloc(sizeof(Fusion));
		df->f[ind]->offset = df->offset;
		df->f[ind]->ptr_shdr1 = secTab1->shdr[i];
		df->f[ind]->shdr = malloc(sizeof(Elf32_Shdr));
		memcpy(df->f[ind]->shdr, secTab1->shdr[i], sizeof(Elf32_Shdr));
		df->f[ind]->shdr->sh_name = i;
		df->f[ind]->shdr->sh_offset = df->offset;

		if((j == secTab2->nb_sections) || (mode == ONLY1))
		{
			/* La section est présente uniquement dans le premier fichier */
			print_debug("Ajout de la section %2i '%s' à l'offset %#x avec une taille de %#x (-> premier fichier uniquement)\n",
				i, get_section_name(secTab1, i), df->offset, secTab1->shdr[i]->sh_size);
			df->f[ind]->size = secTab1->shdr[i]->sh_size;
			df->f[ind]->ptr_shdr2 = NULL;
		}
		else
		{
			/* La section est présente dans les deux fichiers */
			print_debug("Ajout de la section %2i '%s' à l'offset %#x avec une taille de %x+%x=%#x (-> deux fichiers)\n",
				i, get_section_name(secTab2, j), df->offset, secTab1->shdr[i]->sh_size, secTab2->shdr[j]->sh_size, secTab1->shdr[i]->sh_size + secTab2->shdr[j]->sh_size);
			df->f[ind]->size = secTab1->shdr[i]->sh_size + secTab2->shdr[j]->sh_size;
			df->f[ind]->ptr_shdr2 = secTab2->shdr[j];
			df->f[ind]->shdr->sh_size += secTab2->shdr[j]->sh_size;
		}

		strcpy(df->f[ind]->section, get_section_name(secTab1, i));
		df->offset += df->f[ind]->size;
	}

	/* On recherche les sections PROGBITS du second fichier qui ne sont pas présentes dans le premier */
	for(int i = 1; i < secTab2->nb_sections; i++)
	{
		for(j = 0; (j < nb_types) && (secTab2->shdr[i]->sh_type) != types[j]; j++);
		if(((mode == MERGE_NOT_IN) && (j != nb_types)) || ((mode != MERGE_NOT_IN) && (j == nb_types)))
			continue;

		/* Recherche si la section est absente du premier fichier */
		for(j = 0; (j < secTab1->nb_sections) && strcmp(get_section_name(secTab1, j), get_section_name(secTab2, i)); j++);

		if(j == secTab1->nb_sections)
		{
			print_debug("Ajout de la section %2i '%s' à l'offset %#x avec une taille de %#x (-> second fichier uniquement)\n",
				i, get_section_name(secTab2, i), df->offset, secTab2->shdr[i]->sh_size);
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
			df->f[ind]->shdr = malloc(sizeof(Elf32_Shdr));
			memcpy(df->f[ind]->shdr, secTab2->shdr[i], sizeof(Elf32_Shdr));
			df->f[ind]->shdr->sh_name = i;
			df->f[ind]->shdr->sh_offset = df->offset;
		}
	}

	free(types);
}

static Elf32_Section *find_new_section_index(Data_fusion *df, Section_Table *secTab)
{
	int j;
	Elf32_Section *newsec = malloc(sizeof(Elf32_Section) * secTab->nb_sections);

	for(int i = 0; i < secTab->nb_sections; i++)
	{
		for(j = 0; (j < df->nb_sections) && strcmp(df->f[j]->section, get_section_name(secTab, i)); j++);
		newsec[i] = (j < df->nb_sections) ? j : 0;
		print_debug("Ancienne section %2i <==> %2i nouvelle section\n", i, j);
		if(j == df->nb_sections)
			fprintf(stderr, RESET "ATTENTION : la section n°%i « %s » n'apparaît pas dans la nouvelle table des sections !\n", i, get_section_name(secTab, i));
	}

	return newsec;
}

static void update_section_index_in_section(Elf32_Shdr *section, Elf32_Section *newsec, unsigned nb_sections)
{
	if((section->sh_link >= nb_sections) || (section->sh_info >= nb_sections))
		return;
	print_debug("La section qui pointait vers les sections LN %2i et Inf %2i pointe dorénavant vers les indices %2i et %2i\n",
		section->sh_link, section->sh_info, newsec[section->sh_link], newsec[section->sh_info]);
	section->sh_link = newsec[section->sh_link];
	section->sh_info = newsec[section->sh_info];
}

static void update_section_index_in_symbol(Elf32_Sym *symbol, Elf32_Section *newsec, unsigned nb_sections)
{
	if((symbol->st_shndx >= nb_sections) || (symbol->st_shndx == SHN_UNDEF) || (symbol->st_shndx == SHN_ABS))
		return;
	if(newsec[symbol->st_shndx] == 0)
		fprintf(stderr, RESET "ATTENTION : le symbole qui pointait vers la section %i ne pointe plus vers de section !\n", symbol->st_shndx);
	print_debug("Le symbole qui pointait vers l'indice %2i pointe dorénavant vers l'indice %2i\n", symbol->st_shndx, newsec[symbol->st_shndx]);
	symbol->st_shndx = newsec[symbol->st_shndx];
}

static int find_index_in_name_table(char *haystack, char *needle, Elf32_Word size)
{
	int i;

	if(strlen(needle) <= 0)
		return -1;
	for(i = 0; (i < size) && strcmp(&(haystack[i]), needle); i += strlen(&(haystack[i])) + 1);

	return (i < size) ? i : -1;
}

static int add_symbol_in_table(Symtab_Struct *st, Elf32_Sym *symtab)
{
	int ind = st->nbSymbol;

	st->nbSymbol++;
	st->tab = realloc(st->tab, sizeof(Elf32_Sym*) * st->nbSymbol);
	st->tab[ind] = malloc(sizeof(Elf32_Sym));
	memcpy(st->tab[ind], symtab, sizeof(Elf32_Sym));

	return ind;
}

static int merge_and_fix_symbols(Data_fusion *df, Section_Table *secTab1, Section_Table *secTab2, symbolTable *st1, symbolTable *st2, Symtab_Struct *st_out)
{
	int ind, j, shndx;
	char *buff;
	df->symbolNameTable_size = secTab1->shdr[st1->symtab->strIndex]->sh_size;

	for(int i = 1; i < st2->symtab->nbSymbol; i++)
	{
		buff = get_static_symbol_name(st2, i);
		shndx = find_index_in_name_table(st_out->symbolNameTable, buff, df->symbolNameTable_size);

		if(shndx > 0)
		{
			/* On calcule l'indice correspondant au symbole déjà présent en se basant sur son nom */
			for(ind = 0; (ind < st_out->nbSymbol) && strcmp(get_symbol_name(st_out->tab, st_out->symbolNameTable, ind), buff); ind++);
			const int is_global_st_out = (ELF32_ST_BIND(st_out->tab[ind]->st_info)    == STB_GLOBAL);
			const int is_global_st2    = (ELF32_ST_BIND(st2->symtab->tab[i]->st_info) == STB_GLOBAL);

			if(is_global_st_out && is_global_st2 && (st_out->tab[ind]->st_shndx != SHN_UNDEF) && (st2->symtab->tab[i]->st_shndx != SHN_UNDEF))
			{
				/* Un symbole global est défini deux fois, on abandonne... */
				fprintf(stderr, "FATAL : le symbole « %s » est défini plus d'une fois !\n", buff);
				return 3;
			}
			else if(is_global_st_out && is_global_st2 && (st_out->tab[ind]->st_shndx == SHN_UNDEF) && (st2->symtab->tab[i]->st_shndx != SHN_UNDEF))
			{
				/* On remplace le symbole global indéfini par le défini, sans toucher à st_name */
				print_debug("Remplace le symbole global %2i '%s' par sa version définie\n", i, get_static_symbol_name(st2, i));
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
				print_debug("Ajout du symbole local %i '%s' dans la table des symboles uniquement\n", i, get_static_symbol_name(st2, i));
				ind = add_symbol_in_table(st_out, st2->symtab->tab[i]);
				st_out->tab[ind]->st_name = shndx;
				update_section_index_in_symbol(st_out->tab[ind], df->newsec2, df->nb_sections);

				/* On met à jour la valeur du nouveau symbole */
				for(j = 0; (j < secTab1->nb_sections) && df->newsec2[ st2->symtab->tab[i]->st_shndx ] != df->newsec1[j]; j++);
				if(j < secTab1->nb_sections)
					st_out->tab[ind]->st_value += secTab1->shdr[j]->sh_size;
			}
		}
		else
		{
			j = secTab1->nb_sections;
			/* On cherche s'il s'agit d'un symbole de section et qu'il n'est pas déjà défini */
			if((ELF32_ST_BIND(st2->symtab->tab[i]->st_info) == STB_LOCAL) && st2->symtab->tab[i]->st_shndx < secTab2->nb_sections)
				for(j = 0; (j < secTab1->nb_sections) && df->newsec2[ st2->symtab->tab[i]->st_shndx ] != df->newsec1[j]; j++);

			if(j == secTab1->nb_sections)
			{
				/* On ajoute le symbole à la nouvelle table */
				print_debug("Ajout du symbole %i '%s' à la table des symboles\n", i, get_static_symbol_name(st2, i));
				ind = add_symbol_in_table(st_out, st2->symtab->tab[i]);
				update_section_index_in_symbol(st_out->tab[ind], df->newsec2, df->nb_sections);

				if(strlen(buff) > 0)
				{
					st_out->tab[ind]->st_name = df->symbolNameTable_size;
					st_out->symbolNameTable = realloc(st_out->symbolNameTable, df->symbolNameTable_size + strlen(buff) + 1);
					st_out->symbolNameTable[df->symbolNameTable_size] = '\0';
					strcat(&(st_out->symbolNameTable[df->symbolNameTable_size]), buff);
					df->symbolNameTable_size += strlen(buff) + 1;
				}
			}
		}
	}

	return 0;
}

static void update_relocations_info(Data_Rel *drel, Elf32_Section *newsec)
{
	for(int i = 0; i < drel->nb_rel; i++)
		for(int j = 0; j < drel->e_rel[i]; j++)
			drel->rel[i][j]->r_info = ELF32_R_INFO(newsec[ ELF32_R_SYM(drel->rel[i][j]->r_info) ], ELF32_R_TYPE(drel->rel[i][j]->r_info));
}

static void merge_and_fix_relocations(Data_fusion *df, int fd2, Section_Table *secTab1, Section_Table *secTab2, Data_Rel *drel1, Data_Rel *drel2)
{
	int ind, j;

	/* On concatène les tables de réimplantations de drel2 dans drel1 */
	for(int i = 0; i < drel2->nb_rel; i++)
	{
		for(j = 0; (j < drel1->nb_rel) && df->newsec2[ drel2->i_rel[i] ] != df->newsec1[ drel1->i_rel[j] ]; j++);
		if(j < drel1->nb_rel)
		{
			/* La section REL était déjà présente dans le premier fichier, on ajoute à la suite celle-ci */
			print_debug("Concatène la section REL %2i '%s' avec celle du premier fichier\n", i, get_section_name(secTab2, drel2->i_rel[i]));
			ind = drel1->e_rel[j];
			drel1->e_rel[j] += drel2->e_rel[i];
			drel1->rel[j]    = realloc(drel1->rel[j], sizeof(Elf32_Rel*) * drel1->e_rel[j]);
			for(int k = 0; k < drel2->e_rel[i]; k++)
			{
				drel1->rel[j][ind] = malloc(sizeof(Elf32_Rel));
				memcpy(drel1->rel[j][ind], drel2->rel[i][k], sizeof(Elf32_Rel));
				drel1->rel[j][ind]->r_offset += secTab1->shdr[  secTab1->shdr[ drel1->i_rel[j] ]->sh_info  ]->sh_size;
				Elf32_Sword addend;
				lseek(fd2, secTab2->shdr[ drel2->i_rel[i] ]->sh_offset + drel2->rel[i][k]->r_offset, SEEK_SET);
				read(fd2, &addend, sizeof(Elf32_Sword));
				switch(ELF32_R_TYPE(drel2->rel[i][k]->r_info))
				{
					case R_ARM_ABS32_NOI:
					case R_ARM_ABS32:
					case R_ARM_ABS16:
					case R_ARM_ABS12:
					case R_ARM_ABS8:
						addend += df->f[    df->newsec2[  secTab2->shdr[ drel2->i_rel[i] ]->sh_link  ]    ]->ptr_shdr1->sh_size;
						break;
					case R_ARM_JUMP24:
					case R_ARM_CALL:
						addend += (df->f[    df->newsec2[  secTab2->shdr[ drel2->i_rel[i] ]->sh_link  ]    ]->ptr_shdr1->sh_size >> 2);
						break;
					default:
						fprintf(stderr, "ATTENTION : le type de réimplémentation %i n'est pas pris en charge !\n", ELF32_R_TYPE(drel2->rel[i][k]->r_info));
				}
				ind++;
			}
		}
		else
		{
			/* La section est absente du premier fichier, on l'ajoute telle quelle */
			print_debug("Ajout de la section REL %2i '%s' à la table de réimplantations\n", i, get_section_name(secTab2, drel2->i_rel[i]));
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
}


static inline void swap_symbols(Elf32_Sym *sym1, Elf32_Sym *sym2)
{
	Elf32_Sym *tmp = malloc(sizeof(Elf32_Sym));
	memcpy(tmp, sym2, sizeof(Elf32_Sym));
	memcpy(sym2, sym1, sizeof(Elf32_Sym));
	memcpy(sym1, tmp, sizeof(Elf32_Sym));
	free(tmp);
}

static void sort_new_symbol_table(Symtab_Struct *st)
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

static void write_elf_header_in_file(int fd_out, Elf32_Ehdr *ehdr, Data_fusion *df)
{
	int i;
	for(i = 0; (i < df->nb_sections) && strcmp(df->f[i]->section, ".shstrtab"); i++);
	if(i == df->nb_sections)
	{
		fprintf(stderr, "FATAL : Il n'y a pas de section .shstrtab !");
		return;
	}

	ehdr->e_shstrndx = i;
	//ehdr->e_shoff    = df->f[i]->offset;
	ehdr->e_shnum    = df->nb_sections;

	print_debug("Il y a %u sections dans le nouveau fichier\n", ehdr->e_shnum);
	print_debug("La section %i '.shstrtab' sera écrite à l'offset %#x\n", ehdr->e_shstrndx, ehdr->e_shoff);
	lseek(fd_out, 0, SEEK_SET);
	write(fd_out, ehdr->e_ident,      EI_NIDENT);
	write(fd_out, &ehdr->e_type,      sizeof(ehdr->e_type));
	write(fd_out, &ehdr->e_machine,   sizeof(ehdr->e_machine));
	write(fd_out, &ehdr->e_version,   sizeof(ehdr->e_version));
	write(fd_out, &ehdr->e_entry,     sizeof(ehdr->e_entry));
	write(fd_out, &ehdr->e_phoff,     sizeof(ehdr->e_phoff));
	write(fd_out, &ehdr->e_shoff,     sizeof(ehdr->e_shoff));
	write(fd_out, &ehdr->e_flags,     sizeof(ehdr->e_flags));
	write(fd_out, &ehdr->e_ehsize,    sizeof(ehdr->e_ehsize));
	write(fd_out, &ehdr->e_phentsize, sizeof(ehdr->e_phentsize));
	write(fd_out, &ehdr->e_phnum,     sizeof(ehdr->e_phnum));
	write(fd_out, &ehdr->e_shentsize, sizeof(ehdr->e_shentsize));
	write(fd_out, &ehdr->e_shnum,     sizeof(ehdr->e_shnum));
	write(fd_out, &ehdr->e_shstrndx,  sizeof(ehdr->e_shstrndx));
}

static void write_only_sections_in_file(Data_fusion *df, int fd1, int fd_out)
{
	off_t old_offset = df->f[df->nb_written]->offset;
	off_t new_offset = old_offset;
	lseek(fd_out, old_offset, SEEK_SET);

	for(int i = df->nb_written; i < df->nb_sections; i++)
	{
		/* On écrit la section du premier fichier */
		print_debug("Écriture de la section %2i '%s' à l'offset %#x avec une taille de %#x ", i, df->f[i]->section, old_offset, df->f[i]->size);
		new_offset += write_section_in_file(fd1, fd_out, df->f[i]->ptr_shdr1);
		print_debug("(%s)\n", (df->f[i]->size == new_offset - old_offset) ? "correcte" : "ERREUR");
		old_offset = new_offset;
		df->nb_written++;
	}
}

static void merge_and_write_sections_in_file(Data_fusion *df, int fd1, int fd2, int fd_out)
{
	off_t old_offset = df->f[df->nb_written]->offset;
	off_t new_offset = old_offset;
	lseek(fd_out, old_offset, SEEK_SET);

	for(int i = df->nb_written; i < df->nb_sections; i++)
	{
		print_debug("Écriture de la section %2i '%s' à l'offset %#x avec une taille de %#x ", i, df->f[i]->section, old_offset, df->f[i]->size);
		if(df->f[i]->ptr_shdr1 != NULL)
		{
			/* On écrit la section du premier fichier */
			new_offset += write_section_in_file(fd1, fd_out, df->f[i]->ptr_shdr1);
			if (df->f[i]->ptr_shdr2 != NULL) /* On écrit la section du second fichier */
				new_offset += write_section_in_file(fd2, fd_out, df->f[i]->ptr_shdr2);
		}
		else
		{
			/* On écrit uniquement la section du second fichier */
			new_offset += write_section_in_file(fd2, fd_out, df->f[i]->ptr_shdr2);
		}
		print_debug("(%s)\n", (df->f[i]->size == new_offset - old_offset) ? "correcte" : "ERREUR");
		old_offset = new_offset;
		df->nb_written++;
	}
}

static ssize_t write_section_in_file(int fd_in, int fd_out, Elf32_Shdr *shdr)
{
		ssize_t r, w;
		unsigned char *buff = malloc(sizeof(unsigned char) * shdr->sh_size);

		lseek(fd_in, shdr->sh_offset, SEEK_SET);
		r = read(fd_in, buff, shdr->sh_size);

		w = write(fd_out, buff, shdr->sh_size);
		free(buff);

		if(r != w)
			fprintf(stderr, "ATTENTION : %lu octets ont été lus depuis le fichier d''entrée, mais uniquement %lu ont été écrits.\n", r, w);

		return w;
}

static void write_new_section_table_in_file(int fd_out, Elf32_Ehdr *ehdr, Data_fusion *df)
{
	off_t old_offset = ehdr->e_shoff;
	off_t new_offset = old_offset;

	for(int i = 0; i < df->nb_sections; i++)
	{
		print_debug("Écriture de la section n°%2i '%s' dans le fichier à l'offset %#x ", i, df->f[i]->section, old_offset);
		lseek(fd_out, ehdr->e_shoff + i * ehdr->e_shentsize, SEEK_SET);
		new_offset += write(fd_out, &df->f[i]->shdr->sh_name,      sizeof(df->f[i]->shdr->sh_name));
		new_offset += write(fd_out, &df->f[i]->shdr->sh_type,      sizeof(df->f[i]->shdr->sh_type));
		new_offset += write(fd_out, &df->f[i]->shdr->sh_flags,     sizeof(df->f[i]->shdr->sh_flags));
		new_offset += write(fd_out, &df->f[i]->shdr->sh_addr,      sizeof(df->f[i]->shdr->sh_addr));
		new_offset += write(fd_out, &df->f[i]->shdr->sh_offset,    sizeof(df->f[i]->shdr->sh_offset));
		new_offset += write(fd_out, &df->f[i]->shdr->sh_size,      sizeof(df->f[i]->shdr->sh_size));
		new_offset += write(fd_out, &df->f[i]->shdr->sh_link,      sizeof(df->f[i]->shdr->sh_link));
		new_offset += write(fd_out, &df->f[i]->shdr->sh_info,      sizeof(df->f[i]->shdr->sh_info));
		new_offset += write(fd_out, &df->f[i]->shdr->sh_addralign, sizeof(df->f[i]->shdr->sh_addralign));
		new_offset += write(fd_out, &df->f[i]->shdr->sh_entsize,   sizeof(df->f[i]->shdr->sh_entsize));
		print_debug("(%s)\n", (sizeof(Elf32_Shdr) == new_offset - old_offset) ? "correct" : "ERREUR");
		old_offset = new_offset;
	}

	print_debug("Écriture de la table des noms de section à l'offset %#x\n", df->f[ehdr->e_shstrndx]->offset);
	lseek(fd_out, df->f[ehdr->e_shstrndx]->offset, SEEK_SET);
	for(int i = 0; i < df->nb_sections; i++)
		df->file_offset += write(fd_out, df->f[i]->section, strlen(df->f[i]->section) + 1);
}

static void write_new_symbol_table_in_file(int fd_out, Data_fusion *df, Symtab_Struct *st_out)
{
	int i;
	for(i = 0; strcmp(df->f[i]->section, ".symtab"); i++);

	off_t old_offset = df->f[i]->offset;
	off_t new_offset = old_offset;
	lseek(fd_out, old_offset, SEEK_SET);

	for(i = 0; i < st_out->nbSymbol; i++)
	{
		print_debug("Écriture du symbole n°%2i '%s' dans le fichier à l'offset %#x ", i, get_symbol_name(st_out->tab, st_out->symbolNameTable, i), old_offset);
		new_offset += write(fd_out, &st_out->tab[i]->st_name,  sizeof(st_out->tab[i]->st_name));
		new_offset += write(fd_out, &st_out->tab[i]->st_value, sizeof(st_out->tab[i]->st_value));
		new_offset += write(fd_out, &st_out->tab[i]->st_size,  sizeof(st_out->tab[i]->st_size));
		new_offset += write(fd_out, &st_out->tab[i]->st_info,  sizeof(st_out->tab[i]->st_info));
		new_offset += write(fd_out, &st_out->tab[i]->st_other, sizeof(st_out->tab[i]->st_other));
		new_offset += write(fd_out, &st_out->tab[i]->st_shndx, sizeof(st_out->tab[i]->st_shndx));
		print_debug("(%s)\n", (sizeof(Elf32_Sym) == new_offset - old_offset) ? "correct" : "ERREUR");
		old_offset = new_offset;
	}

	print_debug("Écriture de la table des noms de symboles '%s' dans le fichier à l'offset %#x\n", st_out->name, df->file_offset);
	df->file_offset += write(fd_out, &st_out->symbolNameTable, df->symbolNameTable_size);
}

static void destroy_data_fusion(Data_fusion *df)
{
	for(int i = 0; i < df->nb_sections; i++)
		free(df->f[i]);
	free(df->f);
	free(df->newsec1);
	free(df->newsec2);
	free(df);
}
