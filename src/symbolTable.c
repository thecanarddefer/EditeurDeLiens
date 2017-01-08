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

int get_section_index(Section_Table *secTab, int shType, int isDyn) {
	int i = 0,
		idx = -1;
	while (i < secTab->nb_sections) {
		if(secTab->shdr[i]->sh_type == shType) {
			if (isDyn && shType == SHT_STRTAB) {
				if (!strcmp(".dynstr",get_section_name(secTab,i))) {
					idx = i;
				}
			}
			else {
				if (shType == SHT_STRTAB){
					if (!strcmp(".strtab",get_section_name(secTab,i))) {
						idx = i;
					}
				}
				else {
					idx = i;
				}
			}
		}
		i++;
	}
	return idx;
}

char *get_symbol_name(Elf32_Sym **symtab, char *table, unsigned index) {
    return &(table[symtab[index]->st_name]);
}

char *get_static_symbol_name(symbolTable *symTabFull, unsigned index) {
    return get_symbol_name(symTabFull->symtab, symTabFull->symbolNameTable, index);
}

char *get_dynamic_symbol_name(symbolTable *symTabFull, unsigned index) {
    return get_symbol_name(symTabFull->dynsym, symTabFull->dynSymbolNameTable, index);
}

Elf32_Sym **read_Elf32_Sym(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, int *nbSymbol, int sectionIndex) {

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

// Affichage
void dump_symtab(int nbSymbol, Elf32_Sym **symtab, char *symbolNameTable, char *name) {
    char *STT_VAL[]={"NOTYPE","OBJECT","FUNC","SECTION","FILE","COMMON","TLS"};
    char *STB_VAL[]={"LOCAL","GLOBAL","WEAK"};

    int i = 1;

    printf("\n Table de symboles « %s » contient %i entrées:\n", name, nbSymbol);
    printf("   Num:    Valeur Tail Type    Lien   Vis      Ndx Nom\n");
    for (i = 0; i < nbSymbol; ++i) {
        printf("%6d: ", i);
        printf("%08x ", symtab[i]->st_value);
        printf("%5d ", symtab[i]->st_size);
        printf("%-7s ", STT_VAL[ELF32_ST_TYPE(symtab[i]->st_info)]);
        printf("%-6s ", STB_VAL[ELF32_ST_BIND(symtab[i]->st_info)]);
        printf("DEFAULT  "); // TODO: Gerer les differentes possibilités (DEFAULT,HIDDEN,PROTECTED)

        switch(symtab[i]->st_shndx) {
            case SHN_UNDEF:
                printf("UND ");
                break;
            case SHN_ABS:
                printf("ABS ");
                break;

            default:
                printf("%3i ", symtab[i]->st_shndx);
        }
        printf("%-10s ", get_symbol_name(symtab,symbolNameTable,i));
        printf("\n");
    }
}

symbolTable *read_symbolTable(int fd, Elf32_Ehdr *ehdr, Section_Table *secTab) {
    int tmpSymtabIndex = -1,
        tmpStrtabIndex = -1;

    symbolTable *symTabToRead;

    symTabToRead = malloc(sizeof(symbolTable));
    // initialisation
    symTabToRead->nbSymbol = 0;
    symTabToRead->nbDynSymbol = 0;
    symTabToRead->symtabIndex = -1;
    symTabToRead->dynsymIndex = -1;
    symTabToRead->symtab = NULL;
    symTabToRead->dynsym = NULL;
    symTabToRead->symbolNameTable = NULL;
    symTabToRead->dynSymbolNameTable = NULL;

    // SHT_DYNSYM
    tmpSymtabIndex = get_section_index(secTab, SHT_DYNSYM, 1);
    if (tmpSymtabIndex != -1) {
        symTabToRead->dynsym = read_Elf32_Sym(fd, ehdr, secTab->shdr, &symTabToRead->nbDynSymbol, tmpSymtabIndex);
        if (symTabToRead->dynsym != NULL) {
            tmpStrtabIndex = get_section_index(secTab, SHT_STRTAB, 1); // DT_SYMTAB
	    symTabToRead->dynsymIndex = tmpStrtabIndex;
            symTabToRead->dynSymbolNameTable = get_name_table(fd,tmpStrtabIndex,secTab->shdr);
            symTabToRead->dynTableName = get_section_name(secTab,tmpSymtabIndex);
        }
    }
    tmpSymtabIndex = get_section_index(secTab, SHT_SYMTAB, 0);
    if (tmpSymtabIndex != -1) {
        symTabToRead->symtab = read_Elf32_Sym(fd, ehdr, secTab->shdr, &symTabToRead->nbSymbol, tmpSymtabIndex);
        if (symTabToRead->symtab != NULL) {
            tmpStrtabIndex = get_section_index(secTab, SHT_STRTAB, 0);
	    symTabToRead->symtabIndex = tmpStrtabIndex;
            symTabToRead->symbolNameTable = get_name_table(fd,tmpStrtabIndex,secTab->shdr);
            symTabToRead->symTableName = get_section_name(secTab,tmpSymtabIndex);
        }
    }
    return symTabToRead;
}

void displ_symbolTable(symbolTable *symTabToDisp) {
    if (symTabToDisp->nbDynSymbol > 0) {
        dump_symtab(symTabToDisp->nbDynSymbol, symTabToDisp->dynsym, symTabToDisp->dynSymbolNameTable, symTabToDisp->dynTableName);
    }
    if (symTabToDisp->nbSymbol > 0) {
        dump_symtab(symTabToDisp->nbSymbol, symTabToDisp->symtab, symTabToDisp->symbolNameTable, symTabToDisp->symTableName);
    }
}

void destroy_symbolTable(symbolTable *symTabFull)
{
	for(int i = 0; i < symTabFull->nbSymbol; i++)
		free(symTabFull->symtab[i]);
	free(symTabFull->symtab);
	for(int i = 0; i < symTabFull->nbDynSymbol; i++)
		free(symTabFull->dynsym[i]);
	free(symTabFull->dynsym);
	free(symTabFull->dynSymbolNameTable);
	free(symTabFull->symbolNameTable);
	free(symTabFull);
}
