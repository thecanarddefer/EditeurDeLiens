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
 * Recherche si le numéro de section ou le nom de section est valide
 *
 * @param secTab:      une structure de type Section_Table initialisée
 * @param nb_sections: le nombre de sections dans le fichier
 * @param name:        le nom d'une section
 * @param index:       le numéro d'une section
 * @retourne une valeur non-nulle si la section est Val_GNU_MIPS_ABI_FP_DOUBLE
 * EFFET DE BORD: s'il s'agit d'un nom de section et qu'il est valide, index est mis à jour
 **/
int is_valid_section(Section_Table *secTab, Elf32_Half nb_sections, char *name, unsigned *index);

/**
 * Affiche le contenu brut d'une section
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 * @param secTab: une structure de type Section_Table initialisée
 * @param index: le numéro d'une section
 **/
void dump_section (int fd, Elf32_Ehdr *ehdr, Section_Table *secTab, unsigned index);

/**
 * Affiche les informations sur l'en-tête de section lu
 *
 * @param ehdr:   une structure de type Elf32_Ehdr initialisée
 * @param secTab: une structure de type Section_Table initialisée
 **/
void dump_section_header(Elf32_Ehdr *ehdr, Section_Table *secTab);

/**
 * Renvoie si une relocation concerne un symbole dynamique ou non.
 *
 * @param rel_type: le type de la relocation.
 * @retourne: 1 si la relocation porte sur un symbole dynamique, 0 sinon.
 **/
int isDynamicRel(int rel_type);

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
