#ifndef _ELF_COMMON_H_
#define _ELF_COMMON_H_

#include "elf.h"
#include "section.h"

/**
 * Lis l'en-tête d'un fichier ELF 32 bits et stocke les informations dans une structure
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @retourne un pointeur sur une structure de type Elf32_Ehdr
 **/
Elf32_Ehdr *read_elf_header(int fd);

/**
 * Libère la mémoire occupée par une structure Elf32_Ehdr
 *
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 **/
void destroy_elf_header(Elf32_Ehdr *ehdr);

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
 * Lis la table des noms de section d'un fichier ELF 32 bits et retourne la table
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param idxSection: index de la section
 * @param shdr: un tableau de structures de type Elf32_Shdr initialisé
 * @retourne la table des noms de section
 **/
char *get_name_table(int fd, int idxSection, Elf32_Shdr **shdr);

/**
 * Retourne le nom d'une section donnée
 *
 * @param secTab: une structure de type Section_Table initialisée
 * @param index: le numéro d'une section
 * @retourne une chaîne de caractères correspondant au nom de la section
 **/
char *get_section_name(Section_Table *secTab, unsigned index);



#endif
