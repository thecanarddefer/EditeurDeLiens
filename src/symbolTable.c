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



//TODO: char *get_name_table(int fd, Elf32_Word tableIndx, Elf32_Shdr)
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

symbolTable *read_symbolTable(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, char *sectionNameTable) {
    int tmpSymtabIndex = -1,
        tmpStrtabIndex = -1;

    symbolTable *symTabToRead;

    symTabToRead = malloc(sizeof(symbolTable));
    // initialisation
    symTabToRead->nbSymbol = 0;
    symTabToRead->nbDynSymbol = 0;
    symTabToRead->symtab = NULL;
    symTabToRead->dynsym = NULL;
    symTabToRead->symbolNameTable = NULL;
    symTabToRead->dynSymbolNameTable = NULL;

    // SHT_DYNSYM
    tmpSymtabIndex = get_section_index(ehdr->e_shnum,shdr, SHT_DYNSYM, 1, sectionNameTable);
    if (tmpSymtabIndex != -1) {
        symTabToRead->dynsym = read_Elf32_Sym(fd, ehdr, shdr, &symTabToRead->nbDynSymbol, tmpSymtabIndex);
        if (symTabToRead->dynsym != NULL) {
            tmpStrtabIndex = get_section_index(ehdr->e_shnum,shdr, SHT_STRTAB, 1, sectionNameTable); // DT_SYMTAB
            symTabToRead->dynSymbolNameTable = get_symbol_name_table(fd,tmpStrtabIndex,shdr);
            symTabToRead->dynTableName = get_section_name(shdr,sectionNameTable,tmpSymtabIndex);
        }
    }
    tmpSymtabIndex = get_section_index(ehdr->e_shnum,shdr, SHT_SYMTAB, 0, sectionNameTable);
    if (tmpSymtabIndex != -1) {
        symTabToRead->symtab = read_Elf32_Sym(fd, ehdr, shdr, &symTabToRead->nbSymbol, tmpSymtabIndex);
        if (symTabToRead->symtab != NULL) {
            tmpStrtabIndex = get_section_index(ehdr->e_shnum,shdr, SHT_STRTAB, 0, sectionNameTable);
            symTabToRead->symbolNameTable = get_symbol_name_table(fd,tmpStrtabIndex,shdr);
            symTabToRead->symTableName = get_section_name(shdr,sectionNameTable,tmpSymtabIndex);
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
