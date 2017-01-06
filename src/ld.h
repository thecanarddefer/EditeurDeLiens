#ifndef _LD_H_
#define _LD_H_

#include <elf.h>
#include "symbolTable.h"


typedef struct
{
	Elf32_Shdr *ptr_shdr1;
	Elf32_Shdr *ptr_shdr2;
	Elf32_Word size;
	Elf32_Off offset;
} Fusion;

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
 * Ajoute un symbole à la table des symboles
 *
 * @param st:     une structure de type symbolTable
 * @param symtab: un symbole de type Elf32_Sym initialisé
 * @retourne l'indice où le nouveau symbole a été ajouté
 **/
int add_symbol_in_table(symbolTable *st, Elf32_Sym *symtab);

/**
 * Ajoute un symbole à la table des noms de symboles
 *
 * @param symbolNameTable: un pointeur vers une table des noms des symboles
 * @param symbol:          une chaîne de caractères contenant le nom du symbole
 * @param size:            un pointeur vers la taille initiale de la table des noms des symboles
 * POST-CONDITION:         size est mis à jour (size + taille de symbol + 1)
 **/
void add_symbol_in_name_table(char **symbolNameTable, char *symbol, Elf32_Word *size);


#endif
