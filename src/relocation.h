#ifndef _RELOCATION_H_
#define _RELOCATION_H_

#include "elf.h"

typedef struct
{
    unsigned nb_rel, nb_rela;   // Nombre de sections concernées
    unsigned *e_rel, *e_rela;   // Nombre d'entrées par sections
    unsigned *i_rel, *i_rela;   // Index de la section correspondante
    Elf32_Addr *a_rel, *a_rela; // Adresse de décalage par sections
    Elf32_Rel  ***rel;
    Elf32_Rela ***rela;
} Data_Rel;


/**
 * Lis les tables de réimplantations et stocke les informations dans une structure
 *
 * @param fd:     un descripteur de fichier (ELF32)
 * @param secTab: une structure de type Section_Table initialisée
 * @retourne un pointeur sur une struture de type Data_Rel
 **/
Data_Rel *read_relocationTables(int fd, Section_Table *secTab);

/**
 * Renvoie si une relocation concerne un symbole dynamique ou non.
 *
 * @param rel_type: le type de la relocation.
 * @retourne: 1 si la relocation porte sur un symbole dynamique, 0 sinon.
 **/
int isDynamicRel(int rel_type);

/**
 * Libère la mémoire occupée par une structure Data_Rel
 *
 * @param drel: une structure de type Data_Rel initialisée
 **/
void destroy_relocationTables(Data_Rel *drel);

Elf32_Addr get_symbol_value_generic(symbolTable *symTabFull, Elf32_Word info);
// static inline Elf32_Addr get_symbol_value_generic(symbolTable *symTabFull, Elf32_Word info);

char *get_symbol_or_section_name(Section_Table *secTab, symbolTable *symTabFull, Elf32_Word info);
// static inline char *get_symbol_or_section_name(Section_Table *secTab, symbolTable *symTabFull, Elf32_Word info);


#endif
