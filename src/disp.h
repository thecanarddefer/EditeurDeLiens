#ifndef _DISP_H_
#define _DISP_H_

#include <elf.h>
// #include "elf_common.h"

/**
 * Affiche les informations sur l'en-tête lu
 *
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 **/
void dump_header(Elf32_Ehdr *ehdr);

/**
 * Affiche le contenu brut d'une section
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param secTab: une structure de type Section_Table initialisée
 * @param index: le numéro d'une section
 **/
void dump_section (int fd, Section_Table *secTab, unsigned index);

/**
 * Affiche les informations sur l'en-tête de section lu
 *
 * @param secTab: une structure de type Section_Table initialisée
 * @param offset: l'adresse de décalage (ehdr->e_shoff)
 **/
void dump_section_header(Section_Table *secTab, Elf32_Off offset);


/**
 * Affiche la table des symboles
 *
 * @param s: Une structure Symtab_Struct.
 **/
void dump_symtab(Symtab_Struct *s);

/*
 * Affiche une table de symbole.
 *
 * @param symTabToDisp: un pointeur vers une structure symbolTable contenant les informations à afficher.
 */
void displ_symbolTable(symbolTable *st);

/**
 * Affiche les réimplantations
 *
 * @param ehdr:  une structure de type Elf32_Ehdr initialisée
 * @param secTab: une structure de type Section_Table initialisée
 * @param symTabFull: une structure de type symbolTable
 * @param drel:  une structure de type Data_Rel initialisée
 **/
void dump_relocation(Elf32_Ehdr *ehdr, Section_Table *secTab, symbolTable *symTabFull, Data_Rel *drel);

#endif
