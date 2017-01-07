#ifndef _ELF_COMMON_H_
#define _ELF_COMMON_H_

#include <elf.h>


typedef struct
{
	char *sectionNameTable; // Table des noms de sections
	Elf32_Shdr **shdr;
} Section_Table;

typedef struct
{
	unsigned nb_rel, nb_rela;   // Nombre de sections concernées
	unsigned *e_rel, *e_rela;   // Nombre d'entrées par sections
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
 * @param shdr:  un tableau de structures de type Elf32_Shdr initialisé
 * @param table: une chaîne de caractères initialisée contenant la table des noms de section
 * @param index: le numéro d'une section
 * @retourne une chaîne de caractères correspondant au nom de la section
 **/
char *get_section_name(Elf32_Shdr **shdr, char *table, unsigned index);

/*
 * Retourne l'index d'une section selon son type.
 *
 * @param nbSections: le nombre de sections
 * @param shdr:  un tableau de structures de type Elf32_Shdr initialisé.
 * @param shType: le type de la section recherchée.
 * @param isDyn: Si l'on cherche une section dynamique.
 * @param sectionNameTable: une chaîne de caractères initialisée contenant la table des noms de section.
 */
int get_section_index(int nbSections, Elf32_Shdr **shdr, int shType, int isDyn, char *sectionNameTable);

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
 * Lis les tables de réimplantations et stocke les informations dans une structure
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 * @param shdr: un tableau de structures de type Elf32_Shdr initialisé
 * @retourne un pointeur sur une struture de type Data_Rel
 **/
Data_Rel *read_relocationTables(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr);


#endif
