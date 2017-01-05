#ifndef _ELF_COMMON_H_
#define _ELF_COMMON_H_

#include <elf.h>

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
 * @param ehdr: une structure de type Elf32_Ehdr
 * @retourne 0 en cas de succès
 **/
int read_header(int fd, Elf32_Ehdr *ehdr);

/**
 * Lis les sections d'un fichiers ELF 32 bits et stocke les informations dans un tableau de structures
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 * @param shdr: un tableau de structures de type Elf32_Shdr
 * @retourne 0 en cas de succès
 **/
int read_section_header(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr);

/**
 * Lis la table des noms de section d'un fichier ELF 32 bits et retourne la table
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 * @param shdr: un tableau de structures de type Elf32_Shdr initialisé
 * @retourne la table des noms de section
 **/
char *get_section_name_table(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr);

/**
 * Retourne le nom d'une section donnée
 *
 * @param shdr:  un tableau de structures de type Elf32_Shdr initialisé
 * @param table: une chaîne de caractères initialisée contenant la table des noms de section
 * @param index: le numéro d'une section
 * @retourne une chaîne de caractères correspondant au nom de la section
 **/
char *get_section_name(Elf32_Shdr **shdr, char *table, unsigned index);

int get_section_index(int nbSections, Elf32_Shdr **shdr, int shType, int isDyn, char *sectionNameTable);
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
// Elf32_Sym **read_symtab(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, int *nbSymbol, int sectionIndex);

/**
 * Lis la table des noms de symbole .strtab et retourne la table
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param idxStrTab: index de la section .strtab
 * @param shdr: un tableau de structures de type Elf32_Shdr initialisé
 * @retourne la table des noms de symboles
 **/
// char *get_symbol_name_table(int fd, int idxStrTab, Elf32_Shdr **shdr);

/**
 * Retourne le nom d'un symbole donné
 *
 * @param symtab:  un tableau de structure Elf32_Sym initialisé
 * @param table: une chaîne de caractères initialisée contenant la table des noms de section
 * @param index: le numéro d'une section
 * @retourne une chaîne de caractères correspondant au nom du symbole
 **/
char *get_symbol_name(Elf32_Sym **symtab, char *table, unsigned index);

/**
 * Lis la table des réimplantations et stocke les informations dans une structure
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 * @param shdr: un tableau de structures de type Elf32_Shdr initialisé
 * @param drel: une struture de type Data_Rel
 * @retourne 0 en cas de succès
 **/
int read_relocation_header(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, Data_Rel *drel);


#endif
