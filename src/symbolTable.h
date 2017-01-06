#ifndef _SYMBOLTABLE_H_
#define _SYMBOLTABLE_H_

#include <elf.h>


typedef struct
{
    Elf32_Sym **symtab;
    Elf32_Sym **dynsym;

    char *dynTableName; // Nom de la table des symboles dynamiques (SHT_DYMTAB)
    char *symTableName; // Nom de la table des symboles (SHT_SYMTAB)
    char *dynSymbolNameTable; // Table des noms de symboles de .dynsym (.dynstr)
    char *symbolNameTable; // Table des noms de symboles de .symtab (.strtab)

    int nbDynSymbol; // Nombre de symboles dans .dynsym
    int nbSymbol; // Nombre de symboles dans .symtab

} symbolTable;


/**
 * Affiche la table des symboles
 *
 * @param nbElt: le nombre de symboles dans la table
 * @param symtab:  un tableau de structures de type Elf32_Sym
 * @param table: la table des noms de symbole
 **/
void dump_symtab(int nbSymbol, Elf32_Sym **symtab, char *symbolNameTable, char *name);

/**
 * Lis la table des symboles d'un fichiers ELF 32 bits,
 * stocke et retourne les informations dans un tableau de structures
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 * @param shdr: un tableau de structures de type Elf32_Shdr
 * @param idxStrTab: indice de la section .strtab
 * @retourne: le tableau de structure.
 **/
Elf32_Sym **read_Elf32_Sym(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, int *nbSymbol, int sectionIndex);

/*
 * Lit est crée un structure contenant le contenu des tables de symbole .symtab et .dynsym
 *  
 * @param fd:   un descripteur de fichier (ELF32)
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 * @param shdr: un tableau de structures de type Elf32_Shdr
 * @param sectionNameTable: Table des noms de sections
 *
 * @retourne: un pointeur vers une structure symbolTable.
 */
symbolTable *read_symbolTable(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, char *sectionNameTable);

/*
 * Affiche une table de symbole.
 *  
 * @param symTabToDisp: un pointeur vers une structure symbolTable contenant les informations à afficher.
 */
void displ_symbolTable(symbolTable *symTabToDisp);



#endif
