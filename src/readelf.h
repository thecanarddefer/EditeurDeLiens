#ifndef _READELF_H_
#define _READELF_H_

#include <elf.h>
#include "elf_common.h"
#include "symbolTable.h"

#define DSP_FILE_HEADER     (1 << 0)
#define DSP_SECTION_HEADERS (1 << 1)
#define DSP_HEX_DUMP        (1 << 2)
#define DSP_SYMS            (1 << 3)
#define DSP_RELOCS          (1 << 4)


typedef struct
{
	unsigned display;
	unsigned nb_hexdumps;
	unsigned section_ind[32];
	char     section_str[32][32];
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
 * @param name:        le nom d'une section
 * @param index:       le numéro d'une section
 * @retourne une valeur non-nulle si la section est Val_GNU_MIPS_ABI_FP_DOUBLE
 * EFFET DE BORD: s'il s'agit d'un nom de section et qu'il est valide, index est mis à jour
 **/
int is_valid_section(Section_Table *secTab, char *name, unsigned *index);

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
 * @param secTab: une structure de type Section_Table initialisée
 * @param offset: l'adresse de décalage (ehdr->e_shoff)
 **/
void dump_section_header(Section_Table *secTab, Elf32_Off offset);

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
