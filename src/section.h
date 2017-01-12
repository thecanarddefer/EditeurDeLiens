#ifndef _SECTION_H
#define _SECTION_H

#include "elf.h"

typedef struct
{
    unsigned nb_sections;
    char *sectionNameTable; // Table des noms de sections
    Elf32_Shdr **shdr;
} Section_Table;

#define BYTES_COUNT     16
#define BLOCKS_COUNT    4
#define BYTES_PER_BLOCK BYTES_COUNT / BLOCKS_COUNT

/**
 * Lis la table des sections et la table des noms de sections et stocke les informations dans une structure
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 * @retourne un pointeur sur une structure de type Section_Table
 **/
Section_Table *read_sectionTable(int fd, Elf32_Ehdr *ehdr);

/**
 * Recherche si le numéro de section ou le nom de section est valide
 *
 * @param secTab:      une structure de type Section_Table initialisée
 * @param name:        le nom d'une section
 * @param index:       le numéro d'une section
 * @retourne une valeur non-nulle si la section est Val_GNU_MIPS_ABI_FP_DOUBLE
 * EFFET DE BORD: s'il s'agit d'un nom de section et qu'il est valide, index est mis à jour
 **/
int is_valid_section(Section_Table *secTab, char *name, unsigned *index);

/**
 * Libère la mémoire occupée par une structure Section_Table
 *
 * @param secTab: une structure de type Section_Table initialisée
 **/
void destroy_sectionTable(Section_Table *secTab);


#endif
