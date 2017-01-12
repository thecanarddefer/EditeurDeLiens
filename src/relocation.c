#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>
#include <elf.h>
#include "elf_common.h"
#include "util.h"

#include "symbol.h"
#include "section.h"
#include "relocation.h"
#include "disp.h"

Data_Rel *read_relocationTables(int fd, Section_Table *secTab)
{
    unsigned ind, size;
    Data_Rel *drel = malloc(sizeof(Data_Rel));

    /* Initialisation */
    drel->nb_rel  = 0;
    drel->nb_rela = 0;
    drel->e_rel   = NULL;
    drel->e_rela  = NULL;
    drel->a_rel   = NULL;
    drel->a_rela  = NULL;
    drel->i_rel   = NULL;
    drel->i_rela  = NULL;
    drel->rel     = NULL;
    drel->rela    = NULL;

    for(int i = 0; i < secTab->nb_sections; i++)
    {
        lseek(fd, secTab->shdr[i]->sh_offset, SEEK_SET);
        if(secTab->shdr[i]->sh_type == SHT_REL)
        {
            drel->nb_rel++;
            ind  = drel->nb_rel - 1;
            size = secTab->shdr[i]->sh_size / sizeof(Elf32_Rel);

            /* Allocations */
            drel->e_rel    = realloc(drel->e_rel, sizeof(unsigned)   * drel->nb_rel);
            drel->a_rel    = realloc(drel->a_rel, sizeof(Elf32_Addr) * drel->nb_rel);
            drel->i_rel    = realloc(drel->i_rel, sizeof(unsigned)   * drel->nb_rel);
            drel->rel      = realloc(drel->rel,   sizeof(Elf32_Rel*) * drel->nb_rel);
            drel->rel[ind] = malloc(sizeof(Elf32_Rel*) * size);
            for(int j = 0; j < size; j++)
                drel->rel[ind][j] = malloc(sizeof(Elf32_Rel));

            drel->e_rel[ind] = size;
            drel->a_rel[ind] = secTab->shdr[i]->sh_offset;
            drel->i_rel[ind] = i;

            /* Récupération de la table des réimplantations */
            for(int j = 0; j < size; j++)
            {
                read(fd, &drel->rel[ind][j]->r_offset, sizeof(drel->rel[ind][j]->r_offset));
                read(fd, &drel->rel[ind][j]->r_info,   sizeof(drel->rel[ind][j]->r_info));
            }
        }
        else if(secTab->shdr[i]->sh_type == SHT_RELA)
        {
            drel->nb_rela++;
            ind  = drel->nb_rela - 1;
            size = secTab->shdr[i]->sh_size / sizeof(Elf32_Rela);

            /* Allocations */
            drel->e_rela    = realloc(drel->e_rela, sizeof(unsigned)    * drel->nb_rela);
            drel->a_rela    = realloc(drel->a_rela, sizeof(Elf32_Addr)  * drel->nb_rela);
            drel->i_rela    = realloc(drel->i_rela, sizeof(unsigned)    * drel->nb_rela);
            drel->rela      = realloc(drel->rela,   sizeof(Elf32_Rela*) * drel->nb_rela);
            drel->rela[ind] = malloc(sizeof(Elf32_Rela*) * size);
            for(int j = 0; j < size; j++)
                drel->rela[ind][j] = malloc(sizeof(Elf32_Rela));

            drel->e_rela[ind] = size;
            drel->a_rela[ind] = secTab->shdr[i]->sh_offset;
            drel->i_rela[ind] = i;

            /* Récupération de la table des réimplantations */
            for(int j = 0; j < size; j++)
            {
                read(fd, &drel->rela[ind][j]->r_offset, sizeof(drel->rela[ind][j]->r_offset));
                read(fd, &drel->rela[ind][j]->r_info,   sizeof(drel->rela[ind][j]->r_info));
                read(fd, &drel->rela[ind][j]->r_addend, sizeof(drel->rela[ind][j]->r_addend));
            }
        }
    }

    return drel;
}

// static inline Elf32_Addr get_symbol_value_generic(symbolTable *symTabFull, Elf32_Word info)
Elf32_Addr get_symbol_value_generic(symbolTable *symTabFull, Elf32_Word info)
{
    return isDynamicRel(ELF32_R_TYPE(info)) ?
        symTabFull->dynsym->tab[ELF32_R_SYM(info)]->st_value :
        symTabFull->symtab->tab[ELF32_R_SYM(info)]->st_value;
}

// static inline char *get_symbol_or_section_name(Section_Table *secTab, symbolTable *symTabFull, Elf32_Word info)
char *get_symbol_or_section_name(Section_Table *secTab, symbolTable *symTabFull, Elf32_Word info)
{
    char *buff = isDynamicRel(ELF32_R_TYPE(info)) ?
        get_dynamic_symbol_name(symTabFull, ELF32_R_SYM(info)) :
        get_static_symbol_name(symTabFull,  ELF32_R_SYM(info));
    if(strlen(buff) <= 0)
        buff = (secTab->shdr[ELF32_R_SYM(info)]->sh_info != 0) ?
            get_section_name(secTab, secTab->shdr[ELF32_R_SYM(info)]->sh_info) :
            get_section_name(secTab, ELF32_R_SYM(info));
    return buff;
}

int isDynamicRel(int rel_type) {
    return ((rel_type == R_ARM_TLS_DESC)
    || (rel_type == R_ARM_TLS_DTPMOD32)
    || (rel_type == R_ARM_TLS_DTPOFF32)
    || (rel_type == R_ARM_TLS_TPOFF32)
    || (rel_type == R_ARM_COPY)
    || (rel_type == R_ARM_GLOB_DAT)
    || (rel_type == R_ARM_JUMP_SLOT)
    || (rel_type == R_ARM_RELATIVE));
}

void destroy_relocationTables(Data_Rel *drel)
{
    for(int i = 0; i < drel->nb_rel; i++)
    {
        for(int j = 0; j < drel->e_rel[i]; j++)
            free(drel->rel[i][j]);
        free(drel->rel[i]);
    }
    free(drel->rel);
    for(int i = 0; i < drel->nb_rela; i++)
    {
        for(int j = 0; j < drel->e_rela[i]; j++)
            free(drel->rela[i][j]);
        free(drel->rela[i]);
    }
    free(drel->rela);
    free(drel->e_rel);
    free(drel->e_rela);
    free(drel->a_rel);
    free(drel->a_rela);
    free(drel->i_rel);
    free(drel->i_rela);
    free(drel);
}
