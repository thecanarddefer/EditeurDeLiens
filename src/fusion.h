#ifndef _FUSION_H_
#define _FUSION_H_

#include <elf.h>
#include "symbolTable.h"


typedef struct
{
	char section[32];
	Elf32_Shdr *ptr_shdr1;
	Elf32_Shdr *ptr_shdr2;
	Elf32_Word size;
	Elf32_Off offset;
} Fusion;

/**
 * Donne la correspondance des numéros de sections d'un fichier d'entrée avec
 * les numéros de section du fichier de sortie
 *
 * @param fusion:      un descrpteur de fichier vers le fichier d'entrée
 * @param nb_sections: le nombre de sections dans le fichier de sortie
 * @param secTab:      une structure de type Section_Table initialisée
 * @retourne un tableau de type Elf32_Section contenant les nouveaux indices de sections
 **/
Elf32_Section *find_new_section_index(Fusion **fusion, unsigned nb_sections, Section_Table *secTab);

/**
 * Recopie une section PROGBITS dans un autre fichier
 *
 * PRÉ-CONDITION: le curseur de fd_out est placé au bon endroit
 * @param fd_in:  un descrpteur de fichier vers le fichier d'entrée
 * @param fd_out: un descrpteur de fichier vers le fichier de sortie
 * @param: size:  le nombre d'octets à recopier
 * @param offset: l'adresse à partir de laquelle lire dans le fichier d'entrée
 * @retourne 0 si la copie s'est bien déroulée
 **/
int write_progbits_in_file(int fd_in, int fd_out, Elf32_Word size, Elf32_Off offset);

/**
 * Met à jour l'index de section d'un symbole
 *
 * @param symbol:      un symbole de type Elf32_Sym à corriger
 * @param newsec:      un tableau de type Elf32_Section contenant les nouveaux index de sections
 * @param nb_sections: le nombre de sections dans le fichier de sortie
 **/
void update_section_index_in_symbol(Elf32_Sym *symbol, Elf32_Section *newsec, unsigned nb_sections);

/**
 * Calcule l'indice d'une sous-chaîne dans une chaîne
 *
 * @param haystack: la chaîne dans laquelle rechercher la sous-chaîne
 * @param needle:   la sous-chaîne
 * @param size:     la taille de haystack
 **/
int find_index_in_name_table(char *haystack, char *needle, Elf32_Word size);

/**
 * Ajoute un symbole à la table des symboles
 *
 * @param st:     une structure de type symbolTable
 * @param symtab: un symbole de type Elf32_Sym initialisé
 * @retourne l'indice où le nouveau symbole a été ajouté
 **/
int add_symbol_in_table(Symtab_Struct *st, Elf32_Sym *symtab);

/**
 * Trie une table des symboles
 *
 * @param st: une structure de type symbolTable
 **/
void sort_new_symbol_table(Symtab_Struct *st);


#endif
