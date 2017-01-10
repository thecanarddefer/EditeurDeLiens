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
#include "symbolTable.h"


/**************************** ÉTAPE 4 ****************************
*    Utilitaires pour la récupération de la table des symboles
*****************************************************************/
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

char *get_symbol_name(Elf32_Sym **symtab, char *table, unsigned index) {
	return &(table[symtab[index]->st_name]);
}

char *get_static_symbol_name(symbolTable *symTabFull, unsigned index) {
	return get_symbol_name(symTabFull->symtab->tab, symTabFull->symtab->symbolNameTable, index);
}

char *get_dynamic_symbol_name(symbolTable *symTabFull, unsigned index) {
	return get_symbol_name(symTabFull->dynsym->tab, symTabFull->dynsym->symbolNameTable, index);
}

Elf32_Sym **read_Elf32_Sym(int fd, Elf32_Shdr **shdr, int *nbSymbol, int sectionIndex) {

	Elf32_Sym **symtab;

	if(sectionIndex != -1) {
		*nbSymbol = shdr[sectionIndex]->sh_size / shdr[sectionIndex]->sh_entsize; // Nombre de symboles dans la table.

		symtab = malloc(*nbSymbol * sizeof(Elf32_Sym*));
		for (int j = 0; j < *nbSymbol; ++j) {
			symtab[j] = malloc(sizeof(Elf32_Sym));

			lseek(fd, shdr[sectionIndex]->sh_offset + j * shdr[sectionIndex]->sh_entsize, SEEK_SET);

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

Symtab_Struct *read_symtab_struct(int fd, Section_Table *secTab, int shType) {
	int tmpSymtabIndex = -1,
		tmpStrtabIndex = -1;

	Symtab_Struct *s;
	s = malloc(sizeof(Symtab_Struct));

	// Init
	s->strIndex = -1;
	s->nbSymbol = 0;
	s->tab = NULL;
	s->name = NULL;
	s->symbolNameTable = NULL;

	tmpSymtabIndex = get_section_index(secTab, shType);
	if (tmpSymtabIndex != -1) {
		s->tab = read_Elf32_Sym(fd,secTab->shdr, &s->nbSymbol, tmpSymtabIndex);
		if (s->tab != NULL) {
			tmpStrtabIndex = secTab->shdr[tmpSymtabIndex]->sh_link;
			s->strIndex = tmpStrtabIndex;
			s->symbolNameTable = get_name_table(fd, tmpStrtabIndex, secTab->shdr);
			s->name = get_section_name(secTab,tmpSymtabIndex);
		}
	}
	return s;
}

// TODO: symbolTable.c:93:14: error: static declaration of ‘read_symbolTable’ follows non-static declaration
symbolTable *read_symbolTable(int fd, Section_Table *secTab) {
	symbolTable *symTabToRead;
	symTabToRead = malloc(sizeof(symbolTable));
	// initialisation
	symTabToRead->symtab = NULL;
	symTabToRead->dynsym = NULL;

	symTabToRead->dynsym = read_symtab_struct(fd, secTab, SHT_DYNSYM);
	symTabToRead->symtab = read_symtab_struct(fd, secTab, SHT_SYMTAB);

	return symTabToRead;
}



// Affichage
void dump_symtab(Symtab_Struct *s) {
	char *STT_VAL[]={"NOTYPE","OBJECT","FUNC","SECTION","FILE","COMMON","TLS"};
	char *STB_VAL[]={"LOCAL","GLOBAL","WEAK"};

	int i = 1;

	printf("\n Table de symboles « %s » contient %i entrées:\n", s->name, s->nbSymbol);
	printf("   Num:    Valeur Tail Type    Lien   Vis      Ndx Nom\n");
	for (i = 0; i < s->nbSymbol; ++i) {
		printf("%6d: ", i);
		printf("%08x ", s->tab[i]->st_value);
		printf("%5d ", s->tab[i]->st_size);
		printf("%-7s ", STT_VAL[ELF32_ST_TYPE(s->tab[i]->st_info)]);
		printf("%-6s ", STB_VAL[ELF32_ST_BIND(s->tab[i]->st_info)]);
		printf("DEFAULT  "); // TODO: Gerer les differentes possibilités (DEFAULT,HIDDEN,PROTECTED)

		switch(s->tab[i]->st_shndx) {
			case SHN_UNDEF:
				printf("UND ");
				break;
			case SHN_ABS:
				printf("ABS ");
				break;

			default:
				printf("%3i ", s->tab[i]->st_shndx);
		}
		printf("%-10s ", get_symbol_name(s->tab,s->symbolNameTable,i));
		printf("\n");
	}
}

void displ_symbolTable(symbolTable *s) {
	if (s->dynsym->nbSymbol > 0) {
		dump_symtab(s->dynsym);
	}
	if (s->symtab->nbSymbol > 0) {
		dump_symtab(s->symtab);
	}
}


void destroy_symtab_struct(Symtab_Struct *s) {
	for(int i = 0; i < s->nbSymbol; i++)
		free(s->tab[i]);
	free(s->symbolNameTable);
	free(s);
}

// Gestion Memoire
void destroy_symbolTable(symbolTable *s) {
	destroy_symtab_struct(s->symtab);
	destroy_symtab_struct(s->dynsym);

	free(s);
}
