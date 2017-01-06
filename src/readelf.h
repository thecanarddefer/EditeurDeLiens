#ifndef _READELF_H_
#define _READELF_H_

#include <elf.h>
#include "elf_common.h"
#include "symbolTable.h"


enum DSP { DSP_NONE, DSP_FILE_HEADER, DSP_SECTION_HEADERS, DSP_HEX_DUMP, DSP_SYMS, DSP_RELOCS };
typedef struct
{
        enum DSP display;
        unsigned section_ind;
        char     section_str[32];
} Arguments;

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
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 * @param shdr:  un tableau de structures de type Elf32_Shdr initialisé
 * @param index: le numéro d'une section
 **/
void dump_section (int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, unsigned index);

/**
 * Affiche les informations sur l'en-tête de section lu
 *
 * @param ehdr:  une structure de type Elf32_Ehdr initialisée
 * @param shdr:  un tableau de structures de type Elf32_Shdr initialisé
 * @param table: la table des noms de section
 **/
void dump_section_header(Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, char *table);

/**
 * Affiche les réimplantations
 *
 * @param ehdr:  une structure de type Elf32_Ehdr initialisée
 * @param drel:  une structure de type Data_Rel initialisée
 **/
void dump_relocation(Elf32_Ehdr *ehdr, Data_Rel *drel, symbolTable *symTabFull);


#endif
