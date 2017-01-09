#ifndef _SYMBOLTABLE_H_
#define _SYMBOLTABLE_H_

#include <elf.h>
#include "elf_common.h"


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
    int dynsymIndex;
    int symtabIndex;

} symbolTable;

/*
 * Retourne l'index d'une section selon son type.
 *
 * @param secTab:  une structure de type Section_Table initialisé.
 * @param shType: le type de la section recherchée.
 *
 * @retourne l'index (int) de la section de type shType
 */
int get_section_index(Section_Table *secTab, int shType);

/**
 * Retourne le nom d'un symbole donné (par index)
 *
 * @param symtab:  un tableau de structure Elf32_Sym initialisé.
 * @param table: une chaîne de caractères initialisée contenant la table des noms de section.
 * @param index: le numéro d'une section.
 * @retourne une chaîne de caractères correspondant au nom du symbole.
 **/
char *get_symbol_name(Elf32_Sym **symtab, char *table, unsigned index);

/**
 * Retourne le nom d'un symbole statique donné (par index)
 *
 * @param symTabFull: un tableau de structure symbolTable initialisé.
 * @param index: le numéro d'une section.
 * @retourne une chaîne de caractères correspondant au nom du symbole statique.
 **/
char *get_static_symbol_name(symbolTable *symTabFull, unsigned index);

/**
 * Retourne le nom d'un symbole dynamique donné (par index)
 *
 * @param symTabFull: un tableau de structure symbolTable initialisé.
 * @param index: le numéro d'une section.
 * @retourne une chaîne de caractères correspondant au nom du symbole dynamique.
 **/
char *get_dynamic_symbol_name(symbolTable *symTabFull, unsigned index);

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
 * @param shdr: un tableau de structures de type Elf32_Shdr
 * @param idxStrTab: indice de la section .strtab
 * @retourne: le tableau de structure.
 **/
Elf32_Sym **read_Elf32_Sym(int fd, Elf32_Shdr **shdr, int *nbSymbol, int sectionIndex);

/*
 * Lit est crée un structure contenant le contenu des tables de symbole .symtab et .dynsym
 *
 * @param fd:     un descripteur de fichier (ELF32)
 * @param sectab: une structure de type Section_Table initialisée
 *
 * @retourne: un pointeur vers une structure symbolTable.
 */
symbolTable *read_symbolTable(int fd, Section_Table *secTab);

/*
 * Affiche une table de symbole.
 *
 * @param symTabToDisp: un pointeur vers une structure symbolTable contenant les informations à afficher.
 */
void displ_symbolTable(symbolTable *symTabToDisp);

/**
 * Libère la mémoire occupée par une structure symbolTable
 *
 * @param symTabFull: une structure de type symbolTable initialisée
 **/
void destroy_symbolTable(symbolTable *symTabFull);


#endif
