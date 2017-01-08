#ifndef _ELF_COMMON_H_
#define _ELF_COMMON_H_

#include <elf.h>


typedef struct
{
	unsigned nb_sections;
	char *sectionNameTable; // Table des noms de sections
	Elf32_Shdr **shdr;
} Section_Table;

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

/**
 * Lis la table des sections et la table des noms de sections et stocke les informations dans une structure
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 * @retourne un pointeur sur une structure de type Section_Table
 **/
Section_Table *read_sectionTable(int fd, Elf32_Ehdr *ehdr);

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

/**
 * Libère la mémoire occupée par une structure Section_Table
 *
 * @param secTab: une structure de type Section_Table initialisée
 **/
void destroy_sectionTable(Section_Table *secTab);

/**
 * Lis les tables de réimplantations et stocke les informations dans une structure
 *
 * @param fd:     un descripteur de fichier (ELF32)
 * @param secTab: une structure de type Section_Table initialisée
 * @retourne un pointeur sur une struture de type Data_Rel
 **/
Data_Rel *read_relocationTables(int fd, Section_Table *secTab);

/**
 * Libère la mémoire occupée par une structure Data_Rel
 *
 * @param drel: une structure de type Data_Rel initialisée
 **/
void destroy_relocationTables(Data_Rel *drel);


#endif
