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
#include "section.h"

Section_Table *read_sectionTable(int fd, Elf32_Ehdr *ehdr)
{
    Section_Table *secTab = malloc(sizeof(Section_Table));
    secTab->shdr = malloc(sizeof(Elf32_Shdr*) * ehdr->e_shnum);

    for(int i = 0; i < ehdr->e_shnum; i++)
    {
        secTab->shdr[i] = malloc(sizeof(Elf32_Shdr));
        lseek(fd, ehdr->e_shoff + i * ehdr->e_shentsize, SEEK_SET);
        read(fd, &secTab->shdr[i]->sh_name,      sizeof(secTab->shdr[i]->sh_name));
        read(fd, &secTab->shdr[i]->sh_type,      sizeof(secTab->shdr[i]->sh_type));
        read(fd, &secTab->shdr[i]->sh_flags,     sizeof(secTab->shdr[i]->sh_flags));
        read(fd, &secTab->shdr[i]->sh_addr,      sizeof(secTab->shdr[i]->sh_addr));
        read(fd, &secTab->shdr[i]->sh_offset,    sizeof(secTab->shdr[i]->sh_offset));
        read(fd, &secTab->shdr[i]->sh_size,      sizeof(secTab->shdr[i]->sh_size));
        read(fd, &secTab->shdr[i]->sh_link,      sizeof(secTab->shdr[i]->sh_link));
        read(fd, &secTab->shdr[i]->sh_info,      sizeof(secTab->shdr[i]->sh_info));
        read(fd, &secTab->shdr[i]->sh_addralign, sizeof(secTab->shdr[i]->sh_addralign));
        read(fd, &secTab->shdr[i]->sh_entsize,   sizeof(secTab->shdr[i]->sh_entsize));
    }

    secTab->nb_sections      = ehdr->e_shnum;
    secTab->sectionNameTable = get_name_table(fd, ehdr->e_shstrndx, secTab->shdr);

    return secTab;
}

int is_valid_section(Section_Table *secTab, char *name, unsigned *index)
{
    int i;

    if(*index >= secTab->nb_sections)
    {
        fprintf(stderr, "La section %i n'existe pas.\n", *index);
        return 0;
    }

    if(name[0] != '\0')
    {

        for(i = 0; (i < secTab->nb_sections) && strcmp(name, get_section_name(secTab, i)); i++);

        if((i == secTab->nb_sections) || strcmp(name, get_section_name(secTab, i)))
        {
            fprintf(stderr, "La section '%s' n'existe pas.\n",name);
            return 0;
        }
        else
            *index = i;
    }

    return 1;
}

void destroy_sectionTable(Section_Table *secTab)
{
    for(int i = 0; i < secTab->nb_sections; i++)
        free(secTab->shdr[i]);
    free(secTab->shdr);
    free(secTab->sectionNameTable);
    free(secTab);
}
